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
  shim::permute(obj, std::tuple{0,1});
  ASSERT(obj(0,1) == 1);
  ASSERT(obj(1) == 1);
  ASSERT(obj(2,1) == 9); 
  shim::permute(obj, std::tuple{0,1});

  shim::permute(obj, std::tuple{1,0});
  ASSERT(obj(1,0) == 1);
  ASSERT(obj(1,2) == 9);
  shim::permute(obj, std::tuple{0,1});

  shim::permute(obj2, std::tuple{1,0});
  obj[i][j] = obj2[j][i]; // if I did obj2[i][j], then that's just undefined because I'm accessing data outside my area
  shim::permute(obj2, std::tuple{0,1});
  ASSERT(obj(0,1) == 1);
  ASSERT(obj(1) == 1);
  ASSERT(obj(2,1) == 9); 

  auto view = obj.view(slice(0,3,2),slice(1,4,1));
  shim::permute(obj, std::tuple{1,0});
  auto view2 = obj.view(slice(1,4,1),slice(0,3,2));
  shim::permute(obj, std::tuple{0,1});
  ASSERT(compare_arrays(view.vextents, view2.vextents));
  ASSERT(compare_arrays(view.vstrides, view2.vstrides));
  ASSERT(compare_arrays(view.vorigin, view2.vorigin));

  ASSERT(view(0,0) == 1);
  ASSERT(view(0,1) == 2);
  ASSERT(view(0,2) == 3);
  ASSERT(view(1,0) == 9);
  ASSERT(view(1,1) == 10);
  ASSERT(view(1,2) == 11);
  shim::permute(view, std::tuple{1,0});
  ASSERT(view(0,0) == 1);
  ASSERT(view(1,0) == 2);
  ASSERT(view(2,0) == 3);
  ASSERT(view(0,1) == 9);
  ASSERT(view(1,1) == 10);
  ASSERT(view(2,1) == 11);
  shim::permute(view, std::tuple{0,1});

  auto view3 = view.view(slice(0,2,1), slice(0,3,2));
  shim::permute(view, std::tuple{1,0});
  auto view4 = view.view(slice(0,3,2), slice(0,2,1));
  shim::permute(view, std::tuple{0,1});
  ASSERT(compare_arrays(view3.vextents, view4.vextents));
  ASSERT(compare_arrays(view3.vstrides, view4.vstrides));
  ASSERT(compare_arrays(view3.vorigin, view4.vorigin));

  ASSERT(view3(0,0) == 1);
  ASSERT(view3(0,1) == 3);
  ASSERT(view3(1,0) == 9);
  ASSERT(view3(1,1) == 11);
  shim::permute(view3, std::tuple{1,0});
  ASSERT(view3(0,0) == 1);
  ASSERT(view3(1,0) == 3);
  ASSERT(view3(0,1) == 9);
  ASSERT(view3(1,1) == 11);  
  shim::permute(view3, std::tuple{0,1});
}

int main() {
  test_stage(staged, __FILE__);
}
