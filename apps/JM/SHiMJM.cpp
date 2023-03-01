#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

// BuildIt
#include "builder/dyn_var.h"
#include "builder/array.h"
// Shim
#include "staged/staged.h"
// Shim JM
#include "utils.h"
#include "containers.h"
// within JM
#include "defines.h"
#include "typedefs.h"

using namespace std;
using namespace shim;
using builder::static_var;
using builder::dyn_var;
using builder::dyn_arr;

static Iter<'x'> x;
static Iter<'y'> y;

// Notes on JM:
// - mblk->intra_block is linear array where [i] is the i^th mblk in raster order (calloc'd in lencode.c)
// - availability depends on whether neighboring mblks are in the same slice, but I don't see any checks for
// that happening in JM...

template <typename Pred, typename Ref>
static void get_vertical(Pred &pred, Ref &ref) {
  pred[y][x] = ref[-1][x];
}

template <typename Pred, typename Ref>
static void get_horizontal(Pred &pred, Ref &ref) {
  pred[y][x] = ref[y][-1];
}

template <typename Pred, typename Ref>
static void get_16x16_dc(Pred &pred, Ref &ref, 
			 dbool &left_available, dbool &up_available) { 
  auto &p = ref;
  dint s = 0;
  if (up_available && left_available) {
    for (dint q = 0; q < 16; q=q+1) {
      s += p(-1,q) + p(q,-1);
    }
    s = RSHIFT_RND_SF(s+16,5);
  } else if (up_available) {
    for (dint q = 0; q < 16; q=q+1) {
      s += p(-1,q);
    }
    s = RSHIFT_RND_SF(s+8,4);
  } else if (left_available) {
    for (dint q = 0; q < 16; q=q+1) {
      s += p(q,-1);
    }
    s = RSHIFT_RND_SF(s+8,4);
  } else {
    s = 128;
  }
  pred[y][x] = s;
}

template <typename Pred, typename Ref>
static void get_16x16_plane(Pred &pred, Ref &ref) {
  auto p = ref.col_major();
  dint H = 0;
  dint V = 0;
  for (dint q = 0; q < 8; q=q+1) {
    H += (q+1)*(p(8+q,-1) - p(6-q,-1));
    V += (q+1)*(p(-1,8+q) - p(-1,6-q));
  }
  dint a = 16 * (p(-1,15) + p(15,-1));
  dint b = ((dint)(5 * H + 32) >> 6);
  dint c = ((dint)(5 * V + 32) >> 6);
  pred[y][x] = CLIP1Y((a+b*(x-7)+c*(y-7)+16) >> 5);
}

template <typename Pred, typename Ref>
static void get_4x4_dc(Pred &pred, Ref &ref, 
		       dbool &left_available, dbool &up_available) {
  auto p = ref.col_major();
  dint s = 0;
  if (up_available && left_available) {
    s = (p[0][-1] + p[1][-1] + p[2][-1] + p[3][-1] + p[-1][0] + p[-1][1] + p[-1][2] + p[-1][3] + 4) >> 3;
  } else if (up_available) {
    s = (p[-1][0] + p[-1][1] + p[-1][2] + p[-1][3] + 2) >> 2;
  } else if (left_available) {
    s = (p[0][-1] + p[1][-1] + p[2][-1] + p[3][-1] + 2) >> 2;
  } else {
    s = 128;
  }
  pred[y][x] = s;
}

template <typename Pred, typename Ref>
static void get_4x4_ddl(Pred &pred, Ref &ref) {
  auto p = ref.col_major();
  pred[3][3] = (p[6][-1] + 3 * p[7][-1] + 2) >> 2;
  pred[y][x] = select(y == 3 && x == 3, 
		      (p[6][-1] + 3 * p[7][-1] + 2) >> 2,
		      (p[x + y][-1] + 2 * p[x + y + 1][-1] + p[x + y + 2][-1] + 2) >> 2);
}

template <typename Pred, typename Ref>
static void get_4x4_ddr(Pred &pred, Ref &ref) {
  auto p = ref.col_major();
   pred[y][x] = select(x > y, (p[x-y-2][-1] + 2 * p[x-y-1][-1] + p[x-y][-1] + 2) >> 2,
		      select(x < y, (p[-1][y-x-2] + 2 * p[-1][y-x-1] + p[-1][y-x] + 2) >> 2,
			     (p[0][-1] + 2 * p[-1][-1] + p[-1][0] + 2) >> 2));
}

