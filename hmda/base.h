#pragma once

#include <array>
#include <sstream>

namespace hmda {

using loop_type = int32_t;

// For staged versions, things like extents and what not can include dyn_vars which is
// why we need a tuple for it instead of a homogeneous structure like an array

// A shared structure between staged and unstaged blocks as well as views
template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
struct BaseBlockLike {
  static const int rank = Rank;
  using elem = Elem;

  // The extents in each dimension
  BExtents bextents;
  // The strides in each dimension
  BStrides bstrides;
  // The origin 
  BOrigin borigin;
  
  BaseBlockLike(const BExtents &bextents,
		const BStrides &bstrides,
		const BOrigin &borigin) :
    bextents(bextents), bstrides(bstrides), borigin(borigin) { }
  
protected:
  
  // Convert a coordinate into a linear index using the loop_extents
  template <int Depth, typename...TupleTypes>
    auto linearize(const std::tuple<TupleTypes...> &coord) {
    auto c = std::get<Rank-1-Depth>(coord);
    if constexpr (Depth == Rank - 1) {
	return c;
      } else {
      return c + std::get<Rank-1-Depth>(bextents) * linearize<Depth+1>(coord);
    }
  }

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
template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
struct BaseView : public BaseBlockLike<Elem, Rank, BExtents, BStrides, BOrigin> {

  BaseView(const BExtents &bextents,
	   const BStrides &bstrides,
	   const BOrigin &borigin,
	   const VExtents &vextents,
	   const VStrides &vstrides,
	   const VOrigin &vorigin) :
    BaseBlockLike<Elem, Rank, BExtents, BStrides, BOrigin>(bextents, bstrides, borigin),
    vextents(vextents), vstrides(vstrides), vorigin(vorigin) { }

  // The extents in each dimension
  VExtents vextents;
  // The strides in each dimension
  VStrides vstrides;
  // The origin 
  VOrigin vorigin;

protected:

  // Convert view-relative iters into block-relative onees
  template <int Depth, typename...Iters>
  auto compute_block_relative_iters(const std::tuple<Iters...> &viters) {
    if constexpr (Depth == sizeof...(Iters)) {
      return std::tuple{};
    } else {
      return std::tuple_cat(std::tuple{std::get<Depth>(viters) * std::get<Depth>(this->vstrides) +
	    std::get<Depth>(this->vorigin)},
	compute_block_relative_iters<Depth+1>(viters));
    }
  }
  
};

template <typename T, T Val, int Rank>
std::array<T,Rank> make_array() {
  std::array<T, Rank> arr;
  for (int i = 0; i < Rank; i++) arr[i] = Val;
  return arr;
}

template <typename Elem, int Rank>
struct Block;

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
struct SBlock;

template <typename MaybeBlock>
struct IsAnyBlock {
  constexpr bool operator()() { return false; }
};

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
struct IsAnyBlock<SBlock<Elem,Rank,BExtents,BStrides,BOrigin>> {
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

// tuple
template <typename Arg, typename...Args>
auto gather_origin(Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the Astart
    auto start = arg.start;
    static_assert(!std::is_same<End, decltype(start)>::value, "End only valid for the Stop parameter of Slice");
    if constexpr (sizeof...(Args) > 0) {
      return std::tuple_cat(std::tuple{start}, gather_origin(args...));
    } else {
      return std::tuple{start};
    }     
  } else {
    // this should be an arithmetic value or a dyn var
    // the start is just the value
    auto start = std::tuple{arg};
    if constexpr (sizeof...(Args) > 0) {
      return std::tuple_cat(start, gather_origin(args...));
    } else {
      return start;
    }    
  }
}

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
    // this should be an arithmetic value or a dyn var
    // the start is just the value
    auto start = std::tuple{arg};
    arr[Idx] = start;
    if constexpr (sizeof...(Args) > 0) {
      gather_origin<Idx+1,Rank>(arr, args...);
    }    
  }
}

