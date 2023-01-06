// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "builder/static_var.h"
#include "common/loop_type.h"
#include "fwrappers.h"
#include "fwddecls.h"
#include "traits.h"

namespace cola {

///
/// Convert a coordinate to a linear index 
template <int Depth, unsigned long Rank>
builder::dyn_var<loop_type> linearize(const Loc_T<Rank> &extents, builder::dyn_arr<loop_type,Rank> &coord) {
  builder::dyn_var<loop_type> c = coord[Rank-1-Depth];
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + extents[Rank-1-Depth] * linearize<Depth+1,Rank>(extents, coord);
  }
}

///
/// Perform a reduction across a region of obj
template <typename Functor, int Begin, int End, int Depth, typename Obj>
auto reduce_region_inner(Obj &obj) {
  if constexpr (Depth >= Begin && Depth < End) {
    builder::dyn_var<loop_type> item = obj[Depth];
    if constexpr (Depth < End - 1) {
      return Functor()(item, reduce_region_inner<Functor,Begin,End,Depth+1>(obj));
    } else {
      return item;
    }
  } else {
    // won't be hit if Depth >= End
    return reduce_region_inner<Functor,Begin,End,Depth+1>(obj);
  }
}

///
/// Perform a reduction across a region of obj
template <typename Functor, int Begin, int End, unsigned long Rank, typename Obj>
auto reduce_region(Obj &obj) {
  static_assert(Begin < Rank && End <= Rank && Begin < End);
  return reduce_region_inner<Functor,Begin,End,0>(obj);
}

///
/// Perform a reduction across a dyn_var<T[]>
template <typename Functor, unsigned long Rank, typename T>
builder::dyn_var<T> reduce(builder::dyn_arr<T,Rank> &arr) {
  builder::dyn_var<T> acc = arr[0];
  for (builder::static_var<T> i = 1; i < Rank; i=i+1) {
    acc = Functor()(acc, arr[i]);
  }
  return acc;
}

///
/// Perform a multiplication reduction across template values
template <loop_type Val, loop_type...Vals>
constexpr loop_type mul_reduce() {
  if constexpr (sizeof...(Vals) == 0) {
    return Val;
  } else {
    return Val * mul_reduce<Vals...>();
  }
}

///
/// Convert a linear index to a coordinate
template <int Depth, unsigned long Rank, typename LIdx>
void delinearize(Loc_T<Rank> &out, LIdx lidx, const Loc_T<Rank> &extents) {
  if constexpr (Depth+1 == Rank) {
    out[Depth] = lidx;
  } else {
    builder::dyn_var<loop_type> m = reduce_region<MulFunctor, Depth+1, Rank, Rank>(extents);
    builder::dyn_var<loop_type> c = lidx / m;
    out[Depth] = c;
    delinearize<Depth+1, Rank>(out, lidx % m, extents);    
  }
}

///
/// Apply Functor to the elements in arr0 and arr1 and store the result in arr
template <typename Functor, unsigned long Rank, int Depth=0>
void apply(Loc_T<Rank> &arr,
	   const Loc_T<Rank> &arr0, 
	   const Loc_T<Rank> &arr1) {
  auto applied = Functor()(arr0[Depth], arr1[Depth]);
  arr[Depth] = applied;
  if constexpr (Depth < Rank-1) {
    apply<Functor,Rank,Depth+1>(arr,arr0, arr1);
  }
}

#define DISPATCH_PRINT_ELEM(dtype)				\
  template <>							\
  struct DispatchPrintElem<dtype> {				\
    template <typename Val>					\
    void operator()(Val val) { print_elem_##dtype(val); }	\
  }

template <typename T>
struct DispatchPrintElem { };
DISPATCH_PRINT_ELEM(uint8_t);
DISPATCH_PRINT_ELEM(uint16_t);
DISPATCH_PRINT_ELEM(uint32_t);
DISPATCH_PRINT_ELEM(uint64_t);
DISPATCH_PRINT_ELEM(char);
DISPATCH_PRINT_ELEM(int8_t);
DISPATCH_PRINT_ELEM(int16_t);
DISPATCH_PRINT_ELEM(int32_t);
DISPATCH_PRINT_ELEM(int64_t);
DISPATCH_PRINT_ELEM(float);
DISPATCH_PRINT_ELEM(double);

///
/// Call the appropriate print_elem function based on Elem
template <typename Elem, typename Val>
void dispatch_print_elem(Val val) {
  DispatchPrintElem<Elem>()(val);
}

///
/// Create the type that resultsing from concatenting Idx to tuple<Idxs...>
template <typename A, typename B>
struct TupleTypeCat { };

template <typename Idx, typename...Idxs>
struct TupleTypeCat<Idx, std::tuple<Idxs...>> {
  using type = std::tuple<Idxs...,Idx>;
};

// Not sure why I even use this...
template <typename Idx>
struct RefIdxType { 
  using type = Idx;
};

template <>
struct RefIdxType<builder::builder> {
  using type = builder::dyn_var<loop_type>;
};

template <>
struct RefIdxType<builder::dyn_var<loop_type>> {
  using type = builder::dyn_var<loop_type>;
};

}
