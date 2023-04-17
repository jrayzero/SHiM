#pragma once

namespace shim {

template <typename Elem, unsigned long Rank, bool MultiDimRepr=false, bool NonStandardAlloc=false>
struct Block;

template <typename Elem, unsigned long Rank, bool MultiDimRepr=false, bool NonStandardAlloc=false>
struct View;

template <typename BlockLike, typename Idxs> 
struct Ref;

// Expressions and expression-like things
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
template <typename To, typename Operand>
struct TemplateCast;
template <typename Cond, typename TBranch, typename FBranch>
struct TernaryCond;
template <int GOPType, typename...Options>
struct GroupedJunctionOperator;


}
