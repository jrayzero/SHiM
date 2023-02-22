// -*-c++-*-

#pragma once

#include "common/loop_type.h"
#include "fwddecls.h"
#include "fwrappers.h"
#include "defs.h"

namespace shim {

// A default slice parameter value
struct Def { };

///
/// Represents a region to access within a block or a view
template <bool Stop> // if Stop = true, then need to infer this value as the extent when use
struct Range {

  Range(dvar<loop_type> start, 
	dvar<loop_type> stop, 
	dvar<loop_type> stride) : params({start,stop,stride}) { }

  dvar<loop_type> operator[](loop_type idx) { return params[idx]; }

  darr<loop_type,3> params;

};

///
/// Create a range with only default values
auto range() {
  return Range<false>(0,0,1);
}

/// 
/// Create a range with only a start value specified
template <typename Start>
auto range(Start start) {    
  if constexpr (std::is_same<Start,Def>::value) {
    return Range<false>(0,1,1);
  } else {
    return Range<false>(start,start+1,1);
  }
}

///
/// Create a range with start and stop specified
template <typename Start, typename Stop>
auto range(Start start, Stop stop) {
  if constexpr (std::is_same<Start,Def>::value && std::is_same<Stop,Def>::value) {
    return Range<true>(0,0,1);
  } else if constexpr (std::is_same<Start,Def>::value && !std::is_same<Stop,Def>::value) {
    return Range<false>(0,stop,1);
  } else if constexpr (!std::is_same<Start,Def>::value && std::is_same<Stop,Def>::value) {
    return Range<true>(start,0,1);
  } else { // (!std::is_same<Start,Def>::value && !std::is_same<Stop,Def>::value)
    return Range<false>(start,stop,1);
  }
}

///
/// Create a range with all values specified
template <typename Start, typename Stop, typename Stride>
auto range(Start start, Stop stop, Stride stride) {
  if constexpr (std::is_same<Start,Def>::value && std::is_same<Stop,Def>::value && std::is_same<Stride,Def>::value) {
    return Range<true>(0,0,1);
  } else if constexpr (std::is_same<Start,Def>::value && std::is_same<Stop,Def>::value && !std::is_same<Stride,Def>::value) {
    return Range<true>(0,0,stride);
  } else if constexpr (std::is_same<Start,Def>::value && !std::is_same<Stop,Def>::value && std::is_same<Stride,Def>::value) {
    return Range<false>(0,stop,1);
  } else if constexpr (std::is_same<Start,Def>::value && !std::is_same<Stop,Def>::value && !std::is_same<Stride,Def>::value) {
    return Range<false>(0,stop,stride);
  } else if constexpr (!std::is_same<Start,Def>::value && std::is_same<Stop,Def>::value && std::is_same<Stride,Def>::value) {
    return Range<true>(start,0,1);
  } else if constexpr (!std::is_same<Start,Def>::value && std::is_same<Stop,Def>::value && !std::is_same<Stride,Def>::value) {
    return Range<true>(start,0,stride);
  } else if constexpr (!std::is_same<Start,Def>::value && !std::is_same<Stop,Def>::value && std::is_same<Stride,Def>::value) {
    return Range<false>(start,stop,1);
  } else { // (!std::is_same<Start,Def>::value && !std::is_same<Stop,Def>::value && !std::is_same<Stride,Def>::value)
    return Range<false>(start,stop,stride);
  }
}

///
/// Combine start params from a view range into one array
template <int Idx, int Rank, typename Arg, typename...Args>
void gather_origin(Loc_T<Rank> &origin, Arg arg, Args...args) {
  if constexpr (is_range<Arg>::value) {
    origin[Idx] = arg[0];
    if constexpr (Idx < Rank - 1) {
      gather_origin<Idx+1,Rank>(origin,args...);
    }
  } else {
    // the start is just the value
    origin[Idx] = arg;
    if constexpr (Idx < Rank - 1) {
      gather_origin<Idx+1,Rank>(origin,args...);
    }
  }
}

///
/// Combine stop params from a view range into one array
template <int Idx, int Rank, typename Arg, typename...Args>
void gather_stops(Loc_T<Rank> &vec, const Loc_T<Rank> &extents, Arg arg, Args...args) {
  if constexpr (is_range<Arg>::value) {
      // specified as range(a,b,c)
      if constexpr (is_stop_range<Arg>::value) {
	  // we need to infer this
	vec[Idx] = extents[Idx];
        if constexpr (Idx < Rank - 1) {
	  gather_stops<Idx+1,Rank>(vec, extents, args...);
        }	
      } else {
	// use the value as is
	vec[Idx] = arg[1];
	if constexpr (Idx < Rank - 1) {
	  gather_stops<Idx+1,Rank>(vec, extents, args...);
	}
      }
  } else {
    // only a single value specified, meaning the stop is value+1
    vec[Idx] = arg + 1;
    if constexpr (Idx < Rank - 1) {
      gather_stops<Idx+1,Rank>(vec, extents, args...);
    }
  }
}

///
/// Combine stride params from a view range into one array
template <int Idx, int Rank, typename Arg, typename...Args>
void gather_strides(Loc_T<Rank> &stride, Arg arg, Args...args) {
  if constexpr (is_range<Arg>::value) {
    stride[Idx] = arg[2];
    if constexpr (Idx < Rank - 1) {
      gather_strides<Idx+1,Rank>(stride,args...);
    }
  } else {
    // the start is just the value
    stride[Idx] = arg;
    if constexpr (Idx < Rank - 1) {
      gather_strides<Idx+1,Rank>(stride,args...);
    }
  }
}

///
/// Convert the stop parameter from a range into an extent
template <int Depth, int Rank>
void convert_stops_to_extents(Loc_T<Rank> &arr, 
			      const Loc_T<Rank> &starts, 
			      const Loc_T<Rank> &stops, 
			      const Loc_T<Rank> &strides) {
#ifndef UNSTAGED
  dvar<loop_type> extent = hfloor((stops[Depth] - starts[Depth] - (loop_type)1) / 
				  strides[Depth]) + (loop_type)1;
#else
  dvar<loop_type> extent = floor((stops[Depth] - starts[Depth] - (loop_type)1) / 
				 strides[Depth]) + (loop_type)1;
#endif
  arr[Depth] = extent;
  if constexpr (Depth < Rank - 1) {
    convert_stops_to_extents<Depth+1,Rank>(arr, starts, stops, strides);
  }
}

///
/// Convert the stop parameter from a range into an extent
template <int Rank>
void convert_stops_to_extents(Loc_T<Rank> &arr,
			      const Loc_T<Rank> &starts, 
			      const Loc_T<Rank> &stops, 
			      const Loc_T<Rank> &strides) {
  convert_stops_to_extents<0, Rank>(arr, starts, stops, strides);
}

}

