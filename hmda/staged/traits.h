#pragma once

namespace hmda {

template <typename T>
struct IsTuple { 
  constexpr bool operator()() const { return false; } 
};

template <typename...Ts>
struct IsTuple<std::tuple<Ts...>> { 
  constexpr bool operator()() const { return true; } 
};

template <typename T>
constexpr bool is_tuple() { return IsTuple<T>()(); }

}
