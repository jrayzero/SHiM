#pragma once

#include <array>
#include <sstream>

namespace hmda {

using loop_type = int32_t;

// A shared structure between staged and unstaged blocks as well as views
template <typename Elem, int Rank>
struct BaseBlockLike {
  static const int rank = Rank;
  using elem = Elem;

  // The extents in each dimension
  std::array<loop_type,Rank> bextents;
  // The strides in each dimension
  std::array<loop_type,Rank> bstrides;
  // The origin 
  std::array<loop_type,Rank> borigin;

  // The extents to use for things such as determining loop sizes, linearizing, etc
  std::array<loop_type,Rank> primary_extents;

  BaseBlockLike(const std::array<loop_type,Rank> &bextents,
		const std::array<loop_type,Rank> &bstrides,
		const std::array<loop_type,Rank> &borigin,
		const std::array<loop_type,Rank> &primary_extents) : 
  bextents(bextents), bstrides(bstrides), borigin(borigin), primary_extents(primary_extents) { }

protected:

  // Convert a coordinate into a linear index using the primary_extents
  template <int Depth, typename...TupleTypes>
    auto linearize(const std::tuple<TupleTypes...> &coord) {
    auto c = std::get<Rank-1-Depth>(coord);
    if constexpr (Depth == Rank - 1) {
	return c;
      } else {
      return c + std::get<Rank-1-Depth>(primary_extents) * linearize<Depth+1>(coord);
    }
  }

  // Convert a coordinate into a linear index using a specified set of extents
  template <int Depth, typename...TupleTypes>
    static auto linearize(const std::array<loop_type,Rank> &extents, const std::tuple<TupleTypes...> &coord) {
    auto c = std::get<Rank-1-Depth>(coord);
    if constexpr (Depth == Rank - 1) {
	return c;
      } else {
      return c + std::get<Rank-1-Depth>(extents) * linearize<Depth+1>(extents, coord);
    }
  }


};

// Unspecialized version of Block. It gets
// specialized for staged/unstaged
template <typename Elem, int Rank, bool Staged>
struct Block : public BaseBlockLike<Elem, Rank> { };

template <typename T, T Val, int Rank>
std::array<T,Rank> make_array() {
  std::array<T, Rank> arr;
  for (int i = 0; i < Rank; i++) arr[i] = Val;
  return arr;
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

}
