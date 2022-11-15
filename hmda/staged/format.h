#pragma once

#include <sstream>

#include "fwd.h"
#include "traits.h"

namespace hmda {

  namespace staged {

  template <typename T>
  struct FormatItem { 
    static void format(std::stringstream &ss, T t) { ss << t; }
  };

  template <char Ident>
  struct FormatItem<Iter<Ident>> { 
    static void format(std::stringstream &ss, Iter<Ident>) { ss << Ident; }
  };

    template<int Depth, typename...TupleTypes>
    void idx_tuple_to_str(std::stringstream &ss, const std::tuple<TupleTypes...> &tup) {
      if constexpr (Depth != sizeof...(TupleTypes)) {
        if constexpr (Depth > 0) ss << ",";
        auto x = std::get<Depth>(tup);
	FormatItem<decltype(x)>::format(ss, x);
        idx_tuple_to_str<Depth + 1>(ss, tup);
      }
    }

  // print a tuple as comma-separated values
    template<typename...TupleTypes>
    std::string idx_tuple_to_str(const std::tuple<TupleTypes...> &tup) {
      std::stringstream ss;
      idx_tuple_to_str<0>(ss, tup);
      return ss.str();
    }

  }

}
