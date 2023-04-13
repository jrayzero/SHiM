// -*-c++-*-

#pragma once

#include <type_traits>
#include "builder/dyn_var.h"
#include "builder/array.h"
#include "functors.hpp"
#include "fwrappers.hpp"
#include "fwddecls.h"
#include "traits.h"
#include "defs.h"
#include "staged_utils.h"

namespace shim {

///
/// A disjunction or conjunction like Or(a,b,c,...) or And(a,b,c,...) that can be used as part of a boolean expression such as
/// q == Or(a,b,c), which eventually expands to q == a || q == b || q == c. These are not Exprs by themselves,
/// thus they cannot be realized and instead should be expanded before realization
/// GOPType == 0 => or
/// GOPType == 1 => and
template <int GOPType, typename...Options>
struct GroupedJunctionOperator {
  static constexpr int GOPType_T = GOPType;
  explicit GroupedJunctionOperator(std::tuple<Options...> group) : group(std::move(group)) { 
    static_assert(sizeof...(Options) > 0);
  } 
  std::tuple<Options...> group;
  
};

///
/// Create an OR GroupedJunctionOperator
template <typename...Options>
GroupedJunctionOperator<0, Options...> Or(Options...options) {
  return GroupedJunctionOperator<0, Options...>{std::tuple{options...}};
}

///
/// Create an AND GroupedJunctionOperator
template <typename...Options>
GroupedJunctionOperator<1, Options...> And(Options...options) {
  return GroupedJunctionOperator<1, Options...>(std::tuple{options...});
}

///
/// Unspecialized template to determine the core type of a compound expression
template <typename T>
struct GetCoreT { using Core_T = T; };

///
/// Core type is bool
template <>
struct GetCoreT<bool> { using Core_T = bool; };

///
/// Core type is uint8_t
template <>
struct GetCoreT<uint8_t> { using Core_T = uint8_t; };

///
/// Core type is uint16_t
template <>
struct GetCoreT<uint16_t> { using Core_T = uint16_t; };

///
/// Core type is uint32_t
template <>
struct GetCoreT<uint32_t> { using Core_T = uint32_t; };

///
/// Core type is uint64_t
template <>
struct GetCoreT<uint64_t> { using Core_T = uint64_t; };

///
/// Core type is char
template <>
struct GetCoreT<char> { using Core_T = char; };

///
/// Core type is int8_t (signed char)
template <>
struct GetCoreT<int8_t> { using Core_T = int8_t; };

///
/// Core type is int16_t
template <>
struct GetCoreT<int16_t> { using Core_T = int16_t; };

///
/// Core type is int32_t
template <>
struct GetCoreT<int32_t> { using Core_T = int32_t; };

///
/// Core type is int64_t
template <>
struct GetCoreT<int64_t> { using Core_T = int64_t; };

///
/// Core type is float
template <>
struct GetCoreT<float> { using Core_T = float; };

///
/// Core type is double
template <>
struct GetCoreT<double> { using Core_T = double; };

///
/// Core type is Elem
template <typename Elem>
struct GetCoreT<builder::dyn_var<Elem>> { using Core_T = Elem; };

///
/// Core type is Elem
template <typename Elem, unsigned long Rank, bool MultiDimPtr, typename Idxs>
struct GetCoreT<Ref<Block<Elem,Rank,MultiDimPtr>,Idxs>> { using Core_T = Elem; };

///
/// Core type is Elem
template <typename Elem, unsigned long Rank, bool MultiDimPtr, typename Idxs>
struct GetCoreT<Ref<View<Elem,Rank,MultiDimPtr>,Idxs>> { using Core_T = Elem; };

///
/// Core type is the core type of Unary
template <typename Functor, typename CompoundExpr>
struct GetCoreT<Unary<Functor,CompoundExpr>> { 
  using Core_T = typename Unary<Functor,CompoundExpr>::Core_T;
};

///
/// Core type is the core type of Binary
template <typename Functor, typename CompoundExpr0, typename CompoundExpr1>
struct GetCoreT<Binary<Functor,CompoundExpr0,CompoundExpr1>> { 
  using Core_T = typename Binary<Functor,CompoundExpr0,CompoundExpr1>::Core_T;
};

///
/// Core type is loop_type
template <char C>
struct GetCoreT<Iter<C>> { using Core_T = loop_type; };

///
/// Core type is Elem
template <typename Elem, typename IsPrimitive>
struct GetCoreT<SField<Elem,IsPrimitive>> { using Core_T = Elem; };

///
/// Core type is core type of T
template <typename T>
struct GetCoreT<T*> { using Core_T = typename GetCoreT<T>::Core_T; };

///
/// Core type is core type of T
template <typename T>
struct GetCoreT<T&> { using Core_T = typename GetCoreT<T>::Core_T; };

///
/// Core type is core type of T
template <typename T>
struct GetCoreT<T[]> { using Core_T = typename GetCoreT<T>::Core_T; };

///
/// Core type is core type of T
template <typename T, int sz>
struct GetCoreT<T[sz]> { using Core_T = typename GetCoreT<T>::Core_T; };

///
/// Core type is To
template <typename To, typename CompoundExpr>
struct GetCoreT<TemplateCast<To,CompoundExpr>> { using Core_T = To; };

///
/// Core type is TBranch (or FBranch)
template <typename Cond, typename TBranch, typename FBranch>
struct GetCoreT<TernaryCond<Cond,TBranch,FBranch>> { using Core_T = typename GetCoreT<TBranch>::Core_T; };

///
/// Core type is 
template <int GOPType, typename Option, typename...Options>
struct GetCoreT<GroupedJunctionOperator<GOPType,Option,Options...>> { using Core_T = typename GetCoreT<Option>::Core_T; };

///
/// Represents a compound expression. This class just implements the overloads, but all the realization
/// and such happens in the derived classes.
template <typename Derived>
struct Expr {

