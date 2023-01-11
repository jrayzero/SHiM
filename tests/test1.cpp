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
using darr = dyn_arr<int,2>;

static void staged() {
  auto block = Block<int,2>::stack<10,20>();
  auto view = block.view();
  auto view2 = block.view(slice(0,10,2),slice(3,19,4));
  cola_assert(compare_arrays(block.bextents, darr{10, 20}), "compare_arrays(block.bextents, darr{10, 20})");
  cola_assert(compare_arrays(block.bstrides, darr{1,1}), "compare_arrays(block.bstrides, darr{1,1})");
  cola_assert(compare_arrays(block.borigin, darr{0,0}), "compare_arrays(block.borigin, darr{0,0})");
  cola_assert(compare_arrays(view.bextents, darr{10,20}), "compare_arrays(view.bextents, darr{10,20})");
  cola_assert(compare_arrays(view.bstrides, darr{1,1}), "compare_arrays(view.bstrides, darr{1,1})");
  cola_assert(compare_arrays(view.borigin, darr{0,0}), "compare_arrays(view.borigin, darr{0,0})");
  cola_assert(compare_arrays(view.vextents, darr{10,20}), "compare_arrays(view.vextents, darr{10,20})");
  cola_assert(compare_arrays(view.vstrides, darr{1,1}), "compare_arrays(view.vstrides, darr{1,1})");
  cola_assert(compare_arrays(view.vorigin, darr{0,0}), "compare_arrays(view.vorigin, darr{0,0})");
  cola_assert(compare_arrays(view2.bextents, darr{10,20}), "compare_arrays(view2.bextents, darr{10,20})");
  cola_assert(compare_arrays(view2.bstrides, darr{1,1}), "compare_arrays(view2.bstrides, darr{1,1})");
  cola_assert(compare_arrays(view2.borigin, darr{0,0}), "compare_arrays(view2.borigin, darr{0,0})");
  cola_assert(compare_arrays(view2.vextents, darr{5,4}), "compare_arrays(view2.vextents, darr{5,4})");
  cola_assert(compare_arrays(view2.vstrides, darr{2,4}), "compare_arrays(view2.vstrides, darr{2,4})");
  cola_assert(compare_arrays(view2.vorigin, darr{0,3}), "compare_arrays(view2.vorigin, darr{0,3})");
}

int main() {
  test_stage(staged, __FILE__);
}
