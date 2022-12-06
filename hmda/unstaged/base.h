#pragma once

#include <array>
#include <sstream>
#include <cmath>

namespace hmda {

using loop_type = int32_t;

// For staged versions, things like extents and what not can include dyn_vars which is
// why we need a tuple for it instead of a homogeneous structure like an array

// A shared structure between staged and unstaged blocks as well as views
template <typename Elem, int Rank>
struct BaseBlockLike {
  static const int rank = Rank;
  using elem = Elem;
  using arr_type = std::array<loop_type,Rank>;
  
  // The extents in each dimension
  arr_type bextents;
  // The strides in each dimension
  arr_type bstrides;
  // The origin 
  arr_type borigin;
  
  BaseBlockLike(const arr_type &bextents,
		const arr_type &bstrides,
		const arr_type &borigin) :
    bextents(bextents), bstrides(bstrides), borigin(borigin) { }
  
protected:
  
  // Convert a coordinate into a linear index using the loop_extents
/*  template <int Depth, typename...TupleTypes>
    auto linearize(const std::tuple<TupleTypes...> &coord) {
    auto c = std::get<Rank-1-Depth>(coord);
    if constexpr (Depth == Rank - 1) {
	return c;
      } else {
      return c + std::get<Rank-1-Depth>(bextents) * linearize<Depth+1>(coord);
    }
  }*/

  // Convert a coordinate into a linear index using a specified set of extents
  // Extents needs to be something accessible with std::get<int>
  template <int Depth, typename Extents, typename...TupleTypes>
    static auto linearize(const Extents &extents, const std::tuple<TupleTypes...> &coord) {
    auto c = std::get<Rank-1-Depth>(coord);
    if constexpr (Depth == Rank - 1) {
	return c;
      } else {
      return c + std::get<Rank-1-Depth>(extents) * linearize<Depth+1>(extents, coord);
    }
  }

};

// Base for View that includes the additional parameters needed
template <typename Elem, int Rank>
struct BaseView : public BaseBlockLike<Elem, Rank> {

  using arr_type = std::array<loop_type,Rank>;
  
  BaseView(const arr_type &bextents,
	   const arr_type &bstrides,
	   const arr_type &borigin,
	   const arr_type &vextents,
	   const arr_type &vstrides,
	   const arr_type &vorigin) :
    BaseBlockLike<Elem, Rank>(bextents, bstrides, borigin),
    vextents(vextents), vstrides(vstrides), vorigin(vorigin) { }
  
  // The extents in each dimension
  arr_type vextents;
  // The strides in each dimension
  arr_type vstrides;
  // The origin 
  arr_type vorigin;

protected:

  // Convert view-relative iters into block-relative onees
  template <int Depth, typename...Iters>
  auto compute_block_relative_iters(const std::tuple<Iters...> &viters) const {
    if constexpr (Depth == sizeof...(Iters)) {
      return std::tuple{};
    } else {
      return std::tuple_cat(std::tuple{std::get<Depth>(viters) * std::get<Depth>(this->vstrides) +
	    std::get<Depth>(this->vorigin)}, compute_block_relative_iters<Depth+1>(viters));
    }
  }
  
};

template <typename T, T Val, int Rank>
std::array<T,Rank> make_array() {
  std::array<T, Rank> arr;
  for (int i = 0; i < Rank; i++) arr[i] = Val;
  return arr;
}

template <typename T, T Val, int N>
auto make_tup() {
  if constexpr (N == 0) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{Val}, make_tup<T, Val, N-1>());
  }
}


template <typename Elem, int Rank>
struct Block;

template <typename Elem, int Rank>
struct SBlock;

template <typename MaybeBlock>
struct IsAnyBlock {
  constexpr bool operator()() { return false; }
};

template <typename Elem, int Rank>
struct IsAnyBlock<SBlock<Elem,Rank>> {
  constexpr bool operator()() { return true; }
};

template <typename Elem, int Rank>
struct IsAnyBlock<Block<Elem,Rank>> {
  constexpr bool operator()() { return true; }
};

template <typename MaybeBlock>
constexpr bool is_any_block() {
  return IsAnyBlock<MaybeBlock>()();
}

template <typename Iterable>
std::string join(const Iterable &it, std::string joiner=",") {
  std::stringstream ss;
  bool first = true;
  for (auto i : it) {
    if (!first) ss << joiner;
    ss << i;
    first = false;
  }
  return ss.str();
}

template <int Depth, typename...TupleTypes>
void join_tup(std::stringstream &ss, const std::tuple<TupleTypes...> &tup, std::string joiner) {
  if constexpr (Depth < sizeof...(TupleTypes)) {
    if constexpr (Depth > 0) ss << joiner;
    ss << std::get<Depth>(tup);
    join_tup<Depth+1>(ss, tup, joiner);
  }
}

template <typename...TupleTypes>
std::string join_tup(const std::tuple<TupleTypes...> &tup, std::string joiner=",") {
  std::stringstream ss;
  join_tup<0>(ss, tup, joiner);
  return ss.str();
}