// tuple
template <typename Arg, typename...Args>
auto gather_strides(Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the stride
    auto stride = arg.stride;
    static_assert(!std::is_same<End, decltype(stride)>::value, "End only valid for the Stop parameter of Slice");
    if constexpr (sizeof...(Args) > 0) {
      return std::tuple_cat(std::tuple{stride}, gather_strides(args...));
    } else {
      return std::tuple{stride};
    }     
  } else {
    // this should be an arithmetic value or a dyn var
    // the stride is just one
    auto stride = std::tuple{(loop_type)1};
    if constexpr (sizeof...(Args) > 0) {
      return std::tuple_cat(stride, gather_strides(args...));
    } else {
      return stride;
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
    // this should be an arithmetic value or a dyn var
    // the stride is just one
    auto stride = std::tuple{(loop_type)1};
    arr[Idx] = stride;
    if constexpr (sizeof...(Args) > 0) {
      gather_strides<Idx+1,Rank>(arr, args...);
    }    
  }
}

// tuple
template <int Idx, typename BlockLikeExtents, typename Arg, typename...Args>
auto gather_stops(BlockLikeExtents extents, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the stop
    auto stop = arg.stop;
    if constexpr (std::is_same<End, decltype(stop)>::value) {
      if constexpr (sizeof...(Args) > 0) {
	return std::tuple_cat(std::tuple{std::get<Idx>(extents)}, gather_stops<Idx+1>(extents, args...));
      } else {
	return std::tuple{std::get<Idx>(extents)};
      }      
    } else {
      if constexpr (sizeof...(Args) > 0) {
	return std::tuple_cat(std::tuple{stop}, gather_stops<Idx+1>(extents, args...));
      } else {
	return std::tuple{stop};
      }
    }
  } else {
    // this should be an arithmetic value or a dyn var
    // the stop is just the value
    auto stop = std::tuple{arg.stop};
    if constexpr (sizeof...(Args) > 0) {
      return std::tuple_cat(stop, gather_stops<Idx+1>(extents, args...));
    } else {
      return stop;
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
    // this should be an arithmetic value or a dyn var
    // the stop is just the value
    arr[Idx] = arg.stop;
    if constexpr (sizeof...(Args) > 0) {
      gather_stops<Idx+1,Rank>(arr, extents, args...);
    }    
  }
}

// tuple
template <int Depth, typename Starts, typename Stops, typename Strides>
auto convert_stops_to_extents(Starts starts, Stops stops, Strides strides) {
  static_assert(std::tuple_size<Starts>() == std::tuple_size<Stops>() && std::tuple_size<Stops>() == std::tuple_size<Strides>());
  if constexpr (Depth == std::tuple_size<Starts>()) {
    return std::tuple{};
  } else {
    // TODO does dyn_var support std math functions like floor? Because
    // that's what I need for this division
    auto extent = (std::get<Depth>(stops) - std::get<Depth>(starts) - (loop_type)1) / std::get<Depth>(strides) + (loop_type)1;
    return std::tuple_cat(std::tuple{extent}, convert_stops_to_extents<Depth+1>(starts, stops, strides));
  }
}

template <typename Starts, typename Stops, typename Strides>
auto convert_stops_to_extents(Starts starts, Stops stops, Strides strides) {
  return convert_stops_to_extents<0>(starts, stops, strides);
}

// array
template <int Depth, int Rank, typename Starts, typename Stops, typename Strides>
void convert_stops_to_extents(std::array<loop_type, Rank> &arr, Starts starts, Stops stops, Strides strides) {
  if constexpr (Depth < Rank) {
    // TODO does dyn_var support std math functions like floor? Because
    // that's what I need for this division
    auto extent = (std::get<Depth>(stops) - std::get<Depth>(starts) - (loop_type)1) / std::get<Depth>(strides) + (loop_type)1;
    arr[Depth] = extent;
    convert_stops_to_extents<Depth+1,Rank>(arr, starts, stops, strides);
  }
}

template <int Rank, typename Starts, typename Stops, typename Strides>
void convert_stops_to_extents(std::array<loop_type,Rank> &arr, Starts starts, Stops stops, Strides strides) {
  convert_stops_to_extents<0,Rank>(arr, starts, stops, strides);
}

}
