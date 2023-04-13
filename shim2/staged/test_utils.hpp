// -*-c++-*-

#pragma once

#include <iostream>
#include <filesystem>
#include "builder/dyn_var.h"
#include "builder/array.h"
#include "common/loop_type.hpp"
#include "fwrappers.hpp"

namespace shim {

#define ASSERT(cond) hassert(cond, #cond, __FILE__, __LINE__)

//void shim_assert(builder::dyn_var<bool> condition, const char *msg="") {
//  hassert(condition, msg);
//}
 
template <typename T, unsigned long N>
builder::dyn_var<bool> compare_arrays(const builder::dyn_arr<T,N> &arr1, const builder::dyn_arr<T,N> &arr2) {
  builder::dyn_var<bool> res = true;
  for (builder::static_var<int> i = 0; i < N; i=i+1) {
    if (arr1[i] != arr2[i]) {
      res = false;
    }
  }
  return res;
}

template <typename Func>
void test_stage(Func func, std::string file) {
  CompileOptions::isCPP = true;
  std::stringstream f;
  std::string f2 = std::filesystem::path(file).stem();
  f << f2 << "_generated";
  stage(func, "staged", f.str(), "", "");
}

}
