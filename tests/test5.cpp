#include <iostream>
#include <fstream>
#include <sstream>

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "staged/staged.h"
#include "staged/test_utils.h"

using builder::dyn_var;
using builder::dyn_arr;
using namespace cola;
using darr2 = dyn_arr<int,2>;

static void staged() {
  Iter<'i'> i;
  Iter<'j'> j;
  auto image = Block<int,2>::stack<64,64>();
  auto mblk = image.view(slice(32,48,1),slice(16,32,1));
  auto intra_pred = Block<int,2>::stack<4,4>();
  auto intra_pred_interp = intra_pred.logically_interpolate(16,16);
  auto intra_at_mblk = intra_pred_interp.colocate(mblk);

  intra_pred[i][j] = 0;
  
  intra_pred_interp[32][16] = 17;
  ASSERT(intra_pred(2,1) == 17);
  ASSERT(intra_pred_interp(32,16) == 17);
  ASSERT(intra_pred_interp.plidx(2064) == 17);
  ASSERT(intra_at_mblk(0,0) == 17);
  ASSERT(intra_at_mblk.plidx(0) == 17);
  ASSERT(intra_at_mblk.reset()(0,0) == 17);

  intra_at_mblk[-16][0] = 19;
  ASSERT(intra_pred(1,1) == 19);
  ASSERT(intra_pred_interp(16,16) == 19);
  ASSERT(intra_at_mblk(-16,0) == 19);
  ASSERT(intra_at_mblk(-11,0) == 19); // should still map to same location as -16,0
  intra_at_mblk.reset()[-2][0] = 12;
  ASSERT(intra_pred(0,1) == 12);
  ASSERT(intra_pred_interp(0,16) == 12);
  ASSERT(intra_at_mblk(-32,0) == 12);

}

int main() {
  test_stage(staged, __FILE__);
}