  ///
  /// Build not compound expression
  Unary<NotFunctor,Derived> operator!();

  ///
  /// Build invert compound expression
  Unary<BitwiseInvertFunctor,Derived> operator~();

  ///
  /// Build negate compound expression
  Unary<NegateFunctor,Derived> operator-();

  ///
  /// Build addition compound expression
  template <typename Rhs>
  Binary<AddFunctor,Derived,Rhs> operator+(const Rhs &rhs);

  ///
  /// Build subtraction compound expression
  template <typename Rhs>
  Binary<SubFunctor,Derived,Rhs> operator-(const Rhs &rhs);

  ///
  /// Build multiplication compound expression
  template <typename Rhs>
  Binary<MulFunctor,Derived,Rhs> operator*(const Rhs &rhs);

  ///
  /// Build division compound expression
  template <typename Rhs>
  Binary<DivFunctor,Derived,Rhs> operator/(const Rhs &rhs);

  ///
  /// Build left shift compound expression
  template <typename Rhs>
  Binary<LShiftFunctor,Derived,Rhs> operator<<(const Rhs &rhs);

  ///
  /// Build right shift compound expression
  template <typename Rhs>
  Binary<RShiftFunctor,Derived,Rhs> operator>>(const Rhs &rhs);

  ///
  /// Build less than compound expression
  template <typename Rhs>
  auto operator<(const Rhs &rhs);

  ///
  /// Build less than or equals compound expression
  template <typename Rhs>
  auto operator<=(const Rhs &rhs);

  ///
  /// Build greater than compound expression
  template <typename Rhs>
  auto operator>(const Rhs &rhs);

  ///
  /// Build greater than or equals compound expression
  template <typename Rhs>
  auto operator>=(const Rhs &rhs);

  ///
  /// Build equals compound expression
  template <typename Rhs>
  auto operator==(const Rhs &rhs);

  ///
  /// Build not equals compound expression
  template <typename Rhs>
  auto operator!=(const Rhs &rhs);

  ///
  /// Build boolean and compound expression
  template <typename Rhs>
  auto operator&&(const Rhs &rhs);

  ///
  /// Build boolean or compound expression
  template <typename Rhs>
  auto operator||(const Rhs &rhs);

  ///
  /// Build bitwise and compound expression
  template <typename Rhs>
  Binary<BitwiseAndFunctor,Derived,Rhs> operator&(const Rhs &rhs);

  ///
  /// Build bitwise or compound expression
  template <typename Rhs>
  Binary<BitwiseOrFunctor,Derived,Rhs> operator|(const Rhs &rhs);

};

///
/// Represents a loop iterator for use with inline indexing statements.
template <char Ident>
struct Iter : public Expr<Iter<Ident>> {

  /// 
  /// The core type
  using Core_T = loop_type;

  ///
  /// The identifier of this Iter
  static constexpr char Ident_T = Ident;

