#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "staged/staged.h"
// within JM
#include "defines.h"
#include "typedefs.h"

using namespace std;
using namespace cola;
using builder::static_var;
using builder::dyn_var;
using builder::dyn_arr;

using sint = static_var<int>;
using dint = dyn_var<int>;
using dshort = dyn_var<short>;
using dbool = dyn_var<bool>;

static Iter<'y'> y;
static Iter<'x'> x;

#define PD(item) cprint(#item " = %d\\n", item)
#define PB(item) cprint(#item " = %d\\n", item)

// Notes on types in JM:
// 1. imgpel = unsigned short
// 2. mblk->intra_block is linear array where [i] is the i^th mblk in raster order (calloc'd in lencode.c)
// 3. availability depends on whether neighboring mblks are in the same slice, but I don't see any checks for
// that happening in JM...

static dint clip1Y(dint x) {
  if (x < 0) {
    return 0;
  } else if (x > 127) {
    return 127;
  } else {
    return x;
  }
}


template <typename Pred, typename Ref>
static void get_16x16_vertical(Pred &pred, Ref &ref) {
  pred[y][x] = ref[-1][x];
}

template <typename Pred, typename Ref>
static void get_16x16_horizontal(Pred &pred, Ref &ref) {
  pred[y][x] = ref[y][-1];
}

// TODO reductions! Start with naive generation (all comps at innermost loop level) then expand it out
template <typename Pred, typename Ref>
static void get_16x16_dc(Pred &pred, Ref &ref, 
			 dbool &left_available, dbool &up_available) { 
  auto p = ref;
  if (up_available && left_available) {
    dint s = 0;
    for (dint q = 0; q < 16; q=q+1) {
      s += p(-1,q) + p(q,-1);
    }
    cprint("DC 1\\n");
    pred[y][x] = crshift((s + 16), 5);
    // ideally want: 
    // pred[i][j] = ((p[-1][ri] + p[rj][-1]) + 16) >> 5
    // but actually, this is ambiguous--do you add 16 and shift by 5 every
    // iteration, or at the end (I want the latter, but that's not 
    // necessarily clear.
    // think it would be like
    // pred[i][j] = (p[-1][ri] + p[rj][-1]);
    // pred[i][j] = (pred[i][j] + 16) >> 5
    // and then you'd fuse them
  } else if (up_available) {
    dint s = 0;
    for (dint q = 0; q < 16; q=q+1) {
      s += p(-1,q);
    }
    cprint("DC 2\\n");
    pred[y][x] = crshift((s + 8), 4);
  } else if (left_available) {
    dint s = 0;
    for (dint q = 0; q < 16; q=q+1) {
      s += p(q,-1);
    }
    cprint("DC 3\\n");
    pred[y][x] = crshift((s + 8), 4);
  } else {
    pred[y][x] = 128;
  }
}

template <typename Pred, typename Ref>
static void get_16x16_plane(Pred &pred, Ref &ref) {
  auto p = ref;
  dint H = 0;
  dint V = 0;
  for (dint q = 0; q < 8; q=q+1) {
    H += (q+1)*(p(-1,8+q) - p(-1,6-q));
    V += (q+1)*(p(8+q,-1) - p(6-q,-1));
  }
  dint a = 16 * (p(15,-1) + p(-1,15));
  dint b = crshift((dint)(5 * H + 32), 6);
  dint c = crshift((dint)(5 * V + 32), 6);
  // TODO need to support conditionals within this notation
  for (dint y = 0; y < 16; y=y+1) {
    for (dint x = 0; x < 16; x=x+1) {
      pred[y][x] = clip1Y(crshift((dint)(a+b*(x-7)+c*(y-7)+16),5));
    }
  }
}

// availability depends on:
// 1. use_constrained_intra
// 2. whether the data physically exists
// 3. the availability marker for the surrounding mblks which seem to have 
// additionaly constraints
template <typename T, bool M, typename IntraPred>
static void set_intrapred_16x16(View<T,2,M> &mb, 
				dbool &up_available,  
				dbool &left_available,
				dbool &all_available,
				dint &mbAddrB_available, // up
				dint &mbAddrA_available, // left
				dint &mbAddrD_available, // up and left
				dint &use_constrained_intra,
				IntraPred &&intra) {
  // for constrainedIntraPred, you can only use surrounding data that also uses intrapred
  up_available = mb.logically_exists(-1,0) && mbAddrB_available;
  left_available = mb.logically_exists(0,-1) && mbAddrA_available;
  all_available = up_available && left_available && mbAddrD_available;
  if (use_constrained_intra==1) {
    auto coloc = intra.colocate(mb);
    up_available = up_available && (coloc(-16,0) != 0);
    left_available = left_available && (coloc(0,-16) != 0);
    all_available = all_available && (coloc(-16,-16) != 0);
  }
}

