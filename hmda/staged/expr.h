// -*-c++-*-

#pragma once

#include <type_traits>
#include "builder/dyn_var.h"
#include "functors.h"
#include "staged_utils.h"
#include "fwrappers.h"
#include "fwddecls.h"
#include "traits.h"

namespace hmda {

template <typename Derived>
struct Expr {

#define UNARY(functor, name)						\
  Unary<functor##Functor, Derived> name() {				\
    return Unary<functor##Functor, Derived>(*static_cast<Derived*>(this)); \
  }
  
#define BINARY(functor, name)						\
  template <typename Rhs>						\
  Binary<functor##Functor, Derived, Rhs> name(const Rhs &rhs) {		\
    return Binary<functor##Functor, Derived, Rhs>(*static_cast<Derived*>(this), rhs); \
  }
  
  BINARY(Add, operator+);
  BINARY(Sub, operator-);
  BINARY(Mul, operator*);
  BINARY(Div, operator/);
  BINARY(RShift, operator>>);
  BINARY(LShift, operator<<);
  BINARY(LT, operator<);
  BINARY(LTE, operator<=);
  BINARY(GT, operator>);
  BINARY(GTE, operator>=);
  BINARY(Eq, operator==);
  BINARY(NEq, operator!=);
  BINARY(And, operator&&);
  BINARY(Or, operator||);
  BINARY(BAnd, operator&);
  BINARY(BOr, operator|);
  UNARY(Not, operator!);
  UNARY(BNot, operator~);
  UNARY(Invert, operator-);

};

#define FREE_BINARY(functor, name)					\
  template <typename Lhs, typename Rhs,					\
	    typename std::enable_if<std::is_fundamental<Lhs>::value, int>::type = 0, \
	    typename std::enable_if<is_expr<Rhs>::value, int>::type = 0> \
  auto name(Lhs lhs, const Rhs &rhs) {					\
    return Binary<functor##Functor, Lhs, Rhs>(lhs, rhs);		\
  }

FREE_BINARY(Add, operator+);
FREE_BINARY(Sub, operator-);
FREE_BINARY(Mul, operator*);
FREE_BINARY(Div, operator/);
FREE_BINARY(RShift, operator>>);
FREE_BINARY(LShift, operator<<);
FREE_BINARY(LT, operator<);
FREE_BINARY(LTE, operator<=);
FREE_BINARY(GT, operator>);
FREE_BINARY(GTE, operator>=);
FREE_BINARY(Eq, operator==);
FREE_BINARY(NEq, operator!=);
FREE_BINARY(And, operator&&);
FREE_BINARY(Or, operator||);
FREE_BINARY(BAnd, operator&);
FREE_BINARY(BOr, operator|);

template <typename T, typename LhsIdxs, typename Iters>
builder::dyn_var<loop_type> dispatch_realize(T to_realize, const LhsIdxs &lhs_idxs, const Iters &iters) {
  if constexpr (is_expr<T>::value) {
    return to_realize.realize(lhs_idxs, iters);
  } else {
    // something primitive-y, like a loop_type or another builder
    return to_realize;
  }
}

// cast a dyn var or plain value
template <typename To, typename From>
auto cast(From val);

template <typename To, typename Operand>
struct TemplateCast : public Expr<TemplateCast<To,Operand>> {

  using Realized_T = To;

  TemplateCast(const Operand &operand) : operand(operand) { }

  template <typename LhsIdxs, typename Iters>
  builder::dyn_var<To> realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    if constexpr (std::is_fundamental<Operand>::value) {
      builder::dyn_var<To> casted = cast<To>(operand);
      return casted;
    } else {
      auto op = dispatch_realize(operand, lhs_idxs, iters);
      builder::dyn_var<To> casted = cast<To>(op);
      return casted;
    }
  }

private:
  Operand operand;
};

template <typename To, typename From>
auto rcast(From val) {
  return TemplateCast<To,From>(val);
}

template <typename Elem, int Rank>
struct Block;
template <typename Elem, int Rank>
struct View;

