#pragma once

#include "blocks/c_code_generator.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"

namespace hmda {

using loop_type = int32_t;

template <int Rank>
using Loc_T = builder::dyn_var<loop_type[Rank]>;

builder::dyn_var<int(int)> floor_func = builder::as_global("floor");
builder::dyn_var<int(int)> arr_size_func = builder::as_global("compute_arr_size");
builder::dyn_var<void*(int)> malloc_func = builder::as_global("malloc");
builder::dyn_var<void(void*,int,int)> memset_func = builder::as_global("memset");
builder::dyn_var<void(void*,int,int)> memset_heaparr_func = builder::as_global("memset_heaparr");
builder::dyn_var<void(void*,void*,int)> memcpy_func = builder::as_global("memcpy");

template <typename T, T Val, int N>
auto make_tup() {
  if constexpr (N == 0) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{Val}, make_tup<T, Val, N-1>());
  }
}

template <int Depth, int Rank, typename...TupleTypes>
static builder::dyn_var<loop_type> linearize(Loc_T<Rank> extents, const std::tuple<TupleTypes...> &coord) {
  builder::dyn_var<loop_type> c = std::get<Rank-1-Depth>(coord);
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + extents[Rank-1-Depth] * linearize<Depth+1,Rank>(extents, coord);
  }
}

// perform a reduction across a range of something that works with std::get 
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

template <typename Functor, int Begin, int End, typename Obj>
auto reduce_region(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  static_assert(Begin < tuple_sz && End <= tuple_sz && Begin < End);
  static_assert(tuple_sz > 0);
  return reduce_region<Functor,Begin,End,0>(obj);
}

template <typename Functor, int Rank, typename T>
builder::dyn_var<T> reduce(builder::dyn_var<T[Rank]> arr) {
  builder::dyn_var<T> acc = arr[0];
  for (builder::static_var<T> i = 1; i < Rank; i=i+1) {
    acc = Functor()(acc, arr[i]);
  }
  return acc;
}

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

template <int Rank, int Depth, typename...Iters>
auto compute_block_relative_iters(Loc_T<Rank> vstrides, 
				  Loc_T<Rank> vorigin,
				  const std::tuple<Iters...> &viters) {
  if constexpr (Depth == sizeof...(Iters)) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{std::get<Depth>(viters) * vstrides[Depth] +
	  vorigin[Depth]}, 
      compute_block_relative_iters<Rank,Depth+1>(vstrides, vorigin, viters));
  }
}

struct End { }; 

struct Slice {
  builder::dyn_var<loop_type> start;
  builder::dyn_var<loop_type> stop;
  builder::dyn_var<loop_type> stride;
  Slice(builder::dyn_var<loop_type> start, 
	builder::dyn_var<loop_type> stop, 
	builder::dyn_var<loop_type> stride) : start(start),
					      stop(stop),
					      stride(stride) { }
};

auto slice(builder::dyn_var<loop_type> start, builder::dyn_var<loop_type> stop, builder::dyn_var<loop_type> stride) {
  return Slice(start, stop, stride);
}

template <typename MaybeSlice>
struct IsSlice { constexpr bool operator()() { return false; } };

template <>
struct IsSlice<Slice> { constexpr bool operator()() { return true; } };

template <typename MaybeSlice>
constexpr bool is_slice() {
  return IsSlice<MaybeSlice>()();
}

template <typename Functor, int Rank, int Depth>
void apply(Loc_T<Rank> vec,
	   Loc_T<Rank> vec0, 
	   Loc_T<Rank> vec1) {
  auto applied = Functor()(vec0[Depth], vec1[Depth]);
  vec[Depth] = applied;
  if constexpr (Depth < Rank-1) {
    apply<Functor,Rank,Depth+1>(vec,vec0, vec1);
  }
}

template <typename Functor, int Rank>
Loc_T<Rank> apply(Loc_T<Rank> vec0, 
		  Loc_T<Rank> vec1) {
  Loc_T<Rank> vec;
  apply<Functor, Rank, 0>(vec, vec0, vec1);
  return vec;
}

template <int Idx, int Rank, typename Arg, typename...Args>
void gather_origin(Loc_T<Rank> vec, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    vec[Idx] = arg.start;
    if constexpr (Idx < Rank - 1) {
      gather_origin<Idx+1,Rank>(vec,args...);
    }
  } else {
    // the start is just the value
    vec[Idx] = arg;
    if constexpr (Idx < Rank - 1) {
      gather_origin<Idx+1,Rank>(vec,args...);
    }
  }
}


template <int Idx, int Rank, typename Arg, typename...Args>
void gather_strides(Loc_T<Rank> vec, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    vec[Idx] = arg.stride;
    if constexpr (Idx < Rank - 1) {
      gather_strides<Idx+1,Rank>(vec, args...);
    }
  } else {
    vec[Idx] = 1;
    if constexpr (Idx < Rank - 1) {
      gather_strides<Idx+1,Rank>(vec, args...);
    }
  }
}

template <int Idx, int Rank, typename Arg, typename...Args>
void gather_stops(Loc_T<Rank> vec, Loc_T<Rank> extents, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    builder::dyn_var<loop_type> stop = arg.stop;
    if constexpr (std::is_same<End, decltype(stop)>::value) {
      vec[Idx] = extents[Idx];
      if constexpr (Idx < Rank - 1) {
	gather_stops<Idx+1,Rank>(vec, extents, args...);
      }
    } else {
      vec[Idx] = stop;
      if constexpr (Idx < Rank - 1) {
	gather_stops<Idx+1,Rank>(vec, extents, args...);
      }
    }
  } else {
    vec[Idx] = arg;
    if constexpr (Idx < Rank - 1) {
      gather_stops<Idx+1,Rank>(vec, extents, args...);
    }    
  }
}

template <int Depth, int Rank>
void convert_stops_to_extents(Loc_T<Rank> arr, Loc_T<Rank> starts, Loc_T<Rank> stops, Loc_T<Rank> strides) {
  builder::dyn_var<loop_type> extent = floor_func((stops[Depth] - starts[Depth] - (loop_type)1) / 
						  strides[Depth]) + (loop_type)1;
  arr[Depth] = extent;
  if constexpr (Depth < Rank - 1) {
    convert_stops_to_extents<Depth+1,Rank>(arr, starts, stops, strides);
  }
}

template <int Rank>
Loc_T<Rank> convert_stops_to_extents(Loc_T<Rank> starts, Loc_T<Rank> stops, Loc_T<Rank> strides) {
  Loc_T<Rank> arr;
  convert_stops_to_extents<0, Rank>(arr, starts, stops, strides);
  return arr;
}

}
