#pragma once

namespace cola {

// These functors are completely generic so they can work with fundamentals, exprs, and dyn_vars

///
/// Not
struct NotFunctor {
  template <typename T>
  bool operator()(const T op) {
    return !op;
  }
};

///
/// Bitwise invert
struct BitwiseInvertFunctor {
  template <typename T>
  T operator()(const T op) {
    return ~op;
  }
};

///
/// Negate
struct NegateFunctor {
  template <typename T>
  T operator()(const T op) {
    return -op;
  }
};

///
/// Addition
struct AddFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 + op1;
  }
};

///
/// Subtraction
struct SubFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 - op1;
  }
};

///
/// Multiplication
struct MulFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 * op1;
  }
};

///
/// Division
struct DivFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 / op1;
  }
};

///
/// Left shift
struct LShiftFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 << op1;
  }
};

///
/// Right shift
struct RShiftFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 >> op1;
  }
};

///
/// Less than
/// If Eq==true, then it's <=
template <bool Eq>
struct LTFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    if constexpr (Eq == true) {
      return op0 <= op1;
    } else {
      return op0 < op1;
    }
  }
};

///
/// Greater than
/// If Eq==true, then it's >=
template <bool Eq>
struct GTFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    if constexpr (Eq == true) {
      return op0 >= op1;
    } else {
      return op0 > op1;
    }
  }
};

///
/// Equality
/// If Not==true, then it's !=
template <bool Not>
struct EqFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    if constexpr (Not == true) {
      return op0 != op1;
    } else {
      return op0 == op1;
    }
  }
};

///
/// Boolean and
struct AndFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 && op1;
  }
};

/// 
/// Boolean or
struct OrFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 || op1;
  }
};

///
/// Bitwise and
struct BitwiseAndFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 & op1;
  }
};

/// 
/// Bitwise or
struct BitwiseOrFunctor {
  template <typename T, typename U>
  auto operator()(const T op0, const U op1) {
    return op0 | op1;
  }
};

}
