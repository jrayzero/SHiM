// -*-c++-*-

#pragma once

#include <type_traits>

namespace hmda {

template <typename T>
struct is_expr {
  static constexpr value = false;
};

template <typename BlockLike, typename Idxs>
struct is_expr<Ref<BlockLike,Idxs>> {
  static constexpr value = true;
};

template <typename Functor, typename Operand>
struct is_expr<Unary<Functor,Operand>> {
  static constexpr value = true;
};

template <typename Functor, typename Operand0, typename Operand1>
struct is_expr<Binary<Functor,Operand0,Operand1>> {
  static constexpr value = true;
};

template <char Ident>
struct is_expr<Iter<Ident>> {
  static constexpr value = true;
};

template <typename To, typename Operand>
struct is_expr<TemplateCast<To,Operand>> {
  static constexpr value = true;
};


}
