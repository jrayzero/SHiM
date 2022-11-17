#pragma once

namespace hmda {

template <typename Derived>
struct Expr;
template <typename Functor, typename Operand0, typename Operand1>
struct Binary;

template <typename Derived>
struct Expr {

  template <typename Rhs>
  Binary<AddFunctor, Derived, Rhs> operator+(const Rhs &rhs) {
    return Binary<AddFunctor, Derived, Rhs>(*static_cast<Derived*>(this), rhs);
  }
  
};

template <typename Lhs, typename Rhs, typename std::enable_if<std::is_arithmetic<Lhs>::value, int>::type = 0>
auto operator+(Lhs lhs, const Rhs &rhs) {
  return Binary<AddFunctor, Lhs, Rhs>(lhs, rhs);  
}

template <typename Functor, typename Operand0, typename Operand1>
struct Binary : public Expr<Binary<Functor, Operand0, Operand1>> {

  Binary(const Operand0 &operand0, const Operand1 &operand1) :
    operand0(operand0), operand1(operand1) { }

  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    if constexpr (std::is_arithmetic<Operand0>::value && std::is_arithmetic<Operand1>::value) {
      return Functor()(operand0, operand1);
    } else if constexpr (std::is_arithmetic<Operand0>::value) {
      auto op1 = operand1.realize(lhs_idxs, iters);
      return Functor()(operand0, op1);
    } else if constexpr (std::is_arithmetic<Operand1>::value) {
      auto op0 = operand0.realize(lhs_idxs, iters);
      return Functor()(op0, operand1);
    } else {
      auto op0 = operand0.realize(lhs_idxs, iters);
      auto op1 = operand1.realize(lhs_idxs, iters);
      return Functor()(op0, op1);
    }
  }
  
private:
  
  Operand0 operand0;
  Operand1 operand1;
  
};

template <char Ident>
struct Iter : public Expr<Iter<Ident>> {

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

template <char Ident, typename Functor>
struct Redux {

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

template <typename I>
struct IsReduxType {
  constexpr bool operator()() { return false; }
};

template <char Ident, typename Functor>
struct IsReduxType<Redux<Ident, Functor>> {
  constexpr bool operator()() { return true; }
};

template <typename T>
constexpr bool is_redux() {
  return IsReduxType<T>()();
}

}
