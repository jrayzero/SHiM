// -*-c++-*-

#pragma once

namespace hmda {

template <typename T>
struct is_expr {
  static constexpr bool value = false;
};

template <typename BlockLike, typename Idxs>
struct is_expr<Ref<BlockLike,Idxs>> {
  static constexpr bool value = true;
};

template <typename Functor, typename Operand>
struct is_expr<Unary<Functor,Operand>> {
  static constexpr bool value = true;
};

template <typename Functor, typename Operand0, typename Operand1>
struct is_expr<Binary<Functor,Operand0,Operand1>> {
  static constexpr bool value = true;
};

template <char Ident>
struct is_expr<Iter<Ident>> {
  static constexpr bool value = true;
};

template <typename To, typename Operand>
struct is_expr<TemplateCast<To,Operand>> {
  static constexpr bool value = true;
};

template <typename T>
struct is_dyn_var { 
  static constexpr bool value = false;
};

template <typename T>
struct is_dyn_var<builder::dyn_var<T>> { 
  static constexpr bool value = true;
};

template <typename T>
struct is_builder { 
  static constexpr bool value = false;
};

template <>
struct is_builder<builder::builder> { 
  static constexpr bool value = true;
};

template <typename T>
struct is_dyn_like {
  static constexpr bool value = is_dyn_var<T>::value || is_builder<T>::value;
};

template <typename I>
struct is_iter {
  static constexpr bool value = false;
};

template <char Ident>
struct is_iter<Iter<Ident>> {
  static constexpr bool value = true;
};

template <typename T>
struct is_slice { 
  static constexpr bool value = false;
};

template <>
struct is_slice<Slice> { 
  static constexpr bool value = true;
};

}