template <typename Pred, typename Orig>
static dint sad_16x16(Pred &pred, Orig &orig_mblk, dyn_var<int32_t/*distblk*/> min_cost) {
  dint cost = crshift(min_cost, LAMBDA_ACCURACY_BITS);
  // TODO the JM version early exits on each row, as I have here. But see if we can do something 
  // better (like use the SAD function)
  dint i32_cost = 0;
  for (dint i = 0; i < 16; i=i+1) {
    for (dint j = 0; j < 16; j=j+1) {
      i32_cost += cabs(orig_mblk(i,j) - pred(i,j));
    }
    // buildit generates some funky self-comparisons like var > var for this, and they are incorrect
//    if (i32_cost > cost) {
//      return min_cost;
//    }
  }
  return clshift(i32_cost, LAMBDA_ACCURACY_BITS);
}

template <typename Pred, typename Orig>
static dint select_best(dint &best_cost, dint &best_mode, 
			Pred &pred, Orig &&orig_mblk,
			const dint &cur_mode) {
  dyn_var<int32_t/*distblk*/> cur_cost = sad_16x16(pred, orig_mblk, best_cost);
  if (cur_cost < best_cost) {
    best_cost = cur_cost;
    best_mode = cur_mode;
  }
  return cur_cost;
}

// TODO create the appropriate structs and pass them in rather than passing in individual 
// values here.
// TODO support distblk in buildit (int64 in the current case I'm working on) because it fails. Using int32 instead
static void find_sad_16x16_CoLa(dyn_var<imgpel**> raw_img_orig,
				dyn_var<imgpel**> raw_img_recons,
				dyn_var<short*> raw_intra_block,
				// image size in pixels
				dshort pixelH, dshort pixelW,
				// image size in macroblocks
				dshort mbH, dshort mbW,
				// macroblock location in pixels
				dshort mbY, dshort mbX,
				dint use_constrained_intra_pred,
				dint mbAddrB_available,
				dint mbAddrA_available,
				dint mbAddrD_available,
				dint P444_joined,
				dint intra_disable_inter_only,
				dint slice_type,
				dint intra_16x16_hv_disable,
				dint intra_16x16_plane_disable
				) {
  // set up our HMDAs
  auto img_recons = Block<imgpel, 2, true>::user({pixelH, pixelW}, raw_img_recons);
  auto img_orig = Block<imgpel, 2, true>::user({pixelH, pixelW}, raw_img_orig);
  auto intra_block = Block<short,2>::user({mbH, mbW}, raw_intra_block).logically_interpolate(16,16);
  auto mblk = img_recons.view(slice(mbY,mbY+16,1), slice(mbX,mbX+16,1));
  dbool up_available = false;
  dbool left_available = false;
  dbool all_available = false;
  // figure out what's available for the macroblock
  set_intrapred_16x16(mblk, up_available, left_available, all_available,
		      mbAddrB_available,
		      mbAddrA_available,
		      mbAddrD_available,
		      use_constrained_intra_pred,
		      intra_block);
  if (P444_joined) {
    cprint("Not supporting P444 joined.\\n");
    hexit(-1);
  }
  dyn_var<int32_t/*distblk*/> best_cost = INT_MAX;
  dint best_mode = 0;
  // TODO optimization for converting into a reduction, or a special syntax for a reduction
  // there are two reductions here--one within the SAD computation, and one for selecting 
  // the best mode
  for (sint mode = 0; mode < 4; mode=mode+1) {
    // check for disabled modes
    dbool break_out = false;
    if (intra_disable_inter_only==0 || (slice_type != I_SLICE && slice_type != SI_SLICE)) {
      if (intra_16x16_hv_disable != 0 && (mode == VERT_PRED_16 || mode == HOR_PRED_16)) {
	//	continue; // breaks buildit
	break_out = true;
      }
      if (intra_16x16_plane_disable != 0 && mode == PLANE_16) {
	//	continue; // breaks buildit
	break_out = true;
      }
    }
    if (!break_out) {//intra_disable_inter_only != 0 && (slice_type == I_SLICE || slice_type == SI_SLICE)) {
      // now run the modes based on what data is available
      auto pred = Block<imgpel, 2, false>::stack<16,16>();
      if (mode == VERT_PRED_16 && up_available) {
	get_16x16_vertical(pred, mblk);
	dint cur_cost = select_best(best_cost, best_mode, pred, img_orig.colocate(mblk), mode);
      } else if (mode == HOR_PRED_16 && left_available) {
	get_16x16_horizontal(pred, mblk);
	dint cur_cost = select_best(best_cost, best_mode, pred, img_orig.colocate(mblk), mode);
      } else if (mode == PLANE_16 && all_available) {
	get_16x16_plane(pred, mblk);
	dint cur_cost = select_best(best_cost, best_mode, pred, img_orig.colocate(mblk), mode);
      } else if (mode == DC_PRED_16) {
	// DC mode
	get_16x16_dc(pred, mblk, left_available, up_available);
	dint cur_cost = select_best(best_cost, best_mode, pred, img_orig.colocate(mblk), mode);
	for (dint i = 0; i < 16; i=i+1) {
	  for (dint j = 0; j < 16; j=j+1) {
	    cprint("%d ", pred(i,j));
	  }
	  cprint("\\n");
	}
	cprint("\\n");
      }
    }
  }
  cprint("Best mode and cost = (%d,%d)\\n", best_mode, best_cost);
}

int main(int argc, char **argv) {
  stage(find_sad_16x16_CoLa, false, "find_sad_16x16_CoLa", basename(__FILE__, "_generated"), "", "");
}