template <typename Pred, typename Ref>
static void get_4x4_vert_right(Pred &predr, Ref &ref) {
  auto p = ref.col_major();
  auto pred = predr.col_major();
  auto zVR = 2*x-y;
  pred[x][y] = 
    select(zVR == Or(0,2,4,6),
	   (p[x-(y>>1)-1][-1]+p[x-(y>>1)][-1]+1)>>1,
	   select(zVR == Or(1,3,5),
		  (p[x-(y>>1)-2][-1]+2*p[x-(y>>1)-1][-1]+p[x-(y>>1)][-1]+2)>>2,
		  select(zVR == -1,
			 (p[-1][0]+2*p[-1][-1]+p[0][-1]+2)>>2,
			 (p[-1][y-1]+2*p[-1][y-2]+p[-1][y-3]+2)>>2)));
}

template <typename Pred, typename Ref>
static void get_4x4_hor_down(Pred &pred, Ref &ref) {
  auto p = ref.col_major();
  auto zHD = 2*y-x;
  pred[y][x] = select(zHD == Or(0,2,4,6),
		      (p[-1][y - (x >> 1) - 1]+p[-1][y - (x >> 1)]+1) >> 1,
		      select(zHD == Or(1,3,5),
			     (p[-1][y - (x >> 1) - 2]+2 * p[-1][y - (x >> 1) - 1]+p[-1][y - (x >> 1)]+2) >> 2,
			     select(zHD == -1,
				    (p[-1][0]+2 * p[-1][-1]+p[0][-1]+2) >> 2,
				    (p[x - 1][-1]+2 * p[x - 2][-1]+p[x - 3][-1]+2) >> 2)));
}

template <typename Pred, typename Ref>
static void get_4x4_vert_left(Pred &pred, Ref &ref) {
  auto p = ref.col_major();
  pred[y][x] = select(y == Or(0,2),
		      (p[x+(y >> 1)][-1]+p[x+(y >> 1)+1][-1]+1) >> 1,
		      (p[x+(y >> 1)][-1]+2 * p[x+(y >> 1)+1][-1]+p[x+(y >> 1)+2][-1]+2) >> 2);
}

template <typename Pred, typename Ref>
static void get_4x4_hor_up(Pred &pred, Ref &ref) {
  auto p = ref.col_major();
  auto zHU = x+2*y;
  pred[y][x] = select(zHU == Or(0,2,4),
		      (p[-1][y+(x >> 1)]+p[-1][y+(x >> 1)+1]+1) >> 1,
		      select(zHU == Or(1,3),
			     (p[-1][y+(x >> 1)]+2 * p[-1][y+(x >> 1)+1]+p[-1][y+(x >> 1)+2]+2) >> 2,
			     select(zHU == 5,
				    (p[-1][2]+3 * p[-1][3]+2) >> 2,
				    p[-1][3])));
}

// availability depends on:
// 1. use_constrained_intra
// 2. whether the data physically exists
// 3. the availability marker for the surrounding mblks which seem to have 
// additionaly constraints
template <typename T, bool M, typename IntraPred>
static void check_intra16x16_availability(View<T,2,M> &mblk_recons,
				Macroblock &macroblock, 
				dbool &up_available,  
				dbool &left_available,
				dbool &all_available,
				IntraPred &&intra) {
  // for constrainedIntraPred, you can only use surrounding data that also uses intrapred
  up_available = mblk_recons.logically_exists(-1,0) && macroblock->mb_addr_B_available;
  left_available = mblk_recons.logically_exists(0,-1) && macroblock->mb_addr_A_available;
  all_available = up_available && left_available && macroblock->mb_addr_D_available;
  if (macroblock->p_inp->use_constrained_intra_pred==1) {
    auto coloc = intra.colocate(mblk_recons);
    up_available = up_available && (coloc(-1,0) != 0);
    left_available = left_available && (coloc(0,-1) != 0);
    all_available = all_available && (coloc(-1,-1) != 0);
  }
}

