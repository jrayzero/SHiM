#pragma once

#include <iostream>
#include "heaparray.h"

namespace hmda {

template <typename Elem>
HeapArray<Elem> build_heaparr(loop_type sz) {
  return HeapArray<Elem>(sz);
}

template <typename Elem>
Elem read(Elem *data, loop_type lidx) {
  return data[lidx];
}

template <typename Elem>
void write(Elem *data, loop_type lidx, Elem val) {
  data[lidx] = val;
}

template <typename Elem>
Elem read(const HeapArray<Elem> &heaparr, loop_type lidx) {
  return heaparr[lidx];
}

template <typename Elem>
void write(HeapArray<Elem> &heaparr, loop_type lidx, Elem val) {
  heaparr.write(lidx, val);
}

template <typename Elem>
void hmemset(Elem *data, Elem val, loop_type sz) {
  memset(data, val, sz);
}

template <typename Elem>
void hmemset(HeapArray<Elem> &data, Elem val, loop_type sz) {
  memset(data.base->data, val, sz);
}

template <typename To, typename From>
To cast(From val) { 
  return (To)val; 
}

template <typename Elem>
void print_elem(Elem elem) {
  std::cout << elem << " ";
}

template <bool dummy=false>
void print_newline() {
  std::cout << std::endl;
}

template <bool dummy=false>
void print_string(std::string s) {
  std::cout << s;
}

template <typename V, typename S>
V lshift(V val, S amt) {
  return val << amt;
}

template <typename V, typename S>
V rshift(V val, S amt) {
  return val >> amt;
}

}
