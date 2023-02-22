// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "fwddecls.h"

namespace shim {

template <typename T>
struct is_view {
  static constexpr bool value = false;
};

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
struct is_view<View<Elem,Rank,MultiDimPtr>> {
  static constexpr bool value = true;
};

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

template <typename Cond, typename TBranch, typename FBranch>
struct is_expr<TernaryCond<Cond,TBranch,FBranch>> {
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
struct is_expr_or_field {
  static constexpr bool value = false;
};

template <typename BlockLike, typename Idxs>
struct is_expr_or_field<Ref<BlockLike,Idxs>> {
  static constexpr bool value = true;
};

template <typename Functor, typename Operand>
struct is_expr_or_field<Unary<Functor,Operand>> {
  static constexpr bool value = true;
};

template <typename Functor, typename Operand0, typename Operand1>
struct is_expr_or_field<Binary<Functor,Operand0,Operand1>> {
  static constexpr bool value = true;
};

template <char Ident>
struct is_expr_or_field<Iter<Ident>> {
  static constexpr bool value = true;
};

template <typename To, typename Operand>
struct is_expr_or_field<TemplateCast<To,Operand>> {
  static constexpr bool value = true;
};

template <typename Elem, typename IsPrimitive>
struct is_expr_or_field<SField<Elem,IsPrimitive>> {
  static constexpr bool value = true;
};

template <typename S>
struct is_sfield {
  static constexpr bool value = false;
};

template <typename Elem, typename IsPrimitive>
struct is_sfield<SField<Elem,IsPrimitive>> {
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
struct is_range { 
  static constexpr bool value = false;
};

template <bool B>
struct is_range<Range<B>> { 
  static constexpr bool value = true;
};

template <typename T>
struct is_stop_range {
  static constexpr bool value = false;
};

template <>
struct is_stop_range<Range<true>> {
  static constexpr bool value = true;
};

template <typename T>
struct is_hmda_fundamental {
  static constexpr bool value = false;
};

template <>
struct is_hmda_fundamental<uint8_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<uint16_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<uint32_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<uint64_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<char> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<int8_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<int16_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<int32_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<int64_t> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<float> {
  static constexpr bool value = true;
};

template <>
struct is_hmda_fundamental<double> {
  static constexpr bool value = true;
};

template <typename T>
struct is_grouped_junction_operator {
  static constexpr bool value = false;
};

template <int GOPType, typename...Options>
struct is_grouped_junction_operator<GroupedJunctionOperator<GOPType, Options...>> {
  static constexpr bool value = true;
};

}