template <typename Pred, typename Orig>
static dint sad_16x16(Pred &pred, Orig &orig_mblk, dint min_cost) {
  dint i32_cost = 0;
  for (dint i = 0; i < 16; i=i+1) {
    for (dint j = 0; j < 16; j=j+1) {
      i32_cost += cabs(orig_mblk(i,j) - pred(i,j));
    }
  }
  return (i32_cost << LAMBDA_ACCURACY_BITS);
}

template <typename Pred, typename Orig>
static void select_best(dint &best_cost, dint &best_mode, 
			Pred &pred, Orig &&orig_mblk,
			const dint &cur_mode) {
  dyn_var<int32_t> cur_cost = sad_16x16(pred, orig_mblk, best_cost);
  if (cur_cost < best_cost) {
    best_cost = cur_cost;
    best_mode = cur_mode;
  }
}


static dyn_var<int> find_sad_16x16_Shim(Macroblock macroblock) {
  // set up our HMDAs
  auto pixelH = macroblock->p_inp->source.height[0];
  auto pixelW = macroblock->p_inp->source.width[0];
  auto mbH = macroblock->p_inp->source.mb_height;
  auto mbW = macroblock->p_inp->source.mb_width;
  auto img_orig = Block<imgpel, 2, true>::user({pixelH, pixelW}, 
					       macroblock->p_vid->p_cur_img);
  auto img_recons = Block<imgpel, 2, true>::user({pixelH, pixelW}, 
						 macroblock->p_vid->enc_picture->p_curr_img);
  auto intra_block = Block<short,2>::user(LocationBuilder<2>().with_extents({mbH, mbW}).
					  with_coarsening({16,16}).to_loc(),
					  macroblock->p_vid->intra_block).virtually_refine(16,16);
  auto mblk_recons = img_recons.slice(range(macroblock->pix_y,macroblock->pix_y+16,1), 
				     range(macroblock->pix_x,macroblock->pix_x+16,1));
  dbool up_available = false;
  dbool left_available = false;
  dbool all_available = false;
  // figure out what's available for the macroblock
  check_intra16x16_availability(mblk_recons, macroblock, 
				up_available, left_available, all_available,
				intra_block);
  // NOTE: seems to be a bug with member access inside a conditional
  if (macroblock->p_slice->P444_joined != 0) {
    print("Not supporting P444 joined.\\n");
    hexit(-1);
  }
  dint best_cost = INT_MAX;
  dint best_mode = 0;
  // TODO optimization for converting into a reduction, or a special syntax for a reduction
  // there are two reductions here--one within the SAD computation, and one for selecting 
  // the best mode
  for (sint mode = 0; mode < 4; mode=mode+1) {
    // check for disabled modes
    dbool break_out = false;
    if (macroblock->p_inp->intra_disable_inter_only==0 || 
	(macroblock->p_slice->slice_type != I_SLICE && 
	 macroblock->p_slice->slice_type != SI_SLICE)) {
      if (macroblock->p_inp->intra_16x16_hv_disable != 0 && 
	  (mode == VERT_PRED_16 || mode == HOR_PRED_16)) {
	break_out = true;
      }
      if (macroblock->p_inp->intra_16x16_plane_disable != 0 && 
	  mode == PLANE_16) {
	break_out = true;
      }
    }
    if (!break_out) {
      // now run the modes based on what data is available
      auto pred = Block<imgpel,3,true>::user({4,16,16}, macroblock->p_slice->mpr_16x16[0]);
      if (mode == VERT_PRED_16 && up_available) {	
	auto p = pred.slice(range(VERT_PRED_16,VERT_PRED_16+1,1),range(0,16,1),range(0,16,1));
	get_vertical(p, mblk_recons);
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      } else if (mode == HOR_PRED_16 && left_available) {
	auto p = pred.slice(range(HOR_PRED_16,HOR_PRED_16+1,1),range(0,16,1),range(0,16,1));
	get_horizontal(p, mblk_recons);
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      } else if (mode == PLANE_16 && all_available) {
	auto p = pred.slice(range(PLANE_16,PLANE_16+1,1),range(0,16,1),range(0,16,1));
	get_16x16_plane(p, mblk_recons);
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      } else if (mode == DC_PRED_16) {
	// DC mode
	auto p = pred.slice(range(DC_PRED_16,DC_PRED_16+1,1),range(0,16,1),range(0,16,1));
	get_16x16_dc(p, mblk_recons, left_available, up_available);
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      }
    }
  }  
//  print("Best mode and cost = (%d,%d)\\n", best_mode, best_cost);
  macroblock->i16_mode = best_mode;
  return best_cost;
}

