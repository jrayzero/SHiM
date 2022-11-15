#pragma once

#include <iostream>
#include <sstream>
#include "fwd.h"

namespace hmda {

struct HMDAError {
  static std::stringstream ss;
};

#define SS HMDAError::ss

#define HMDA_ASSERT(cond, msg) \
  if (!cond) {								\
    msg;								\
    std::cerr << "[HMDA Assert Fail] " << HMDAError::ss.str() << std::endl; \
    HMDAError::ss.str("");						\
    exit(-1);								\
  }

#define HMDA_FAIL(msg) \
  msg;									\
  std::cerr << "[HMDA Fail] " << HMDAError::ss.str() << std::endl;	\
  HMDAError::ss.str("");						\
  exit(-1);								

template <char Ident, int Depth, typename LhsIdx, typename...LhsIdxs>
struct FindIter { 
  template <typename...Iterations>
  int operator()(const std::tuple<Iterations...> &iters) const {
    if constexpr (sizeof...(LhsIdxs) == Depth) {
      HMDA_FAIL(SS << "Could not find LHS index for RHS Iter " << Ident << ".");
      return -1; // stub
    } else {
      return FindIter<Ident, Depth+1, LhsIdxs...>()(iters);
    }
  }
};

template <char Ident, int Depth, typename...LhsIdxs>
struct FindIter<Ident, Depth, Iter<Ident>, LhsIdxs...> {
  template <typename...Iterations>
  int operator()(const std::tuple<Iterations...> &iters) const { 
    return std::get<Depth>(iters);
  }
};

template <char Ident, typename...Iterations, typename...LhsIdxs>
int find_iter(const std::tuple<Iterations...> &iters, const std::tuple<LhsIdxs...> &lhs_idxs) {
  return FindIter<Ident, 0, LhsIdxs...>()(iters);
}

// make a constant value tuple
template <int Value, int N>
auto make_tuple() {
  if constexpr (N == 0) {
    return std::tuple<>{};
  } else {
    return std::tuple_cat(std::tuple<int>{Value}, make_tuple<Value, N-1>());
  }
}

// apply a reduction across all the elements in a tuple
template <typename Functor, int Depth, typename...TupleTypes>
auto reduce(const std::tuple<TupleTypes...> &tup) {
  auto elem = std::get<Depth>(tup);
  if constexpr (Depth < sizeof...(TupleTypes) - 1) {
      return Functor()(elem, reduce<Functor, Depth+1>(tup));
  } else {
    return elem;
  }
}

// apply a reduction across the elements within a specific range of a tuple
template <typename Functor, int Start, int Stop, int Depth, typename...TupleTypes>
auto reduce_region(const std::tuple<TupleTypes...> &tup) {
  static_assert(Start < Stop);
  static_assert(Stop <= sizeof...(TupleTypes));
  if constexpr (Depth >= Start && Depth < Stop) {
    auto elem = std::get<Depth>(tup);    
    if constexpr (Depth < Stop - 1) {
      return Functor()(elem, reduce_region<Functor, Start, Stop, Depth+1>(tup));      
    } else {
      return elem;
    }
  } else {
    return reduce_region<Functor, Start, Stop, Depth + 1>(tup);
  }
}

// convert a coordinate into a linear index
template <int Depth, typename...ExtentTypes, typename...CoordTypes>
auto linearize(const std::tuple<ExtentTypes...> &extents, const std::tuple<CoordTypes...> &coord) {
  static_assert(sizeof...(ExtentTypes) == sizeof...(CoordTypes));
  auto c = std::get<Depth>(coord);
  if constexpr (Depth == sizeof...(ExtentTypes) - 1) {
    return c;
  } else {
    return c * reduce_region<std::multiplies<int>, Depth+1, sizeof...(ExtentTypes), 0>(extents) + 
      linearize<Depth+1>(extents, coord);
  }
}


}
