#pragma once

#include <sstream>
#include <iostream>
#include <cmath>
#include "heaparray.h"

namespace cola {

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

//template <typename To, typename From>
//To cast(From val) { 
//  return (To)val; 
//}

/*template <typename Elem>
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
}*/

template <typename V, typename S>
V lshift(V val, S amt) {
  return val << amt;
}

template <typename V, typename S>
V rshift(V val, S amt) {
  return val >> amt;
}

template <typename V, typename S>
V bor(V left, S right) {
  return left | right;
}

template <typename V, typename S>
V band(V left, S right) {
  return left & right;
}

template <typename V, typename S>
V pow(V left, S right) {
  return std::pow(left, right);
}

template <typename V>
V ceil(V val) {
  return std::ceil(val);
}

template <typename Arg, typename...Args>
void cat(std::stringstream &ss, Arg arg, Args...args) {
  ss << arg;
  if constexpr (sizeof...(Args) > 0) {
    cat(ss, args...);
  }
}

/*template <typename...Args>
void printn(Args...args) {
  std::stringstream ss;
  cat(ss, args...);  
  std::cout << ss.str() << std::endl;
}

template <typename...Args>
void print(Args...args) {
  std::stringstream ss;
  cat(ss, args...);  
  std::cout << ss.str();
}*/

template <bool dummy=false>
void cola_assert(bool cond, std::string msg) {
  if (!cond) {
    std::cerr << "Condition failed (" << msg << ")" << std::endl;
    exit(-1);
  }
}

}