// the block_y, block_x parameters are an unfortunate consequence of how JM stores the locations (it
// does them linearly)
/*static dyn_var<int> find_sad_4x4_Shim(Macroblock macroblock, dint block_y, dint block_x) {
  // TODO refactor the stuff that's for the image and not specific to 4x4/16x16
  // set up our HMDAs
  auto pixelH = macroblock->p_inp->source.height[0];
  auto pixelW = macroblock->p_inp->source.width[0];
  auto mbH = macroblock->p_inp->source.mb_height;
  auto mbW = macroblock->p_inp->source.mb_width;
  auto img_orig = Block<imgpel, 2, true>::user({pixelH, pixelW}, 
					       macroblock->p_vid->p_cur_img);
  auto img_recons = Block<imgpel, 2, true>::user({pixelH, pixelW}, 
						 macroblock->p_vid->enc_picture->p_curr_img);
  auto intra_block = Block<short,2>::user(LocationBuilder<2>().with_extents({mbH, mbW}).
					  with_coarsening({16,16}).to_loc(),
					  macroblock->p_vid->intra_block).virtually_refine(4,4); 
  auto smblk_recons = img_recons.slice(range(block_y,block_y+4), 
				       range(block_x,block_x+4));
  dbool up_available = false;
  dbool left_available = false;
  dbool up_rightavailable = false;
  dbool up_left_available = false;
  // figure out what's available for the macroblock
  check_intra4x4_availability(smblk_recons, macroblock, 
			      up_available, left_available, up_right_available, up_left_available,
			      intra_block);
  // NOTE: seems to be a bug with member access inside a conditional
  if (macroblock->p_slice->P444_joined != 0) {
    print("Not supporting P444 joined.\\n");
    hexit(-1);
  }
  dint best_cost = INT_MAX;
  dint best_mode = 0;
  // TODO optimization for converting into a reduction, or a special syntax for a reduction
  // there are two reductions here--one within the SAD computation, and one for selecting 
  // the best mode
  for (sint mode = 0; mode < 9; mode=mode+1) {
    // check for disabled modes
    dbool break_out = false;
    if (macroblock->p_inp->intra_disable_inter_only==0 || 
	(macroblock->p_slice->slice_type != I_SLICE && 
	 macroblock->p_slice->slice_type != SI_SLICE)) {
      if (macroblock->p_inp->intra_16x16_hv_disable != 0 && 
	  (mode == VERT_PRED_16 || mode == HOR_PRED_16)) {
	break_out = true;
      }
      if (macroblock->p_inp->intra_16x16_plane_disable != 0 && 
	  mode == PLANE_16) {
	break_out = true;
      }
    }
    if (!break_out) {
      // now run the modes based on what data is available
      auto pred = Block<imgpel,3,true>::user({9,4,4}, macroblock->p_slice->mpr_4x4[0]);
      if (mode == VERT_PRED && up_available) {	
	auto p = pred.slice(VERT_PRED,range(0,4),range(0,4));
	
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      } else if (mode == HOR_PRED && left_available) {
	auto p = pred.slice(HOR_PRED,range(0,4),range(0,4));

	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      } else if (mode == DIAG_DOWN_LEFT_PRED && up_available && up_right_available) {
	auto p = pred.slice(DIAG_DOWN_LEFT_PRED,range(0,4),range(0,4));
	
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
	
      } else if (mode == DIAG_DOWN_RIGHT_PRED && up_available && up_left_available && left_available) {
	auto p = pred.slice(DIAG_DOWN_RIGHT_PRED,range(0,4),range(0,4));
	
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
	
      } else if (mode == VERT_RIGHT_PRED && up_available && up_left_available && left_available) {
	auto p = pred.slice(VERT_RIGHT_PRED,range(0,4),range(0,4));
	
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
	
      } else if (mode == HOR_DOWN_PRED && up_available && up_left_available && left_available) {
	auto p = pred.slice(HOR_DOWN_PRED,range(0,4),range(0,4));
	
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
	
      } else if (mode == VERT_LEFT_PRED && up_available && up_right_available) {
	auto p = pred.slice(VERT_LEFT_PRED,range(0,4),range(0,4));
	
	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
	
      } else if (mode == HOR_UP_PRED && up_available) {
	auto p = pred.slice(HOR_UP_PRED,range(0,4),range(0,4));	

	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);	
      } else if (mode == DC_PRED) {
	// DC mode
	auto p = pred.slice(DC_PRED,range(0,4),range(0,4));

	select_best(best_cost, best_mode, p, img_orig.colocate(mblk_recons), mode);
      }
    }
  }
//  print("Best mode and cost = (%d,%d)\\n", best_mode, best_cost);
  macroblock->ipmode_DPCM = NO_INTRA_PMODE;
  return best_cost;
}*/


