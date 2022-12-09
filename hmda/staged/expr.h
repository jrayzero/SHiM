// -*-c++-*-

#pragma once

#include <type_traits>
#include "builder/dyn_var.h"
#include "common/functors.h"
#include "fwrappers.h"
#include "fwddecls.h"
#include "traits.h"

namespace hmda {

///
/// Unspecialized template to determine the core type of a compound expression
template <typename T>
struct GetCoreT { };

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

template <typename Elem, int Rank, typename Idxs>
struct GetCoreT<Ref<Block<Elem,Rank>,Idxs>> { using Core_T = Elem; };

template <typename Elem, int Rank, typename Idxs>
struct GetCoreT<Ref<View<Elem,Rank>,Idxs>> { using Core_T = Elem; };

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
/// Core type is To
template <typename To, typename CompoundExpr>
struct GetCoreT<TemplateCast<To,CompoundExpr>> { using Core_T = To; };

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
  Binary<LTFunctor<false>,Derived,Rhs> operator<(const Rhs &rhs);

  ///
  /// Build less than or equals compound expression
  template <typename Rhs>
  Binary<LTFunctor<true>,Derived,Rhs> operator<=(const Rhs &rhs);

  ///
  /// Build greater than compound expression
  template <typename Rhs>
  Binary<GTFunctor<false>,Derived,Rhs> operator>(const Rhs &rhs);

  ///
  /// Build greater than or equals compound expression
  template <typename Rhs>
  Binary<GTFunctor<true>,Derived,Rhs> operator>=(const Rhs &rhs);

  ///
  /// Build equals compound expression
  template <typename Rhs>
  Binary<EqFunctor<false>,Derived,Rhs> operator==(const Rhs &rhs);

  ///
  /// Build not equals compound expression
  template <typename Rhs>
  Binary<EqFunctor<true>,Derived,Rhs> operator!=(const Rhs &rhs);

  ///
  /// Build boolean and compound expression
  template <typename Rhs>
  Binary<AndFunctor,Derived,Rhs> operator&&(const Rhs &rhs);

  ///
  /// Build boolean or compound expression
  template <typename Rhs>
  Binary<OrFunctor,Derived,Rhs> operator||(const Rhs &rhs);

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
  template <typename LhsIdxs, typename Iters>
  builder::dyn_var<loop_type> realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    // figure out the index of Ident within lhs_idxs so I can get the correct
    // iter
    constexpr int idx = find_iter_idx<LhsIdxs>();
    return std::get<idx>(iters);
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
  template <typename LhsIdxs, typename Iters>
  builder::dyn_var<To> realize(const LhsIdxs &lhs_idxs, const Iters &iters);

private:

  CompoundExpr compound_expr;

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
  /// Realize the operation on the compound expression
  template <typename LhsIdxs, typename Iters>
  builder::dyn_var<Core_T> realize(const LhsIdxs &lhs_idxs, const Iters &iters);

private:
  