  ///
  /// Realize the value of the current loop iteration
  template <typename LhsIdxs, unsigned long N>
  dvar<loop_type> realize(const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters) {
    // figure out the index of Ident within lhs_idxs so I can get the correct
    // iter
    constexpr int idx = find_iter_idx<LhsIdxs>();
    static_assert(idx < N);
    return iters[idx];
  }

private:

  ///
  /// Find the location of the current Iter within the specified indicies
  /// by matching the char identifier.
  template <typename LhsIdxs>
  static constexpr int find_iter_idx();
  
};

///
/// Represents a cast operation on a compound expression
template <typename To, typename CompoundExpr>
struct TemplateCast : public Expr<TemplateCast<To,CompoundExpr>> {
  
  ///
  /// The core type
  using Core_T = To;
  
  TemplateCast(const CompoundExpr &compound_expr) : compound_expr(compound_expr) { }
  
  ///
  /// Realize the operation on the compound expression
  template <typename LhsIdxs, unsigned long N>
  dvar<To> realize(const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters);

private:

  CompoundExpr compound_expr;

};

/// 
/// Represents a ternary operation on compound expressions
template <typename Cond, typename TBranch, typename FBranch>
struct TernaryCond : public Expr<TernaryCond<Cond,TBranch,FBranch>> {

  /// 
  /// The core type
  using Core_T = typename GetCoreT<TBranch>::Core_T;

  TernaryCond(const Cond &cond, const TBranch &tbranch, const FBranch &fbranch) :
    cond(cond), tbranch(tbranch), fbranch(fbranch) { 
    static_assert(std::is_same<Core_T, typename GetCoreT<FBranch>::Core_T>(),
		  "Ternary branches must be the same!");
  }

  /// 
  /// Realize the ternary (which generates a call to a macro)
  template <typename LhsIdxs, unsigned long N>
  dvar<Core_T> realize(const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters);

  Cond cond;
  TBranch tbranch;
  FBranch fbranch;

};

///
/// A binary operation on compound expressions.
template <typename Functor, typename CompoundExpr0, typename CompoundExpr1>
struct Binary : public Expr<Binary<Functor, CompoundExpr0, CompoundExpr1>> {

  /// 
  /// The core type
  using Core_T = typename GetCoreT<CompoundExpr0>::Core_T;

  Binary(const CompoundExpr0 &compound_expr0, const CompoundExpr1 &compound_expr1) :
    compound_expr0(compound_expr0), compound_expr1(compound_expr1) {     
    static_assert(std::is_same<
		  typename GetCoreT<CompoundExpr0>::Core_T,
		  typename GetCoreT<CompoundExpr1>::Core_T>(), 
		  "Operands to Binary must have the same core type");
  }

  ///
  /// Realize the operation on the compound expression.
  /// Utilized with inline expressions that utilize blocks, views, and refs.
  template <typename LhsIdxs, unsigned long N>
  dvar<Core_T> realize(const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters);

  ///
  /// Realize the operation on the compound expression.
  /// Utilized with StagedObject fields when you do something like
  /// dvar<int> i = t.field0 + 2;
  /// The user could mess this up and do something illegal like
  /// use a ref on the righthand side, or an Iter or whatever,
  /// but it would fail during execution (at least if using an 
  /// Iter) since the Iter wouldn't exist.
  operator dvar<Core_T>();

private:
  
