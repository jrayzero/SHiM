#pragma once

// forward decls

namespace hmda {

  namespace staged {

    template<typename Derived>
    struct Expr;
    template<typename BlockLike, typename Idxs>
    struct Ref;
    template<typename Functor, typename Operand0, typename Operand1>
    struct Binary;
    template<char Ident>
    struct Iter;
    template<typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
    struct Block;

  }

}