// use this for creating views via slicing (build via the "slice" function for easier
// usage). Since it's templated, can support dyn_var or primitive types
// specify that the Stop is the end of the block like
struct End { }; 
template <typename Start, typename Stop, typename Stride>
struct Slice {
  Start start;
  Stop stop;
  Stride stride;
  Slice(Start start, Stop stop, Stride stride) : start(start),
						 stop(stop),
						 stride(stride) { }
};

template <typename Start, typename Stop, typename Stride>
auto slice(Start start, Stop stop, Stride stride) {
  return Slice(start, stop, stride);
}

template <typename MaybeSlice>
struct IsSlice { constexpr bool operator()() { return false; } };

template <typename A, typename B, typename C>
struct IsSlice<Slice<A,B,C>> { constexpr bool operator()() { return true; } };

template <typename MaybeSlice>
constexpr bool is_slice() {
  return IsSlice<MaybeSlice>()();
}

// functions to extract the start, stop, stride from a slice
// either return a tuple or an array

// array
template <int Idx, int Rank, typename Arg, typename...Args>
void gather_origin(std::array<loop_type, Rank> &arr, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the Astart
    auto start = arg.start;
    static_assert(!std::is_same<End, decltype(start)>::value, "End only valid for the Stop parameter of Slice");
    arr[Idx] = start;
    if constexpr (sizeof...(Args) > 0) {
      gather_origin<Idx+1,Rank>(arr, args...);
    }    
  } else {
    // the start is just the value
    arr[Idx] = arg;//start;
    if constexpr (sizeof...(Args) > 0) {
      gather_origin<Idx+1,Rank>(arr, args...);
    }    
  }
}

// array
template <int Idx, int Rank, typename Arg, typename...Args>
void gather_strides(std::array<loop_type,Rank> &arr, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the stride
    auto stride = arg.stride;
    static_assert(!std::is_same<End, decltype(stride)>::value, "End only valid for the Stop parameter of Slice");
    arr[Idx] = stride;
    if constexpr (sizeof...(Args) > 0) {
      gather_strides<Idx+1,Rank>(arr, args...);
    }     
  } else {
    // the stride is just one
    auto stride = std::tuple{(loop_type)1};
    arr[Idx] = 1;//stride;
    if constexpr (sizeof...(Args) > 0) {
      gather_strides<Idx+1,Rank>(arr, args...);
    }    
  }
}

// array
template <int Idx, int Rank, typename BlockLikeExtents, typename Arg, typename...Args>
void gather_stops(std::array<loop_type,Rank> &arr, BlockLikeExtents extents, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the stop
    auto stop = arg.stop;
    if constexpr (std::is_same<End, decltype(stop)>::value) {
      arr[Idx] = std::get<Idx>(extents);
      if constexpr (sizeof...(Args) > 0) {
	gather_stops<Idx+1,Rank>(arr, extents, args...);
      }      
    } else {
      arr[Idx] = stop;
      if constexpr (sizeof...(Args) > 0) {
	gather_stops<Idx+1,Rank>(arr, extents, args...);
      }
    }
  } else {
    // the stop is just the value
    arr[Idx] = arg.stop;
    if constexpr (sizeof...(Args) > 0) {
      gather_stops<Idx+1,Rank>(arr, extents, args...);
    }    
  }
}

// array
template <int Depth, int Rank, typename Starts, typename Stops, typename Strides>
void convert_stops_to_extents(std::array<loop_type, Rank> &arr, Starts starts, Stops stops, Strides strides) {
  if constexpr (Depth < Rank) {
    auto extent = floor((std::get<Depth>(stops) - std::get<Depth>(starts) - (loop_type)1) / std::get<Depth>(strides)) + (loop_type)1;
    arr[Depth] = extent;
    convert_stops_to_extents<Depth+1,Rank>(arr, starts, stops, strides);
  }
}

template <int Rank, typename Starts, typename Stops, typename Strides>
void convert_stops_to_extents(std::array<loop_type,Rank> &arr, Starts starts, Stops stops, Strides strides) {
  convert_stops_to_extents<0,Rank>(arr, starts, stops, strides);
}

// perform a reduction across something that works with std::get
template <typename Functor, int Depth, typename Obj>
auto reduce(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  if constexpr (Depth < tuple_sz - 1) {
    return Functor()(std::get<Depth>(obj), reduce<Functor,Depth+1>(obj));
  } else if constexpr (Depth == tuple_sz - 1) {
    return std::get<Depth>(obj);
  }
}

template <typename Functor, typename Obj>
auto reduce(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  static_assert(tuple_sz > 0);
  if constexpr (tuple_sz == 1) {
    return std::get<0>(obj);
  } else {
    return reduce<Functor,0>(obj);
  }
}

// perform a reduction across a range of something that works with std::get 
template <typename Functor, int Begin, int End, int Depth, typename Obj>
auto reduce_region(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  if constexpr (Depth >= Begin && Depth < End) {
    auto item = std::get<Depth>(obj);
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

}
