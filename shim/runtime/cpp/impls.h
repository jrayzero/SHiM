// -*-c++-*-

#pragma once

#include <sstream>
#include <iostream>
#include <cmath>
#include "heaparray.h"

#define SHIM_ASSERT(cond, msg, file, line)				\
  if (!(cond)) {							\
    std::cerr << "Condition failed (" << (msg) << ") [" << file << ":" << line << "]" << std::endl; \
    exit(-1);								\
  }

namespace shim {

template <typename Elem>
HeapArray<Elem> build_heaparr(loop_type sz) {
  return HeapArray<Elem>(sz);
}

template <typename Elem>
void hmemset(Elem *data, Elem val, loop_type sz) {
  memset(data, val, sz);
}

template <typename Elem>
void hmemset(HeapArray<Elem> &data, Elem val, loop_type sz) {
  memset(data.base->data, val, sz);
}

/*template <bool dummy=false>
void shim_assert(bool cond, std::string msg) {
  if (!cond) {
    std::cerr << "Condition failed (" << msg << ")" << std::endl;
    exit(-1);
  }
}*/


}