template <typename To, typename From>
struct DispatchCast { };
// this defines cast for both plain/dynvars AND if they are wrapped in an Expr
#define DISPATCH_CAST(dtype)						\
  template <typename From>						\
  struct DispatchCast<dtype,From> { auto operator()(From val) { return cast_##dtype(val); } }; \
  template <typename Elem, int Rank, typename Idxs>			\
  struct DispatchCast<dtype,Ref<Block<Elem,Rank>,Idxs>> {		\
    auto operator()(Ref<Block<Elem,Rank>,Idxs> val) { return rcast<dtype>(val); } \
  };									\
  template <typename Elem, int Rank, typename Idxs>			\
  struct DispatchCast<dtype,Ref<View<Elem,Rank>,Idxs>> {		\
    auto operator()(Ref<View<Elem,Rank>,Idxs> val) { return rcast<dtype>(val); } \
  };									\
  template <char C>							\
  struct DispatchCast<dtype,Iter<C>> {					\
    auto operator()(Iter<C> val) { return rcast<dtype>(val); }		\
  };									\
  template <typename Functor, typename Operand>				\
  struct DispatchCast<dtype,Unary<Functor,Operand>> {			\
    auto operator()(Unary<Functor,Operand> val) { return rcast<dtype>(val); } \
  };									\
  template <typename Functor, typename Operand0, typename Operand1>	\
  struct DispatchCast<dtype,Binary<Functor,Operand0,Operand1>> {	\
    auto operator()(Binary<Functor,Operand0,Operand1> val) { return rcast<dtype>(val); } \
  };									

DISPATCH_CAST(uint8_t);
DISPATCH_CAST(uint16_t);
DISPATCH_CAST(uint32_t);
DISPATCH_CAST(uint64_t);
DISPATCH_CAST(int16_t);
DISPATCH_CAST(int32_t);
DISPATCH_CAST(int64_t);
DISPATCH_CAST(float);
DISPATCH_CAST(double);

// cast a dyn var or plain value
template <typename To, typename From>
auto cast(From val) {
  // don't separately generate the fptr because it will have the wrong type
  return DispatchCast<To,From>()(val);
}

template <typename T>
struct GetRealizedT { };

#define REALIZED(dtype) \
  template <>						\
  struct GetRealizedT<dtype> { using Realized_T=dtype; };
REALIZED(uint8_t);
REALIZED(uint16_t);
REALIZED(uint32_t);
REALIZED(uint64_t);
REALIZED(int16_t);
REALIZED(int32_t);
REALIZED(int64_t);
REALIZED(float);
REALIZED(double);

template <typename Elem>
struct GetRealizedT<builder::dyn_var<Elem>> { using Realized_T=Elem; };

template <typename Elem, int Rank, typename Idxs>
struct GetRealizedT<Ref<Block<Elem,Rank>,Idxs>> { using Realized_T=Elem; };

template <typename Elem, int Rank, typename Idxs>
struct GetRealizedT<Ref<View<Elem,Rank>,Idxs>> { using Realized_T=Elem; };

template <typename Functor, typename Operand0, typename Operand1>
struct GetRealizedT<Binary<Functor,Operand0,Operand1>> { 
  using Realized_T=typename Binary<Functor,Operand0,Operand1>::Realized_T;
};

template <char C>
struct GetRealizedT<Iter<C>> { using Realized_T=typename Iter<C>::Realized_T; };

template <typename T, typename Operand>
struct GetRealizedT<TemplateCast<T,Operand>> { using Realized_T=typename TemplateCast<T,Operand>::Realized_T; };

template <typename Functor, typename Operand0, typename Operand1>
struct Binary : public Expr<Binary<Functor, Operand0, Operand1>> {

  using Realized_T = typename GetRealizedT<Operand0>::Realized_T;

  Binary(const Operand0 &operand0, const Operand1 &operand1) :
    operand0(operand0), operand1(operand1) { 
    static_assert(std::is_same<
		  typename GetRealizedT<Operand0>::Realized_T,
		  typename GetRealizedT<Operand1>::Realized_T>());
  }

  template <typename LhsIdxs, typename Iters>
  builder::dyn_var<Realized_T> realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    if constexpr (std::is_fundamental<Operand0>::value && std::is_fundamental<Operand1>::value) {
      return Functor()(operand0, operand1);
    } else if constexpr (std::is_fundamental<Operand0>::value) {
      builder::dyn_var<Realized_T> op1 = dispatch_realize(operand1, lhs_idxs, iters);
      return Functor()(operand0, op1);
    } else if constexpr (std::is_fundamental<Operand1>::value) {
      builder::dyn_var<Realized_T> op0 = dispatch_realize(operand0, lhs_idxs, iters);
      return Functor()(op0, operand1);
    } else {
      builder::dyn_var<Realized_T> op0 = dispatch_realize(operand0, lhs_idxs, iters);
      builder::dyn_var<Realized_T> op1 = dispatch_realize(operand1, lhs_idxs, iters);
      return Functor()(op0, op1);
    }
  }

private:
  
  Operand0 operand0;
  Operand1 operand1;
  
};

template <char Ident>
struct Iter : public Expr<Iter<Ident>> {

  using Realized_T = loop_type;

  static constexpr char Ident_T = Ident;

  template <typename LhsIdxs, typename Iters>
  builder::dyn_var<loop_type> realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    // figure out the index of Ident within lhs_idxs so I can get the correct
    // iter
    constexpr int idx = find_iter_idx<LhsIdxs>();
    return std::get<idx>(iters);
  }

private:

  template <typename LhsIdxs>
  struct FindIterIdxProxy { };

  template <typename...LhsIdxs>
  struct FindIterIdxProxy<std::tuple<LhsIdxs...>> {
    constexpr int operator()() const { return FindIterIdx<0, LhsIdxs...>()(); }
  };
  
  template <int Depth, typename LhsIdx, typename...LhsIdxs>
  struct FindIterIdx {
    constexpr int operator()() const {
      if constexpr (sizeof...(LhsIdxs) == 0) {
	return -1; // not found
      } else {
	return FindIterIdx<Depth+1, LhsIdxs...>()();
      }
    }
  };

  template <int Depth, typename...LhsIdxs>
  struct FindIterIdx<Depth, Iter<Ident>, LhsIdxs...> {
    constexpr int operator()() const { return Depth; }
  };

  template <typename LhsIdxs>
  static constexpr int find_iter_idx() {
    constexpr int idx = FindIterIdxProxy<LhsIdxs>()();
    static_assert(idx != -1);
    return idx;
  }
  
};

}
