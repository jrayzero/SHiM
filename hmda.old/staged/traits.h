#pragma once

#include "fwd.h"

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

namespace staged {

  template <typename T>
  struct IsIter {
    constexpr bool operator()() { return false; }
  };

  template <char Ident>
  struct IsIter<hmda::staged::Iter<Ident>> {
    constexpr bool operator()() { return true; }
  };

  template <typename T>
  constexpr bool is_iter() {
    return IsIter<T>()();
  }

}

}

