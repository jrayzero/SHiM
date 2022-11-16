#pragma once

namespace hmda {

  struct AddFunctor {

    static constexpr char op[] = "+";
    static constexpr bool is_func_like = false;

    template<typename Lhs, typename Rhs>
    auto operator()(const Lhs &lhs, const Rhs &rhs) {
      return lhs + rhs;
    }

  };

  struct MulFunctor {

    static constexpr char op[] = "*";
    static constexpr bool is_func_like = false;

    template<typename Lhs, typename Rhs>
    auto operator()(const Lhs &lhs, const Rhs &rhs) {
      return lhs * rhs;
    }

  };

}