// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "common/loop_type.h"
#include "fwrappers.h"
#include "fwddecls.h"
#include "traits.h"

namespace hmda {

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
auto reduce_region(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  if constexpr (Depth >= Begin && Depth < End) {
    builder::dyn_var<loop_type> item = obj[Depth];
    if constexpr (Depth < End - 1) {
      return Functor()(item, reduce_region<Functor,Begin,End,Depth+1>(obj));
    } else {
      return item;
    }
  } else {
    // won't be hit if Depth >= End
    return reduce_region<Functor,Begin,End,Depth+1>(obj);
  }
}

///
/// Perform a reduction across a region of obj
template <typename Functor, int Begin, int End, typename Obj>
auto reduce_region(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  static_assert(Begin < tuple_sz && End <= tuple_sz && Begin < End);
  static_assert(tuple_sz > 0);
  return reduce_region<Functor,Begin,End,0>(obj);
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
template <int Depth, typename LIdx, typename Extents>
auto delinearize(LIdx lidx, const Extents &extents) {
  constexpr int tuple_size = std::tuple_size<Extents>();
  if constexpr (Depth+1 == tuple_size) {
    return std::tuple{lidx};
  } else {
    builder::dyn_var<loop_type> m = reduce_region<MulFunctor, Depth+1, tuple_size>(extents);
    builder::dyn_var<loop_type> c = lidx / m;
    return std::tuple_cat(std::tuple{c}, delinearize<Depth+1>(lidx % m, extents));
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

}
