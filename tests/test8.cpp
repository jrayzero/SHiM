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

static void staged() {
  Iter<'i'> i;
  Iter<'j'> j;
  Iter<'k'> k;
  auto obj = Block<int,2>::heap({3,4});
  auto obj2 = Block<int,2>::heap({3,4});
  obj[i][j] = i*4+j;
  obj2[i][j] = i*4+j;
  {
    auto pobj = obj.permute({0,1});
    ASSERT(pobj(0,1) == 1);
    ASSERT(pobj(1) == 1);
    ASSERT(pobj(2,1) == 9); 
  }
  {
    auto pobj = obj.permute({1,0});
    ASSERT(pobj(1,0) == 1);
    ASSERT(pobj(1,2) == 9);
  }
  {
    auto pobj2 = obj2.permute({1,0});
    obj[i][j] = pobj2[j][i]; // if I did obj2[i][j], then that's just undefined because I'm accessing data outside my area
    ASSERT(obj(0,1) == 1);
    ASSERT(obj(1) == 1);
    ASSERT(obj(2,1) == 9); 
  }

  auto view = obj.slice(range(0,3,2),range(1,4,1));
  auto pobjtmp = obj.permute({1,0});
  auto view2 = pobjtmp.slice(range(1,4,1),range(0,3,2));

  ASSERT(compare_arrays(view.get_view_location().get_extents(), 
			view2.get_view_location().get_extents()));
  ASSERT(compare_arrays(view.get_view_location().get_strides(), 
			view2.get_view_location().get_strides()));
  ASSERT(compare_arrays(view.get_view_location().get_origin(), 
			view2.get_view_location().get_origin()));

  ASSERT(view(0,0) == 1);
  ASSERT(view(0,1) == 2);
  ASSERT(view(0,2) == 3);
  ASSERT(view(1,0) == 9);
  ASSERT(view(1,1) == 10);
  ASSERT(view(1,2) == 11);
  {
    auto pview = view.col_major();
    ASSERT(pview(0,0) == 1);
    ASSERT(pview(1,0) == 2);
    ASSERT(pview(2,0) == 3);
    ASSERT(pview(0,1) == 9);
    ASSERT(pview(1,1) == 10);
    ASSERT(pview(2,1) == 11);
  }

  auto view3 = view.slice(range(0,2,1), range(0,3,2));
  auto pviewtmp = view.permute({1,0});
  auto view4 = pviewtmp.slice(range(0,3,2), range(0,2,1));
  ASSERT(compare_arrays(view3.get_view_location().get_extents(), 
			view4.get_view_location().get_extents()));
  ASSERT(compare_arrays(view3.get_view_location().get_strides(), 
			view4.get_view_location().get_strides()));
  ASSERT(compare_arrays(view3.get_view_location().get_origin(),
			view4.get_view_location().get_origin()));

  ASSERT(view3(0,0) == 1);
  ASSERT(view3(0,1) == 3);
  ASSERT(view3(1,0) == 9);
  ASSERT(view3(1,1) == 11);
  {
    auto pview3 = view3.permute({1,0});
    ASSERT(pview3(0,0) == 1);
    ASSERT(pview3(1,0) == 3);
    ASSERT(pview3(0,1) == 9);
    ASSERT(pview3(1,1) == 11);  
  }
}

int main() {
  test_stage(staged, __FILE__);
}
