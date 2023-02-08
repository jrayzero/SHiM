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

static void staged() {
  Iter<'i'> i;
  Iter<'j'> j;
  auto block = Block<int,2>::stack<5,5>();
  auto view = block.view(slice(1,4,2),slice(2,4,1));
  auto block2 = Block<int,2>(view.vextents, view.vstrides, view.vorigin);
  auto view2 = block2.view();
  block2[i][j] = i*2+j;
  ASSERT(block2(0,0) == 0);
  ASSERT(block2(0,1) == 1);
  ASSERT(block2(1,0) == 2);
  ASSERT(block2(1,1) == 3);
  ASSERT(view2(0,0) == 0);
  ASSERT(view2(0,1) == 1);
  ASSERT(view2(1,0) == 2);
  ASSERT(view2(1,1) == 3);
  auto block3 = Block<int,2>::stack<10,10>();
  auto v = block3.view(slice(1,10,2),slice(1,10,3));
  auto block4 = Block<int,2>(v.vextents, v.vstrides, v.vorigin);
  auto view4 = block4.view(slice(1,5,2), slice(0,3,1));
  block4[i][j] = i*3+j;
  ASSERT(view4(0,0) == 3);
  ASSERT(view4(0,1) == 4);
  ASSERT(view4(0,2) == 5);
  ASSERT(view4(1,0) == 9);
  ASSERT(view4(1,1) == 10);
  ASSERT(view4(1,2) == 11);
}

int main() {
  test_stage(staged, __FILE__);
}

