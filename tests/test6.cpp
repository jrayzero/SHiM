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
  Iter<'i'> i;
  Iter<'j'> j;
  Iter<'k'> k;
  Iter<'l'> l;
  auto block = Block<int,4>::stack<2,4,6,8>();
  block[i][j][k][l] = i+j+k+l;
  ASSERT(block(1,1,2,3) == 7);
  ASSERT(block.plidx(259) == 7);  
  
  auto view = block.freeze(1); // set the first dim to 1
 
  ASSERT(view(1,2,3) == 7);
  ASSERT(view.plidx(67) == 7); 
  view[1][2][3] = 199;
  ASSERT(view(1,2,3) == 199);
  // override the frozen dimensions by specifying 4 
  view[0][1][2][3] = 120;
  ASSERT(view(0,1,2,3) == 120);
  // do one with 4 dims on the rhs side
  view[1][2][3] = view[0][1][2][3] * 10;
  ASSERT(view(1,1,2,3) == 1200); 
  // freeze again, will now have [1,3] frozen
  auto view2 = view.freeze(3);
  view2[4][5] = view[0][1][2][3] + 1;
  ASSERT(view2(4,5) == 121);
  ASSERT(view2(1,3,4,5) == view2(4,5));
  ASSERT(view2(1,3,4,5) == 121);
  // now if you specify too few dimensions, it will pad between the frozen and unfrozen
  view2[0][5] = 37;
  ASSERT(view2(5) == view2(0,5));
  ASSERT(view2(5) == 37);
  ASSERT(view2.plidx(5) == 37);

  // now insert some padding too with inline indexing
  view2[5] = 48;
  view2[3] = 33;
  ASSERT(view2(5) == view2(0,5));
  ASSERT(view2(5) == 48);
  ASSERT(view2.plidx(5) == 48);
  ASSERT(view2(3) == view2(0,3));
  ASSERT(view2(3) == 33);
  ASSERT(view2.plidx(3) == 33);
  view2[5] = view2[3] + 1;
  ASSERT(view2(5) == 34);
}

int main() {
  test_stage(staged, __FILE__);
}

