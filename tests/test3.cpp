#include <iostream>
#include <fstream>
#include <sstream>

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "staged/staged.h"
#include "staged/test_utils.h"

using builder::dyn_var;
using builder::dyn_arr;
using namespace shim;
using darr2 = dyn_arr<int,2>;

static void staged(builder::dyn_var<int**> data) {
  auto block = Block<int,2,true>::user({10,20},data);
  auto view = block.view();
  auto view2 = block.view(slice(0,10,2),slice(3,19,4));
  ASSERT(compare_arrays(block.bextents, darr2{10, 20}));
  ASSERT(compare_arrays(block.bstrides, darr2{1,1}));
  ASSERT(compare_arrays(block.borigin, darr2{0,0}));
  ASSERT(compare_arrays(view.bextents, darr2{10,20}));
  ASSERT(compare_arrays(view.bstrides, darr2{1,1}));
  ASSERT(compare_arrays(view.borigin, darr2{0,0}));
  ASSERT(compare_arrays(view.vextents, darr2{10,20}));
  ASSERT(compare_arrays(view.vstrides, darr2{1,1}));
  ASSERT(compare_arrays(view.vorigin, darr2{0,0}));
  ASSERT(compare_arrays(view2.bextents, darr2{10,20}));
  ASSERT(compare_arrays(view2.bstrides, darr2{1,1}));
  ASSERT(compare_arrays(view2.borigin, darr2{0,0}));
  ASSERT(compare_arrays(view2.vextents, darr2{5,4}));
  ASSERT(compare_arrays(view2.vstrides, darr2{2,4}));
  ASSERT(compare_arrays(view2.vorigin, darr2{0,3}));
  Iter<'i'> i;
  Iter<'j'> j;
  view2[i][j] = 13;
  for (dyn_var<int> m = 0; m < 5; m=m+1) {
    for (dyn_var<int> n = 0; n < 4; n=n+1) {
      ASSERT(view2(m,n) == 13);
    }
  }
  ASSERT(block(0,3) == 13);
  ASSERT(block(0,7) == 13);
  ASSERT(block(0,11) == 13);
  ASSERT(block(0,15) == 13);
  ASSERT(block(2,3) == 13);
  ASSERT(block(2,7) == 13);
  ASSERT(block(2,11) == 13);
  ASSERT(block(2,15) == 13);
  ASSERT(block(4,3) == 13);
  ASSERT(block(4,7) == 13);
  ASSERT(block(4,11) == 13);
  ASSERT(block(4,15) == 13);
  ASSERT(block(6,3) == 13);
  ASSERT(block(6,7) == 13);
  ASSERT(block(6,11) == 13);
  ASSERT(block(6,15) == 13);
  ASSERT(block(8,3) == 13);
  ASSERT(block(8,7) == 13);
  ASSERT(block(8,11) == 13);
  ASSERT(block(8,15) == 13);
}

int main() {
  test_stage(staged, __FILE__);
}
