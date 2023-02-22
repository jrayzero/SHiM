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
  auto block = Block<int,2>::stack<10,10>();
  block[i][j] = select(i+j == Or(0,1,2), 9, 1);
  ASSERT(block(0,0) == 9);
  ASSERT(block(0,1) == 9);
  ASSERT(block(0,2) == 9);
  ASSERT(block(1,0) == 9);
  ASSERT(block(1,1) == 9);
  ASSERT(block(2,0) == 9);
  for (dyn_var<int> q = 0; q < 10; q=q+1) {
    for (dyn_var<int> r = 0; r < 10; r=r+1) {
      if (q + r != 0 && q + r != 1 && q + r != 2) {
	ASSERT(block(q,r) == 1);
      }
    }
  }
  block[i][j] = select(Or(0,1,2) == i + j, 8, 2);
  ASSERT(block(0,0) == 8);
  ASSERT(block(0,1) == 8);
  ASSERT(block(0,2) == 8);
  ASSERT(block(1,0) == 8);
  ASSERT(block(1,1) == 8);
  ASSERT(block(2,0) == 8);
  for (dyn_var<int> q = 0; q < 10; q=q+1) {
    for (dyn_var<int> r = 0; r < 10; r=r+1) {
      if (q + r != 0 && q + r != 1 && q + r != 2) {
	ASSERT(block(q,r) == 2);
      }
    }
  }
  block[i][j] = select(Or(0,i) == i + j, 7, 3);
  ASSERT(block(0,0) == 7);
  ASSERT(block(1,0) == 7);
  ASSERT(block(2,0) == 7);
  ASSERT(block(3,0) == 7);
  ASSERT(block(4,0) == 7);
  ASSERT(block(5,0) == 7);
  ASSERT(block(6,0) == 7);
  ASSERT(block(7,0) == 7);
  ASSERT(block(8,0) == 7);
  ASSERT(block(9,0) == 7);
  for (dyn_var<int> q = 0; q < 10; q=q+1) {
    for (dyn_var<int> r = 0; r < 10; r=r+1) {
      if (q + r != 0 && q + r != 1 && q + r != q) {
	ASSERT(block(q,r) == 3);
      }
    }
  }
}

int main() {
  test_stage(staged, __FILE__);
}