  CompoundExpr0 compound_expr0;
  CompoundExpr1 compound_expr1;
  
};


template <int Depth, typename Functor, int GOPType, typename Rhs, typename...Options>
auto apply_lhs_grouped_junction_operator(std::tuple<Options...> group, const Rhs &rhs) {
  if constexpr (Depth == 0 && sizeof...(Options) == 0) { 
    // no junction needed
    return rhs;
  } else if constexpr (Depth < sizeof...(Options) - 1) {
    auto d = std::get<Depth>(group);
    Binary<Functor, decltype(d), Rhs> cur_binary(d, rhs);
    auto applied = apply_lhs_grouped_junction_operator<Depth+1,Functor,GOPType>(group, rhs);
    if constexpr (GOPType == 0) {
      return Binary<OrFunctor,decltype(cur_binary),decltype(applied)>(cur_binary, applied);
    } else {
      return Binary<AndFunctor,decltype(cur_binary),decltype(applied)>(cur_binary, applied);
    }
  } else {
    auto d = std::get<Depth>(group);
    Binary<Functor, decltype(d), Rhs> cur_binary(d, rhs);
    return cur_binary;
  }
}

template <int Depth, typename Functor, int GOPType, typename Lhs, typename...Options>
auto apply_rhs_grouped_junction_operator(const Lhs &lhs, std::tuple<Options...> group) {
  if constexpr (Depth == 0 && sizeof...(Options) == 0) { 
    // no junction needed
    return lhs;
  } else if constexpr (Depth < sizeof...(Options) - 1) {
    auto d = std::get<Depth>(group);
    Binary<Functor, Lhs, decltype(d)> cur_binary(lhs, d);
    auto applied = apply_rhs_grouped_junction_operator<Depth+1,Functor,GOPType>(lhs, group);
    if constexpr (GOPType == 0) {
      return Binary<OrFunctor,decltype(applied),decltype(cur_binary)>(applied, cur_binary);
    } else {
      return Binary<AndFunctor,decltype(applied),decltype(cur_binary)>(applied, cur_binary);
    }
  } else {
    auto d = std::get<Depth>(group);
    Binary<Functor, Lhs, decltype(d)> cur_binary(lhs, d);
    return cur_binary;
  }
}

template <typename Functor, typename Lhs, typename Rhs>
auto maybe_apply_grouped_junction_operator(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!(is_grouped_junction_operator<Lhs>::value && is_grouped_junction_operator<Rhs>::value));
  if constexpr (is_grouped_junction_operator<Lhs>::value) {
    return apply_lhs_grouped_junction_operator<0,Functor,Lhs::GOPType_T>(lhs.group, rhs);
  } else if constexpr (is_grouped_junction_operator<Rhs>::value) {
    return apply_rhs_grouped_junction_operator<0,Functor,Rhs::GOPType_T>(lhs, rhs.group);
  } else {
    return Binary<Functor,Lhs,Rhs>(lhs,rhs);
  }
}

template <typename Derived>
Unary<NotFunctor,Derived> Expr<Derived>::operator!() {
  return Unary<NotFunctor,Derived>(*static_cast<Derived*>(this));
}  

template <typename Derived>
Unary<BitwiseInvertFunctor,Derived> Expr<Derived>::operator~() {
  return Unary<BitwiseInvertFunctor,Derived>(*static_cast<Derived*>(this));
}

template <typename Derived>
Unary<NegateFunctor,Derived> Expr<Derived>::operator-() {
  return Unary<NegateFunctor,Derived>(*static_cast<Derived*>(this));
}

