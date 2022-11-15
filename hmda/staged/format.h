#pragma once

#include <sstream>

namespace hmda {

template <typename T>
struct TypeToStr { };

template <>
struct TypeToStr<bool> {
  std::string operator()() { return "bool"; }
};

template <>
struct TypeToStr<int32_t> {
  std::string operator()() { return "int32"; }
};


template <>
struct TypeToStr<int64_t> {
  std::string operator()() { return "int64"; }
};

template <>
struct TypeToStr<float> {
  std::string operator()() { return "float"; }
};

template <typename T>
std::string type_to_str() {
  return TypeToStr<T>()();
}

template <int Depth, typename...TupleTypes>
void tuple_to_str(std::stringstream &ss, const std::tuple<TupleTypes...> &tup) {
  if constexpr (Depth != sizeof...(TupleTypes)) {
    if constexpr (Depth > 0) ss << ",";
    ss << std::get<Depth>(tup);
    tuple_to_str<Depth+1>(ss, tup);
  }
}

// print a captured tuple as comma-separated values
template <typename...TupleTypes>
std::string tuple_to_str(const std::tuple<TupleTypes...> &tup) {
  std::stringstream ss;
  tuple_to_str<0>(ss, tup);
  return ss.str();
}

}
