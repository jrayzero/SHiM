// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "common/loop_type.hpp"
#include "fwrappers.hpp"

using builder::dyn_var;
using builder::dyn_arr;
using builder::static_var;

namespace shim {

template <unsigned long Rank>
using Property = dyn_arr<loop_type,Rank>;

#define DISPATCH_PRINT_ELEM(dtype, formatter)		\
  template <>						\
  struct DispatchPrintElem<dtype> {			\
    template <typename Val>				\
    void operator()(Val val) { print(formatter, val); }	\
  }

template <typename T>
struct DispatchPrintElem { };
DISPATCH_PRINT_ELEM(uint8_t, "%u ");
DISPATCH_PRINT_ELEM(uint16_t, "%u ");
DISPATCH_PRINT_ELEM(uint32_t, "%u ");
DISPATCH_PRINT_ELEM(uint64_t, "%u ");
DISPATCH_PRINT_ELEM(char, "%c ");
DISPATCH_PRINT_ELEM(int8_t, "%d ");
DISPATCH_PRINT_ELEM(int16_t, "%d ");
DISPATCH_PRINT_ELEM(int32_t, "%d ");
DISPATCH_PRINT_ELEM(int64_t, "%d ");
DISPATCH_PRINT_ELEM(float, "%f ");
DISPATCH_PRINT_ELEM(double, "%f ");

template <typename Elem, typename Val>
void dispatch_print_elem(Val val) {
  DispatchPrintElem<Elem>()(val);
}

/// Return the physical layout of the underlying data based on whether MultiDimPtr==true 
/// (return Rank) or false (return 1)
template <unsigned long Rank, bool MultiDimPtr>
constexpr int physical() {
  if constexpr (MultiDimPtr) {
    return Rank;
  } else {
    return 1;
  }
}

template <typename Elem>
struct Peel { constexpr int operator()() { return 0; } };
template <typename Elem>
struct Peel<Elem*> { constexpr int operator()() { return 1 + Peel<Elem>()(); } };

template <typename Elem>
constexpr int peel() {
  return Peel<Elem>()();
}

template <typename A, typename B>
struct TupleTypeCat { };

template <typename Idx, typename...Idxs>
struct TupleTypeCat<Idx, std::tuple<Idxs...>> {
  using type = std::tuple<Idxs...,Idx>;
};

template <typename Idx>
struct RefIdxType { 
  using type = Idx;
};

template <>
struct RefIdxType<builder::builder> {
  using type = dyn_var<loop_type>;
};

template <>
struct RefIdxType<dyn_var<loop_type>> {
  using type = dyn_var<loop_type>;
};

template <int N, int Depth, unsigned long Sz, typename Elem>
auto dyn_arr_to_tuple(const dyn_arr<Elem,Sz> &arr) {
  if constexpr (Depth == N - 1) {
    return std::tuple{arr[Depth]};
  } else if constexpr (Depth < N - 1) {
    return std::tuple_cat(std::tuple{arr[Depth]}, dyn_arr_to_tuple<N,Depth+1,Sz,Elem>(arr));
  } else {
    return std::tuple{};
  }
}



}