template <typename T, bool M, typename IntraPred>
static void check_intra4x4_availability(View<T,2,M> &mblk_recons,
					Macroblock &macroblock, 
					dbool &up_available,  
					dbool &left_available,
					dbool &up_right_available,
					dbool &up_left_available,
					dint up_right_offset,
					IntraPred &&intra) {
  // for constrainedIntraPred, you can only use surrounding data that also uses intrapred
//  mblk_recons.dump_loc();
  up_available = mblk_recons.logically_exists(-1,0);// && macroblock->mb_addr_B_available;
  left_available = mblk_recons.logically_exists(0,-1);// && macroblock->mb_addr_A_available;
  up_left_available = mblk_recons.logically_exists(-1,-1);// && macroblock->mb_addr_D_available;
  up_right_available = mblk_recons.logically_exists(-1,up_right_offset);// && macroblock->mb_addr_C_available;
  if (macroblock->p_inp->use_constrained_intra_pred==1) {
    auto coloc = intra.colocate(mblk_recons);
    up_available = up_available && (coloc(-1,0) != 0);
    left_available = left_available && (coloc(0,-1) != 0);
    up_left_available = up_left_available && (coloc(-1,-1) != 0);
    up_right_available = up_right_available && (coloc(-1,up_right_offset) != 0);
  }
}

// also returns the most probable mode
dint compute_4x4_availability(Macroblock macroblock, dint block_y, dint block_x,
			      dyn_var<int*> up_available, dyn_var<int*> left_available,
			      dyn_var<int*> all_available) {
  auto pixelH = macroblock->p_inp->source.height[0];
  auto pixelW = macroblock->p_inp->source.width[0];
  auto mbH = macroblock->p_inp->source.mb_height;
  auto mbW = macroblock->p_inp->source.mb_width;
  auto img_recons = Block<imgpel, 2, true>::user({pixelH, pixelW}, 
						 macroblock->p_vid->enc_picture->p_curr_img);
  auto intra_block = Block<short,2>::user(LocationBuilder<2>().with_extents({mbH, mbW}).
  					  with_coarsening({16,16}).to_loc(),
					  macroblock->p_vid->intra_block).virtually_refine(4,4); 
//  auto ipredmode = Block<short,2>::user(LocationBuilder<2>().with_extents({mbH, mbW}).
//					with_coarsening({4,4}).to_loc(),
//					macroblock->p_vid->intra_block).virtually_refine(4,4); 
  auto smblk_recons = img_recons.slice(range(block_y+macroblock->pix_y,block_y+macroblock->pix_y+4), 
				       range(block_x+macroblock->pix_x,block_x+macroblock->pix_x+4));
  dbool bup_available = false;
  dbool bleft_available = false;
  dbool bup_right_available = false;
  dbool bup_left_available = false;
  // figure out what's available for the macroblock
  check_intra4x4_availability(smblk_recons, macroblock, 
			      bup_available, bleft_available, bup_right_available, bup_left_available, 4,
			      intra_block);
  
  up_available[0] = bup_available;
  left_available[0] = bleft_available;
  all_available[0] = bleft_available && bup_available && bup_left_available;
  return 0;
}

int main(int argc, char **argv) {
  CompileOptions::isCPP = IS_CPP == 1;
  stringstream includes;
  includes << "#include \"global.h\"" << endl;
  includes << "#include \"mbuffer.h\"" << endl;
  // 16x16
  stage(find_sad_16x16_Shim, "find_sad_16x16_Shim", basename(__FILE__, "_generated"), 
	includes.str(), includes.str());
  // 4x4
  stage(compute_4x4_availability, "shim_4x4_available", basename(__FILE__, "_generated"), 
	includes.str(), includes.str());
}
