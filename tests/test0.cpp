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
  auto view = block.slice();
  auto view2 = block.slice(range(0,10,2),range(3,19,4));
  ASSERT(compare_arrays(block.get_location().get_extents(), darr2{10, 20}));
  ASSERT(compare_arrays(block.get_location().get_strides(), darr2{1,1}));
  ASSERT(compare_arrays(block.get_location().get_origin(), darr2{0,0}));
  ASSERT(compare_arrays(view.get_block_location().get_extents(), darr2{10,20}));
  ASSERT(compare_arrays(view.get_block_location().get_strides(), darr2{1,1}));
  ASSERT(compare_arrays(view.get_block_location().get_origin(), darr2{0,0}));
  ASSERT(compare_arrays(view.get_view_location().get_extents(), darr2{10,20}));
  ASSERT(compare_arrays(view.get_view_location().get_strides(), darr2{1,1}));
  ASSERT(compare_arrays(view.get_view_location().get_origin(), darr2{0,0}));
  ASSERT(compare_arrays(view2.get_block_location().get_extents(), darr2{10,20}));
  ASSERT(compare_arrays(view2.get_block_location().get_strides(), darr2{1,1}));
  ASSERT(compare_arrays(view2.get_block_location().get_origin(), darr2{0,0}));
  ASSERT(compare_arrays(view2.get_view_location().get_extents(), darr2{5,4}));
  ASSERT(compare_arrays(view2.get_view_location().get_strides(), darr2{2,4}));
  ASSERT(compare_arrays(view2.get_view_location().get_origin(), darr2{0,3}));
}

int main() {
  test_stage(staged, __FILE__);
}
