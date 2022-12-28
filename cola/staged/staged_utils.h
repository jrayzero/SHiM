// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "common/loop_type.h"
#include "fwrappers.h"
#include "fwddecls.h"
#include "traits.h"

namespace cola {

///
/// Fill a Loc_T object with template values
template <int Idx, typename D, loop_type Val, loop_type...Vals>
void to_Loc_T(D &dyn) {
  dyn[Idx] = Val;
  if constexpr (sizeof...(Vals) > 0) {
    to_Loc_T<Idx+1,D,Vals...>(dyn);
  }
}

///
/// Create a Loc_T object from template values
template <loop_type...Vals>
Loc_T<sizeof...(Vals)> to_Loc_T() {
  builder::dyn_var<loop_type[sizeof...(Vals)]> loc_t;
  to_Loc_T<0,decltype(loc_t),Vals...>(loc_t);
  return loc_t;
}

///
/// Make a homogeneous N-tuple containing Val
template <typename T, T Val, int N>
auto make_tup() {
  if constexpr (N == 0) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{Val}, make_tup<T, Val, N-1>());
  }
}

///
/// Convert a coordinate to a linear index 
template <int Depth, int Rank, typename...TupleTypes>
static builder::dyn_var<loop_type> linearize(Loc_T<Rank> extents, const std::tuple<TupleTypes...> &coord) {
  builder::dyn_var<loop_type> c = std::get<Rank-1-Depth>(coord);
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + extents[Rank-1-Depth] * linearize<Depth+1,Rank>(extents, coord);
  }
}

///
/// Perform a reduction across a region of obj
template <typename Functor, int Begin, int End, int Depth, typename Obj>
auto reduce_region_inner(Obj obj) {
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
template <typename Functor, int Begin, int End, int Rank, typename Obj>
auto reduce_region(Obj obj) {
  static_assert(Begin < Rank && End <= Rank && Begin < End);
  return reduce_region_inner<Functor,Begin,End,0>(obj);
}

///
/// Perform a reduction across a dyn_var<T[]>
template <typename Functor, int Rank, typename T>
builder::dyn_var<T> reduce(builder::dyn_var<T[Rank]> arr) {
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
template <int Depth, int Rank, typename LIdx, typename Extents>
auto delinearize(LIdx lidx, const Extents &extents) {
  if constexpr (Depth+1 == Rank) {
    return std::tuple{lidx};
  } else {
    builder::dyn_var<loop_type> m = reduce_region<MulFunctor, Depth+1, Rank, Rank>(extents);
    builder::dyn_var<loop_type> c = lidx / m;
    return std::tuple_cat(std::tuple{c}, delinearize<Depth+1, Rank>(lidx % m, extents));
  }
}

///
/// Apply Functor to the elements in arr0 and arr1 and store the result in arr
template <typename Functor, int Rank, int Depth>
void apply(Loc_T<Rank> arr,
	   Loc_T<Rank> arr0, 
	   Loc_T<Rank> arr1) {
  auto applied = Functor()(arr0[Depth], arr1[Depth]);
  arr[Depth] = applied;
  if constexpr (Depth < Rank-1) {
    apply<Functor,Rank,Depth+1>(arr,arr0, arr1);
  }
}

///
/// Apply Functor to the elements in arr0 and arr1 and produce the resulting arr
template <typename Functor, int Rank>
Loc_T<Rank> apply(Loc_T<Rank> arr0, 
		  Loc_T<Rank> arr1) {
  Loc_T<Rank> arr;
  apply<Functor, Rank, 0>(arr, arr0, arr1);
  return arr;
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

// Create a new Loc_T and copy over the contents of obj
template <int Rank>
Loc_T<Rank> deepcopy(Loc_T<Rank> obj) {
  Loc_T<Rank> copy;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    copy[i] = obj[i];
  }
  return copy;
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
