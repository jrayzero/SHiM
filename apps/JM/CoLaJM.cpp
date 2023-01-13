#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "staged/staged.h"

using namespace std;
using namespace cola;
using builder::static_var;
using builder::dyn_var;
using builder::dyn_arr;

using sint = static_var<int>;
using dint = dyn_var<int>;
using dshort = dyn_var<short>;
using dbool = dyn_var<bool>;

static Iter<'i'> i;
static Iter<'j'> j;

#define PD(item) cprint(#item " = %d\\n", item)
#define PB(item) cprint(#item " = %d\\n", item)

// Notes on types in JM:
// 1. imgpel = unsigned short
// 2. mblk->intra_block is linear array where [i] is the i^th mblk in raster order (calloc'd in lencode.c)
// 3. availability depends on whether neighboring mblks are in the same slice, but I don't see any checks for
// that happening in JM...

// Add on some additional functions to views here
// TODO may want to end up using this to collect all the data that I need
template <typename Elem, unsigned long Rank, bool MultiDimPtr>
struct JMView {

  explicit JMView(View<Elem,Rank,MultiDimPtr> *view) : view(view) { }

  // TODO add this to the library
  /// 
  /// True if all coords are >= 0
  template <typename...Coords>
  dbool logically_exists(Coords...coords) {
    static_assert(sizeof...(coords) == Rank);
    dyn_arr<int,Rank> rel;
    // TODO make this function more user-friendly and have Depth default to 0
    view->template compute_block_relative_iters<0>(dyn_arr<int,Rank>{coords...}, rel);
    for (sint i = 0; i < Rank; i++) {
      if (rel[i] < 0) {
	return false; 
      }
    }
    return true;
  }

  View<Elem,Rank,MultiDimPtr> *view;
  
};

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
JMView<Elem,Rank,MultiDimPtr> to_JMView(View<Elem,Rank,MultiDimPtr> *view) {
  return JMView(view);
}

/*
template <typename Pred, typename Ref>
static void get_16x16_vertical(Pred &pred, Ref &ref) {
  pred[y][x] = ref.colocate(pred)[-1][x];
}

template <typename Pred, typename Ref>
static void get_16x16_horizontal(Pred &pred, Ref &ref) {
  pred[y][x] = ref.colocate(pred)[y][-1];
}

// TODO reductions! Start with naive generation (all comps at innermost loop level) then expand it out
template <typename Pred, typename Ref>
static void get_16x16_dc(Pred &pred, Ref &ref, 
			 dbool &left_available, dbool &up_available) { 
  auto p = ref.colocate(pred);
  if (up_available && left_available) {
    dint s = 0;
    for (dint q = 0; q < 16; q=q+1) {
      s += p(-1,q) + p(q,-1);
    }
    pred[y][x] = (s + 16) > 5;
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
    pred[y][x] = (s + 8) >> 4;
  } else if (left_available) {
    dint s = 0;
    for (dint q = 0; q < 16; q=q+1) {
      s += p(q,-1);
    }
    pred[y][x] = (s + 8) >> 4;
  } else {
    pred[y][x] = (1 << (8-1));
  }
}

template <typename Pred, typename Pred>
static void get_16x16_dc(Pred &pred, Ref &ref) {
  auto p = ref.colocate(pred);
  dint H = 0;
  dint V = 0;
  for (dint q = 0; q < 8; q=q+1) {
    H += (q+1)*(p(-1,8+q) - p(-1,6-q));
    V += (q+1)*(p(8+q,-1) - p(6-q,-1))
  }
  dint a = 16 * (p(15,-1) + p(-1,15));
  dint b = (5 * H + 32) >> 6;
  dint c = (5 * V + 32) >> 6;
  pred[y][x] = clip1Y((a+b*(x-7)+c*(y-7)+16)>>5);
}
*/

// availability depends on:
// 1. use_constrained_intra
// 2. whether the data physically exists
// 3. the availability marker for the surrounding mblks which seem to have 
// additionaly constraints
template <typename T, bool M, typename IntraPred>
static void set_intrapred_16x16(JMView<T,2,M> &mb, 
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
    auto coloc = intra.colocate(*mb.view);
    // now any access to coloc will get divided by the above values BEFORE using the specified coordinates.
    // so coloc(-1,0) below would first divide the block/view extents and origin by 16 in each dimension, and
    // then do the access
    up_available = up_available && (coloc(-16,0) != 0);
    left_available = left_available && (coloc(0,-16) != 0);
    all_available = all_available && (coloc(-16,-16) != 0);
    PB(up_available);
    PB(left_available);
    PB(all_available);
  }
}

static void find_sad_16x16_CoLa(dyn_var<unsigned short**> raw_img_enc,
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
				dint mbAddrD_available
				) {
//  PD(pixelH);
//  PD(pixelW);
//  PD(mbH);
//  PD(mbW);
//  PD(mbY);
//  PD(mbX);
//  PD(use_constrained_intra_pred);
  // set up our HMDAs
  auto img_enc = Block<unsigned short, 2, true>::user({pixelH, pixelW}, raw_img_enc);
  auto intra_block = Block<short,2>::user({mbH, mbW}, raw_intra_block).logically_interpolate(16,16);
  auto mblk = img_enc.view(slice(mbY,mbY+16,1), slice(mbX,mbX+16,1));
  dbool up_available = false;
  dbool left_available = false;
  dbool all_available = false;
  auto jm_mblk = to_JMView(&mblk);
  // figure out what's available for the macroblock
  set_intrapred_16x16(jm_mblk, up_available, left_available, all_available,
		      mbAddrB_available,
		      mbAddrA_available,
		      mbAddrD_available,
		      use_constrained_intra_pred,
		      intra_block);
}

int main(int argc, char **argv) {
  stage(find_sad_16x16_CoLa, false, "find_sad_16x16_CoLa", basename(__FILE__, "_generated"), "", "");
}
