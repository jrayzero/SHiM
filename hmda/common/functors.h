#pragma once

namespace hmda {

// These are inline functors (i.e. a+b, not +(a,b))
#define UNARY_FUNCTOR(name, op)			\
  struct name##Functor {			\
    template <typename L>			\
    auto operator()(const L &l) {		\
      return op l;				\
    }						\
  };

#define BINARY_FUNCTOR(name, op)		\
  struct name##Functor {			\
    template <typename L, typename R>		\
    auto operator()(const L &l, const R &r) {	\
      return l op r;				\
    }						\
  };

BINARY_FUNCTOR(Add, +);
BINARY_FUNCTOR(Sub, -);
BINARY_FUNCTOR(Mul, *);
BINARY_FUNCTOR(Div, /);
BINARY_FUNCTOR(RShift, >>);
BINARY_FUNCTOR(LShift, <<);
BINARY_FUNCTOR(LT, <);
BINARY_FUNCTOR(LTE, <=);
BINARY_FUNCTOR(GT, >);
BINARY_FUNCTOR(GTE, >=);
BINARY_FUNCTOR(Eq, ==);
BINARY_FUNCTOR(NEq, !=);
BINARY_FUNCTOR(And, &&);
BINARY_FUNCTOR(Or, ||);
BINARY_FUNCTOR(BAnd, &);
BINARY_FUNCTOR(BOr, |);
UNARY_FUNCTOR(Not, !);
UNARY_FUNCTOR(BNot, ~);
UNARY_FUNCTOR(Invert, -);

}
