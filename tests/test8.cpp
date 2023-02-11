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
  obj.dump_data();

  shim::permute(obj2, std::tuple{1,0});
  obj[i][j] = obj2[j][i]; // if I did obj2[i][j], then that's just undefined because I'ma ccssing data outside my area
  shim::permute(obj2, std::tuple{0,1});

  obj.dump_data();
/*
  auto obj2 = Block<int,3>::heap({5,5,5});
  // reads

  obj2[i][j][k] = i*j*5+j*5+k;
  ASSERT(obj(1,2,3) == 23);
  ASSERT(obj(0,4,1) == 21);
  ASSERT(obj(4,1) == 21);
  ASSERT(obj(4,4,4) == 104);
  ASSERT(obj2(1,2,3) == 23);
  ASSERT(obj2(0,4,1) == 21);
  ASSERT(obj2(4,4,4) == 104);
  shim::reverse();
  ASSERT(obj(3,2,1) == 23);
  ASSERT(obj(1,4,0) == 21);
  ASSERT(obj(1,4) == 21);
  ASSERT(obj(4,4,4) == 104);
  shim::reverse();
  shim::reverse(obj);
  ASSERT(obj(3,2,1) == 23);
  ASSERT(obj(1,4,0) == 21);
  ASSERT(obj(1,4) == 21);
  ASSERT(obj(4,4,4) == 104);
  ASSERT(obj2(1,2,3) == 23);
  ASSERT(obj2(0,4,1) == 21);
  ASSERT(obj2(4,1) == 21);
  ASSERT(obj2(4,4,4) == 104);
  shim::reverse(obj);
  shim::reverse(obj,obj2);
  ASSERT(obj(3,2,1) == 23);
  ASSERT(obj(1,4,0) == 21);
  ASSERT(obj(1,4) == 21);
  ASSERT(obj(4,4,4) == 104);
  ASSERT(obj2(3,2,1) == 23);
  ASSERT(obj2(1,4,0) == 21);
  ASSERT(obj2(1,4) == 21);
  ASSERT(obj2(4,4,4) == 104);
  shim::reverse(obj,obj2);*/
  // writes
/*  auto obj3 = Block<int,2>::heap({3,4});
  obj3[i][j] = i*4+j;
  ASSERT(obj3(1,2) == 6);
  shim::reverse(obj3);
  ASSERT(obj3(2,1) == 6);
  shim::reverse(obj3);
  shim::reverse(obj3);
  obj3[i][j] = i*4+j;
  obj3.dump_data();
  ASSERT(obj3(0,1) == 4);
  shim::reverse(obj3);*/  
}

int main() {
  test_stage(staged, __FILE__);
}
