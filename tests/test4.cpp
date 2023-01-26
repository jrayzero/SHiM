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
  auto block = Block<int,2>::stack<10,20>();
  auto block2 = Block<int,2>::stack<4,4>();
  auto block2_view = block2.view(slice(1,4,1),slice(0,4,2));
  Iter<'i'> i;
  Iter<'j'> j;
  block[i][j] = 0;
  block2[i][j] = 0;
  auto block2_coloc = block2.colocate(block2_view);
  block2_coloc[i][j] = 9;
  ASSERT(compare_arrays(block2_view.bextents, darr2{4,4}));
  ASSERT(compare_arrays(block2_view.bstrides, darr2{1,1}));
  ASSERT(compare_arrays(block2_view.borigin, darr2{0,0}));
  ASSERT(compare_arrays(block2_view.vextents, darr2{3,2}));
  ASSERT(compare_arrays(block2_view.vstrides, darr2{1,2}));
  ASSERT(compare_arrays(block2_view.vorigin, darr2{1,0}));
  block.colocate(block2_view)[i][j] = 16;
  ASSERT(block2(1,0) == 9);
  ASSERT(block2(2,0) == 9);
  ASSERT(block2(3,0) == 9);
  ASSERT(block2(1,2) == 9);
  ASSERT(block2(2,2) == 9);
  ASSERT(block2(3,2) == 9);
  ASSERT(block2_coloc(0,0) == 9);
  ASSERT(block2_coloc(0,1) == 9);
  ASSERT(block2_coloc(0,2) == 9);
  ASSERT(block2_coloc(1,0) == 9);
  ASSERT(block2_coloc(1,1) == 9);
  ASSERT(block2_coloc(1,2) == 9);
  ASSERT(block(1,0) == 16);
  ASSERT(block(2,0) == 16);
  ASSERT(block(3,0) == 16);
  ASSERT(block(1,2) == 16);
  ASSERT(block(2,2) == 16);
  ASSERT(block(3,2) == 16);
}

int main() {
  test_stage(staged, __FILE__);
}