  CompoundExpr0 compound_expr0;
  CompoundExpr1 compound_expr1;
  
};

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
  return Binary<AddFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<SubFunctor,Derived,Rhs> Expr<Derived>::operator-(const Rhs &rhs) {
  return Binary<SubFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<MulFunctor,Derived,Rhs> Expr<Derived>::operator*(const Rhs &rhs) {
  return Binary<MulFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<DivFunctor,Derived,Rhs> Expr<Derived>::operator/(const Rhs &rhs) {
  return Binary<DivFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<LShiftFunctor,Derived,Rhs> Expr<Derived>::operator<<(const Rhs &rhs) {
  return Binary<LShiftFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<RShiftFunctor,Derived,Rhs> Expr<Derived>::operator>>(const Rhs &rhs) {
  return Binary<RShiftFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<LTFunctor<false>,Derived,Rhs> Expr<Derived>::operator<(const Rhs &rhs) {
  return Binary<LTFunctor<false>,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<LTFunctor<true>,Derived,Rhs> Expr<Derived>::operator<=(const Rhs &rhs) {
  return Binary<LTFunctor<true>,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<GTFunctor<false>,Derived,Rhs> Expr<Derived>::operator>(const Rhs &rhs) {
  return Binary<GTFunctor<false>,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<GTFunctor<true>,Derived,Rhs> Expr<Derived>::operator>=(const Rhs &rhs) {
  return Binary<GTFunctor<true>,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<EqFunctor<false>,Derived,Rhs> Expr<Derived>::operator==(const Rhs &rhs) {
  return Binary<EqFunctor<false>,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<EqFunctor<true>,Derived,Rhs> Expr<Derived>::operator!=(const Rhs &rhs) {
  return Binary<EqFunctor<true>,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<AndFunctor,Derived,Rhs> Expr<Derived>::operator&&(const Rhs &rhs) {
  return Binary<AndFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<OrFunctor,Derived,Rhs> Expr<Derived>::operator||(const Rhs &rhs) {
  return Binary<OrFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<BitwiseAndFunctor,Derived,Rhs> Expr<Derived>::operator&(const Rhs &rhs) {
  return Binary<BitwiseAndFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

template <typename Derived>
template <typename Rhs>
Binary<BitwiseOrFunctor,Derived,Rhs> Expr<Derived>::operator|(const Rhs &rhs) {
  return Binary<BitwiseOrFunctor,Derived,Rhs>(*static_cast<Derived*>(this), rhs);
}

///
/// Free version of Expr<Derived>::operator+ between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<AddFunctor,Lhs,Rhs> operator+(const Lhs &lhs, const Rhs &rhs) {
  return Binary<AddFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator- between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<SubFunctor,Lhs,Rhs> operator-(const Lhs &lhs, const Rhs &rhs) {
  return Binary<SubFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator* between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<MulFunctor,Lhs,Rhs> operator*(const Lhs &lhs, const Rhs &rhs) {
  return Binary<MulFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator/ between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<DivFunctor,Lhs,Rhs> operator/(const Lhs &lhs, const Rhs &rhs) {
  return Binary<DivFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator<< between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<LShiftFunctor,Lhs,Rhs> operator<<(const Lhs &lhs, const Rhs &rhs) {
  return Binary<LShiftFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator>> between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<RShiftFunctor,Lhs,Rhs> operator>>(const Lhs &lhs, const Rhs &rhs) {
  return Binary<RShiftFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator< between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<LTFunctor<false>,Lhs,Rhs> operator<(const Lhs &lhs, const Rhs &rhs) {
  return Binary<LTFunctor<false>,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator<= between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<LTFunctor<true>,Lhs,Rhs> operator<=(const Lhs &lhs, const Rhs &rhs) {
  return Binary<LTFunctor<true>,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator> between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<GTFunctor<false>,Lhs,Rhs> operator>(const Lhs &lhs, const Rhs &rhs) {
  return Binary<GTFunctor<false>,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator>= between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<GTFunctor<true>,Lhs,Rhs> operator>=(const Lhs &lhs, const Rhs &rhs) {
  return Binary<GTFunctor<true>,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator== between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<EqFunctor<false>,Lhs,Rhs> operator==(const Lhs &lhs, const Rhs &rhs) {
  return Binary<EqFunctor<false>,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator!= between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<EqFunctor<true>,Lhs,Rhs> operator!=(const Lhs &lhs, const Rhs &rhs) {
  return Binary<EqFunctor<true>,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator&& between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<AndFunctor,Lhs,Rhs> operator&&(const Lhs &lhs, const Rhs &rhs) {
  return Binary<AndFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator|| between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<OrFunctor,Lhs,Rhs> operator||(const Lhs &lhs, const Rhs &rhs) {
  return Binary<OrFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator& between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<BitwiseAndFunctor,Lhs,Rhs> operator&(const Lhs &lhs, const Rhs &rhs) {
  return Binary<BitwiseAndFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Free version of Expr::operator| between non-Expr and Expr
template <typename Lhs, typename Rhs,
	  typename std::enable_if<is_expr<Rhs>::value, int>::type = 0>
Binary<BitwiseOrFunctor,Lhs,Rhs> operator|(const Lhs &lhs, const Rhs &rhs) {
  return Binary<BitwiseOrFunctor,Lhs,Rhs>(lhs, rhs);
}

///
/// Unwind an Expr by realizing the subexpressions, or just return the value if not
/// an Expr.
template <typename T, typename LhsIdxs, typename Iters>
builder::dyn_var<loop_type> dispatch_realize(T to_realize, const LhsIdxs &lhs_idxs, const Iters &iters) {
  if constexpr (is_expr<T>::value) {
    return to_realize.realize(lhs_idxs, iters);
  } else {
    // something primitive-y, like a loop_type or another builder
    return to_realize;
  }
}

template <typename Functor, typename CompoundExpr0, typename CompoundExpr1>
template <typename LhsIdxs, typename Iters>
builder::dyn_var<typename GetCoreT<CompoundExpr0>::Core_T> Binary<Functor,CompoundExpr0,CompoundExpr1>::realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
  if constexpr (std::is_fundamental<CompoundExpr0>::value && std::is_fundamental<CompoundExpr1>::value) {
    return Functor()(compound_expr0, compound_expr1);
  } else if constexpr (std::is_fundamental<CompoundExpr0>::value) {
    builder::dyn_var<Core_T> op1 = dispatch_realize(compound_expr1, lhs_idxs, iters);
    return Functor()(compound_expr0, op1);
  } else if constexpr (std::is_fundamental<CompoundExpr1>::value) {
    builder::dyn_var<Core_T> op0 = dispatch_realize(compound_expr0, lhs_idxs, iters);
    return Functor()(op0, compound_expr1);
  } else {
    builder::dyn_var<Core_T> op0 = dispatch_realize(compound_expr0, lhs_idxs, iters);
    builder::dyn_var<Core_T> op1 = dispatch_realize(compound_expr1, lhs_idxs, iters);
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
struct DispatchCast<uint8_t,From> { auto operator()(From val) { return cast_uint8_t(val); } };
  
///
/// Cast to uint16_t
template <typename From>
struct DispatchCast<uint16_t,From> { auto operator()(From val) { return cast_uint16_t(val); } };

///
/// Cast to uint32_t
template <typename From>
struct DispatchCast<uint32_t,From> { auto operator()(From val) { return cast_uint32_t(val); } };

///
/// Cast to uint64_t
template <typename From>
struct DispatchCast<uint64_t,From> { auto operator()(From val) { return cast_uint64_t(val); } };

///
/// Cast to int16_t
template <typename From>
struct DispatchCast<int16_t,From> { auto operator()(From val) { return cast_int16_t(val); } };

///
/// Cast to int32_t
template <typename From>
struct DispatchCast<int32_t,From> { auto operator()(From val) { return cast_int32_t(val); } };

///
/// Cast to int64_t
template <typename From>
struct DispatchCast<int64_t,From> { auto operator()(From val) { return cast_int64_t(val); } };
    
///
/// Cast to float
template <typename From>
struct DispatchCast<float,From> { auto operator()(From val) { return cast_float(val); } };

///
/// Cast to double
template <typename From>
struct DispatchCast<double,From> { auto operator()(From val) { return cast_double(val); } };

    
///
/// Syntactic sugar for creating a TemplateCast
template <typename To, typename From>
auto rcast(From val) {
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

template <typename To, typename CompoundExpr>
template <typename LhsIdxs, typename Iters>
builder::dyn_var<To> TemplateCast<To,CompoundExpr>::realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
  if constexpr (std::is_fundamental<CompoundExpr>::value) {
    builder::dyn_var<To> casted = cast<To>(compound_expr);
    return casted;
  } else {
    auto op = dispatch_realize(compound_expr, lhs_idxs, iters);
    builder::dyn_var<To> casted = cast<To>(op);
    return casted;
  }
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
