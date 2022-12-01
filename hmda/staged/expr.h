#pragma once

#include <type_traits>
#include "builder/dyn_var.h"
#include "functors.h"
#include "staged_utils.h"

namespace hmda {

template <typename Derived>
struct Expr;
template <typename Functor, typename Operand0>
struct Unary;
template <typename Functor, typename Operand0, typename Operand1>
struct Binary;
template <char Ident>
struct Iter;
template <typename BlockLIke, typename Idxs>
struct Ref;

template <typename T>
struct IsExpr {
  constexpr bool operator()() { return false; }
};

template <typename BlockLike, typename Idxs>
struct IsExpr<Ref<BlockLike,Idxs>> {
  constexpr bool operator()() { return true; }
};


template <typename Functor, typename Operand>
struct IsExpr<Unary<Functor,Operand>> {
  constexpr bool operator()() { return true; }
};

template <typename Functor, typename Operand0, typename Operand1>
struct IsExpr<Binary<Functor,Operand0,Operand1>> {
  constexpr bool operator()() { return true; }
};

template <char Ident>
struct IsExpr<Iter<Ident>> {
  constexpr bool operator()() { return true; }
};

template <typename T>
constexpr bool is_expr() {
  return IsExpr<T>()();
}

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
	    typename std::enable_if<is_expr<Rhs>(), int>::type = 0>	\
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
  if constexpr (IsExpr<T>()()) {
    return to_realize.realize(lhs_idxs, iters);
  } else {
    // something primitive-y, like a loop_type or another builder
    return to_realize;
  }
}

template <typename Functor, typename Operand0, typename Operand1>
struct Binary : public Expr<Binary<Functor, Operand0, Operand1>> {

  Binary(const Operand0 &operand0, const Operand1 &operand1) :
    operand0(operand0), operand1(operand1) { }

  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    if constexpr (std::is_fundamental<Operand0>::value && std::is_fundamental<Operand1>::value) {
      return Functor()(operand0, operand1);
    } else if constexpr (std::is_fundamental<Operand0>::value) {
      auto op1 = dispatch_realize(operand1, lhs_idxs, iters);
      return Functor()(operand0, op1);
    } else if constexpr (std::is_fundamental<Operand1>::value) {
      auto op0 = dispatch_realize(operand0, lhs_idxs, iters);
      return Functor()(op0, operand1);
    } else {
      auto op0 = dispatch_realize(operand0, lhs_idxs, iters);
      auto op1 = dispatch_realize(operand1, lhs_idxs, iters);
      return Functor()(op0, op1);
    }
  }

  // for use with loops
  template <typename...LoopIters, typename...StmtExtents>
  static builder::dyn_var<loop_type> realize_from_types(const std::tuple<LoopIters...> &iters, const std::tuple<StmtExtents...> &stmt_extents) {
    auto op0 = Operand0::realize_from_types(iters, stmt_extents);
    auto op1 = Operand0::realize_from_types(iters, stmt_extents);
    return Functor()(op0, op1);
  }

  
private:
  
  Operand0 operand0;
  Operand1 operand1;
  
};

template <char Ident>
struct Iter : public Expr<Iter<Ident>> {

  static constexpr char Ident_T = Ident;

  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
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

template <typename I>
struct IsIterType {
  constexpr bool operator()() { return false; }
};

template <char Ident>
struct IsIterType<Iter<Ident>> {
  constexpr bool operator()() { return true; }
};

template <typename T>
constexpr bool is_iter() {
  return IsIterType<T>()();
}

}
