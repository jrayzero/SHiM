#include "builder/dyn_var.h"
#include "builder/array.h"
#include "staged/staged.h"
#include "staged/test_utils.h"

using builder::dyn_var;
using builder::dyn_arr;
using namespace shim;
using darr2 = dyn_arr<int,2>;

static void staged() {
  auto block = Block<int,2>::heap({10,20});
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
}

int main() {
  test_stage(staged, __FILE__);
}