template <typename Derived>
template <typename Rhs>
Binary<AddFunctor,Derived,Rhs> Expr<Derived>::operator+(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<AddFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<SubFunctor,Derived,Rhs> Expr<Derived>::operator-(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<SubFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<MulFunctor,Derived,Rhs> Expr<Derived>::operator*(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<MulFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<DivFunctor,Derived,Rhs> Expr<Derived>::operator/(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<DivFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<LShiftFunctor,Derived,Rhs> Expr<Derived>::operator<<(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<LShiftFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<RShiftFunctor,Derived,Rhs> Expr<Derived>::operator>>(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<RShiftFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator<(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<LTFunctor<false>>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator<=(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<LTFunctor<true>>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator>(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<GTFunctor<false>>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator>=(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<GTFunctor<true>>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator==(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<EqFunctor<false>>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator!=(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<EqFunctor<true>>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator&&(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<AndFunctor>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
auto Expr<Derived>::operator||(const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<OrFunctor>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<BitwiseAndFunctor,Derived,Rhs> Expr<Derived>::operator&(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<BitwiseAndFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<BitwiseOrFunctor,Derived,Rhs> Expr<Derived>::operator|(const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<BitwiseOrFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

///
/// Free version of Expr::operator+ between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<AddFunctor,Lhs,Rhs> operator+(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<AddFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator- between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<SubFunctor,Lhs,Rhs> operator-(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<SubFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator* between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<MulFunctor,Lhs,Rhs> operator*(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<MulFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator/ between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<DivFunctor,Lhs,Rhs> operator/(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<DivFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator<< between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<LShiftFunctor,Lhs,Rhs> operator<<(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<LShiftFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator>> between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<RShiftFunctor,Lhs,Rhs> operator>>(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<RShiftFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator< between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator<(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<LTFunctor<false>>(lhs, rhs);
}

///
/// Free version of Expr::operator<= between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator<=(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<LTFunctor<true>>(lhs, rhs);
}

///
/// Free version of Expr::operator> between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator>(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<GTFunctor<false>>(lhs, rhs);
}

///
/// Free version of Expr::operator>= between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator>=(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<GTFunctor<true>>(lhs, rhs);
}

///
/// Free version of Expr::operator== between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator==(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<EqFunctor<false>>(lhs, rhs);
}

///
/// Free version of Expr::operator!= between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator!=(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<EqFunctor<true>>(lhs, rhs);
}

///
/// Free version of Expr::operator&& between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator&&(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<AndFunctor>(lhs, rhs);
}

///
/// Free version of Expr::operator|| between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
auto operator||(const Lhs &lhs, const Rhs &rhs) {
  return maybe_apply_grouped_junction_operator<OrFunctor>(lhs, rhs);
}

///
/// Free version of Expr::operator& between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<BitwiseAndFunctor,Lhs,Rhs> operator&(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<BitwiseAndFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator| between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<BitwiseOrFunctor,Lhs,Rhs> operator|(const Lhs &lhs, const Rhs &rhs) {
  static_assert(!is_grouped_junction_operator<Lhs>::value);
  static_assert(!is_grouped_junction_operator<Rhs>::value);
  return Binary<BitwiseOrFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Unwind an Expr by realizing the subexpressions, or just return the value if not
/// an Expr.
template <typename T, typename LhsIdxs, unsigned long N>
dvar<typename GetCoreT<T>::Core_T> dispatch_realize(T to_realize, const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters) {
  if constexpr (is_expr<T>::value) {
    return to_realize.realize(lhs_idxs, iters);
  } else {
    // something primitive-y, like a loop_type or another builder
    return to_realize;
  }
}

template <typename Functor, typename CompoundExpr0, typename CompoundExpr1>
template <typename LhsIdxs, unsigned long N>
dvar<typename GetCoreT<CompoundExpr0>::Core_T> Binary<Functor,CompoundExpr0,CompoundExpr1>::realize(const LhsIdxs &lhs_idxs, 
												    const darr<loop_type,N> &iters) {
  if constexpr (std::is_fundamental<CompoundExpr0>::value && std::is_fundamental<CompoundExpr1>::value) {
    return Functor()(compound_expr0, compound_expr1);
  } else if constexpr (std::is_fundamental<CompoundExpr0>::value) {
    dvar<Core_T> op1 = dispatch_realize(compound_expr1, lhs_idxs, iters);
    return Functor()(compound_expr0, op1);
  } else if constexpr (std::is_fundamental<CompoundExpr1>::value) {
    dvar<Core_T> op0 = dispatch_realize(compound_expr0, lhs_idxs, iters);
    return Functor()(op0, compound_expr1);
  } else {
    dvar<Core_T> op0 = dispatch_realize(compound_expr0, lhs_idxs, iters);
    dvar<Core_T> op1 = dispatch_realize(compound_expr1, lhs_idxs, iters);
    return Functor()(op0, op1);
  }
}

///
/// Unspecialized functor for calling the appropriate external cast operation 
template <typename To, typename From>
struct DispatchCast { };

///
/// Cast to uint8_t
template <typename From>
struct DispatchCast<uint8_t,From> { dvar<uint8_t>  operator()(From val) { return cast_uint8_t(val); } };
  
///
/// Cast to uint16_t
template <typename From>
struct DispatchCast<uint16_t,From> { dvar<uint16_t> operator()(From val) { return cast_uint16_t(val); } };

///
/// Cast to uint32_t
template <typename From>
struct DispatchCast<uint32_t,From> { dvar<uint32_t>  operator()(From val) { return cast_uint32_t(val); } };

///
/// Cast to uint64_t
template <typename From>
struct DispatchCast<uint64_t,From> { dvar<uint64_t>  operator()(From val) { return cast_uint64_t(val); } };

///
/// Cast to char
template <typename From>
struct DispatchCast<char,From> { dvar<char>  operator()(From val) { return cast_char(val); } };

///
/// Cast to int8_t
template <typename From>
struct DispatchCast<int8_t,From> { dvar<int8_t>  operator()(From val) { return cast_int8_t(val); } };

///
/// Cast to int16_t
template <typename From>
struct DispatchCast<int16_t,From> { dvar<int16_t>  operator()(From val) { return cast_int16_t(val); } };

///
/// Cast to int32_t
template <typename From>
struct DispatchCast<int32_t,From> { dvar<int32_t>  operator()(From val) { return cast_int32_t(val); } };

///
/// Cast to int64_t
template <typename From>
struct DispatchCast<int64_t,From> { dvar<int64_t>  operator()(From val) { return cast_int64_t(val); } };
    
///
/// Cast to float
template <typename From>
struct DispatchCast<float,From> { dvar<float> operator()(From val) { return cast_float(val); } };

///
/// Cast to double
template <typename From>
struct DispatchCast<double,From> { dvar<double> operator()(From val) { return cast_double(val); } };

    
///
/// Syntactic sugar for creating a TemplateCast
template <typename To, typename From>
TemplateCast<To,From> rcast(From val) {
  return TemplateCast<To,From>(val);
}

///
/// Apply either a TemplateCast (for Exprs) or call the external cast function that generates
/// a (T)val cast in the generated code.
template <typename To, typename From>
auto cast(From val) {
  if constexpr (is_hmda_fundamental<From>::value || is_dyn_like<From>::value) {
    return DispatchCast<To,From>()(val);
  } else {
    return rcast<To>(val);
  }
}

/// 
/// Build a TernaryCond
template <typename Cond, typename TBranch, typename FBranch>
TernaryCond<Cond,TBranch,FBranch> select(Cond cond, TBranch tbranch, FBranch fbranch) {
  return TernaryCond<Cond,TBranch,FBranch>(cond,tbranch,fbranch);
}

template <typename To, typename CompoundExpr>
template <typename LhsIdxs, unsigned long N>
dvar<To> TemplateCast<To,CompoundExpr>::realize(const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters) {
  if constexpr (std::is_fundamental<CompoundExpr>::value) {
    dvar<To> casted = cast<To>(compound_expr);
    return casted;
  } else {
    auto op = dispatch_realize(compound_expr, lhs_idxs, iters);
    dvar<To> casted = cast<To>(op);
    return casted;
  }
}

template <typename Cond, typename TBranch, typename FBranch>
template <typename LhsIdxs, unsigned long N>
dvar<typename GetCoreT<TBranch>::Core_T> 
TernaryCond<Cond,TBranch,FBranch>::realize(const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters) {
  dvar<typename GetCoreT<Cond>::Core_T> cond_realized;
  dvar<typename GetCoreT<TBranch>::Core_T> tbranch_realized;
  dvar<typename GetCoreT<FBranch>::Core_T> fbranch_realized;
  if constexpr (std::is_fundamental<Cond>::value) {
    cond_realized = cond;
  } else {
    cond_realized = dispatch_realize(cond, lhs_idxs, iters);
  }
  if constexpr (std::is_fundamental<TBranch>::value) {
    tbranch_realized = tbranch;
  } else {
    tbranch_realized = dispatch_realize(tbranch, lhs_idxs, iters);
  }
  if constexpr (std::is_fundamental<FBranch>::value) {
    fbranch_realized = fbranch;
  } else {
    fbranch_realized = dispatch_realize(fbranch, lhs_idxs, iters);
  }
  return ternary_cond_wrapper(cond_realized, tbranch_realized, fbranch_realized);
}

template <char Ident, int Depth, typename LhsIdx, typename...LhsIdxs>
struct FindIterIdx {
  constexpr int operator()() const {
    if constexpr (sizeof...(LhsIdxs) == 0) {
      return -1; // not found
    } else {
      return FindIterIdx<Ident, Depth+1, LhsIdxs...>()();
    }
  }
};

template <char Ident, typename LhsIdxs>
struct FindIterIdxProxy { };
  
template <char Ident, typename...LhsIdxs>
struct FindIterIdxProxy<Ident, std::tuple<LhsIdxs...>> {
  constexpr int operator()() const { return FindIterIdx<Ident, 0, LhsIdxs...>()(); }
};

template <char Ident, int Depth, typename...LhsIdxs>
struct FindIterIdx<Ident, Depth, Iter<Ident>, LhsIdxs...> {
  constexpr int operator()() const { return Depth; }
};

template <char Ident>
template <typename LhsIdxs>
constexpr int Iter<Ident>::find_iter_idx() {
  constexpr int idx = FindIterIdxProxy<Ident,LhsIdxs>()();
  static_assert(idx != -1);
  return idx;
}

}
