#pragma once

#include "expr.h"
#include "base.h"

// These are loops for inline indexing ONLY.
// They don't apply for block/view decompostion loops

using namespace std;

namespace hmda {

// Here's how things go together 
// @N => DFSOrder N
// <N> => WhichStmt
// LoopDFSIter<N> which loop level iter to access
// Start<A,B> with Extent<C,D> => loop starts at Start A within Stmt B and goes for Extent C within Stmt D
// these read from the starts and extents tuples, which are fully realized values (i.e. they don't
// contain Iter types any more)
// We assume that any scheduling uses static values 
//
// a(i,j) = b(j) + i
//
// Unscheduled loop:
// @0 LoopLevel Extent<0,0> => would be a.extents[0]
//   @1 LoopLevel Extent<1,0> => would be 1 
//      Stmt <0> {LoopDFSIter<0>,LoopDFSIter<1>}
//
// Interchange the loops:
// @0 LoopLevel Extent<1,0>
//   @1 LoopLevel Extent<0,0>
//      Stmt <0> {LoopDFSIter<1>,LoopDFSIter<0>}
//
// Fission on the inner loop by two (ignoring any conditionals needed for determining if there
// are enough iterations).
// @0 LoopLevel Extent<1,0>
//   @1 LoopLevel 2 // the extent is a static value here
//      Stmt <0> {LoopDFSIter<1>,LoopDFSIter<0>}
//   @1 LoopLevel Extent<0,0>-2
//      Stmt <0> {LoopDFSIter<2>+2,LoopDFSIter<0>}
//
// Back to the interchange--now split the inner loop instead of fission (again ignore conditionals)
// @0 LoopLevel Extent<1,0>
//   @1 LoopLevel 2 
//     @2 LoopLevel Extent<0,0>/2
//        Stmt <0> {LoopDFSIter<1>*(floor(LoopDFSIter<2>/2))+LoopDFSIter<2>,LoopDFSIter<0>}
// Notice that we always keep a tuple of 2 loop iterators since that is what we need for the statement
// access, even though we now have a depth 3 loop.
//
// Now, let's see what the fission looks like with all the conditionals and such added in 
// Fission on the inner loop by two (ignoring any conditionals needed for determining if there
// are enough iterations).
// @0 LoopLevel Extent<1,0>
//   IfLevel Extent<0,0> >= 2
//     @1 LoopLevel 2
//        Stmt <0> {LoopDFSIter<1>,LoopDFSIter<0>}
//     @1 LoopLevel Extent<0,0>-2 // if Extent<0,0> == 2, this just won't execute
//        Stmt <0> {2+LoopDFSIter<3>,LoopDFSIter<0>}
// 
// And now the loop split with a cleanup loop
// @0 LoopLevel Extent<1,0>
//   @1 LoopLevel 2 
//     @2 LoopLevel Extent<0,0>/2
//        Stmt <0> {LoopDFSIter<1>*(floor(Extent<0,0>/2))+LoopDFSIter<2>,LoopDFSIter<0>}
//   @1 LoopLevel Extent<0,0> - 2*floor(Extent<0,0>/2)
//        Stmt <0> {LoopDFSIter<3>+2*floor(Extent<0,0>/2),LoopDFSIter<0>}
//
// Now something with a constant access
// 3, not an Iter
// a(i,3) = ...some value...
// @0 LoopLevel Extent<0,0>
//   @1 LoopLevel Extent<1,0> => would be 1
//     Stmt <0> {LoopDFSIter<0>,LoopDFSIter<1>+LhsOffset<1,0>} => Offset would be 3 (already realized) 

// References an extent at index WhichExtent within stmt WhichStmt in a to-be-specified StmtGroup
// The corresponding extent whould already be realized.
// Basically a placeholder.
template <int WhichExtent, int WhichStmt>
struct Extent : public Expr<Extent<WhichExtent, WhichStmt>> { 

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    for (int i = 0; i < indent; i++) ss << " ";
    ss << "Extent<" << WhichExtent << "," << WhichStmt << ">";
    return ss.str();
  }
  
  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static builder::dyn_var<loop_type> realize_from_types(const LoopIters &, const Extents &extents, const Offsets &, const Stmts &) {
    auto extent = get<WhichStmt>(extents);
    builder::dyn_var<loop_type> x = get<WhichExtent>(extent);
    return x;
  }

};

template <int WhichOffset, int WhichStmt>
struct Offset : public Expr<Offset<WhichOffset, WhichStmt>> {

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    for (int i = 0; i < indent; i++) ss << " ";
    ss << "Offset<" << WhichOffset << "," << WhichStmt << ">";
    return ss.str();
  }

  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static builder::dyn_var<loop_type> realize_from_types(const LoopIters &, const Extents &, const Offsets &offsets, Stmts) {
    auto offset = get<WhichStmt>(offsets);
    builder::dyn_var<loop_type> x = get<WhichOffset>(offset);
    return x;
  }
    
};

// References a particular LoopLevel at DFSOrder within a to-be-specified loop structure.
// Used to map the iterators of a scheduled loop back to the original loop iterators so you
// can access stmts correctly. Used with the LoopIterTransforms in StmtLevels.
// Basically a placeholder
template <int DFSOrder>
struct LoopDFSIter : public Expr<LoopDFSIter<DFSOrder>> { 

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    for (int i = 0; i < indent; i++) ss << " ";
    ss << "@" << DFSOrder;
    return ss.str();
  }

  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static builder::dyn_var<loop_type> realize_from_types(const LoopIters &iters, const Extents &, const Offsets &, Stmts) {      
    builder::dyn_var<loop_type> x = get<DFSOrder>(iters);
    return x;
  }

};

// An iteration level with a loop structure.
// This is an expression template that stores indices into the statement groups,
// iterator groups, etc. So it's a purely compile-time structure
// DFSOrder: the index of this LoopLevel within a DFS of the whole loop structure
// Nested: the next level nested within the current LoopLevel.
// Next: the next level after the current LoopLevel.
// ExtentExpr: An Extent or compound expr on Extent
template <int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct LoopLevel { 

  using self = LoopLevel<DFSOrder, Nested, Next, ExtentExpr>;

  static constexpr int DFSOrder_T = DFSOrder;
  using Nested_T = Nested;
  using Next_T = Next;
  using ExtentExpr_T = ExtentExpr;

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    for (int i = 0; i < indent; i++) ss << " ";
    ss << "@" << DFSOrder << " Loop " << ExtentExpr::dump(0) << endl;
    ss << Nested::dump(indent+2);
    ss << Next::dump(indent);
    return ss.str();
  }

  /*      template <typename Iters, typename Extents, typename LhsIdxs, typename Stmts>
	  static void realize(const Iters &iters,
	  const Extents &extents,
	  const LhsIdxs &lhs_idxs,
	  Stmts stmts) {*/
  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static void realize_from_types(const LoopIters &iters, const Extents &extents,
				 const Offsets &offset, Stmts stmts) {

    // determine the extent of this loop 
    builder::dyn_var<loop_type> extent = ExtentExpr::realize_from_types(iters, extents, offset, stmts);
    // make the loop
    builder::dyn_var<loop_type> i = 0;
    for (; i < extent; i=i+1) {
      Nested::realize_from_types(tuple_cat(iters, tuple{i}), extents, offset, stmts);
    }
    Next::realize_from_types(iters, extents, offset, stmts);      
  }
      
};

// A statement within a loop structure
// Next: the next level after this StmtLevel
// LoopIterTransforms: expressions representing the current state of the loop containing loop structur
// We have WhichStmt since it's possible to fuse different stmt loops together.
template <int WhichStmt, typename Next, typename...LoopIterExprs>
struct StmtLevel { 

  using self = StmtLevel<WhichStmt, Next, LoopIterExprs...>;

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    for (int i = 0; i < indent; i++) ss << " ";
    ss << "Stmt<" << WhichStmt << ">";
    dump<0, LoopIterExprs...>(ss);
    ss << endl;
    ss << Next::dump(indent);
    return ss.str();
  }

  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static void realize_from_types(const LoopIters &iters, const Extents &extents,
				 const Offsets &offsets, Stmts stmts) {
    // transform each of the LoopIterExprs to get an dyn_var on them
    auto xform_iters = transform_iters<LoopIters, Extents, Offsets, Stmts, LoopIterExprs...>(iters, extents, offsets, stmts);
    // and realize the actual stmt (which should be a LhsRhs wrapper)
    get<WhichStmt>(stmts).realize(xform_iters); // we don't need the other information here b/c all the evaluation needs are the correct iters
  }

  /*      template <typename...LoopIters, typename...Extents, typename...LhsAccessors, typename...Stmts>  
	  static void realize(const tuple<LoopIters...> &iters, 
	  const tuple<Extents...> &extents,
	  const tuple<LhsAccessors...> &acc,
	  tuple<Stmts...> &stmts) {
	  // transform the iters to map to the original loop structure (based on the Lhs!)
	  // here, sizeof(LoopIters) may not be equal to sizeof(LoopIterTransforms) since you can do things like loop split and such
	  // but we need to end up with sizeof(LoopIterTransforms) worth of transformed iterators
	  auto xform_iters = transform_iters<tuple<LoopIters...>, tuple<Extents...>, LoopIterTransforms...>(iters, extents);
	  // now realize the actual statement
	  get<WhichStmt>(stmts).realize(xform_iters, acc);
	  // and move on to the Next level
	  Next::realize(iters, extents, acc, stmts);
	  }*/

private:

  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts, typename LoopIterExpr, typename...LoopIterExprs2>
  static auto transform_iters(const LoopIters &iters, const Extents &extents, const Offsets &offsets, Stmts stmts) {
    builder::dyn_var<loop_type> x = LoopIterExpr::realize_from_types(iters, extents, offsets, stmts);
    auto iter = tuple{x};
    if constexpr (sizeof...(LoopIterExprs2) > 0) {
      return tuple_cat(iter, transform_iters<LoopIters, Extents, Offsets, Stmts, LoopIterExprs2...>(iters, extents,
												    offsets, stmts));
    } else {
      return iter;
    }
  }

  template <int Idx, typename A, typename...B>
  static void dump(stringstream &ss) {
    if constexpr (Idx > 0) ss << ",";
    ss << A::dump(0);
    if constexpr (sizeof...(B) > 0) dump<Idx+1, B...>(ss);
  }

};

// TODO realize function
// A conditional within a loop structure
// Condition: another expression template thing
// Nested: the next level nested within the current IfLevel
// Next: the next level after the current OfLevel.
template <typename Condition, typename Nested, typename Next>
struct IfLevel { 

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    for (int i = 0; i < indent; i++) ss << " ";
    ss << "if " << Condition::dump(0) << endl;
    ss << Nested::dump(indent+2);
    ss << Next::dump(indent);
    return ss.str();
  }

};

// A level that does nothing (essentially a sentinel).
// This kills the current path in the loop structure (i.e. there's no Next, Nested, etc)
struct NullLevel { 

  using self = NullLevel;

  // Print high-level structure
  static string dump(int indent) {
    return "";
  }

  // Does nothing
  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static void realize_from_types(const LoopIters &, const Extents &,
				 const Offsets &, const Stmts &) { }
        
};

// This just connects two things together sequentially
template <typename A, typename B>
struct TransientLevel {

  template <typename LoopIters, typename Extents, typename Offsets, typename Stmts>
  static void realize_from_types(const LoopIters &iters, const Extents &extents,
				 const Offsets &offsets, Stmts stmts) {
    A::realize_from_types(iters, extents, offsets, stmts);
    B::realize_from_types(iters, extents, offsets, stmts);    
  }

  // Print high-level structure
  static string dump(int indent) {
    stringstream ss;
    ss << A::dump(indent);
    ss << B::dump(indent);
    return ss.str();
  }
  
};

template <typename...MaybeTuples>
struct CheckAllTuples { 
  constexpr bool operator()() { return false; }
};

template <>
struct CheckAllTuples<> {
  constexpr bool operator()() { return true; }
};

template <typename...TupleElems, typename...MaybeTuples>
struct CheckAllTuples<tuple<TupleElems...>, MaybeTuples...> {
  constexpr bool operator()() {
    return CheckAllTuples<MaybeTuples...>()();
  }
};

template <typename...MaybeTuples>
constexpr bool check_all_tuples() {
  return CheckAllTuples<MaybeTuples...>()();
}

// API for dealing with a loop structure
template <typename Entry>
struct LoopStructure {

  using self = LoopStructure<Entry>;
  
  using entry = Entry;

  // Print high-level structure
  static string dump() {
    return Entry::dump(0);
  }
  
  // Apply a schedule and return a new loop schedule
  template <typename Sched, typename...Schedules>
  static auto apply_schedules() {
    auto sched_applied = Sched::apply_to(self());
    using applied = decltype(sched_applied);
    return LoopStructure<applied>();
  }

  // Run the things for real
  // Example: if have two statments in this loop nest
  // stmt0: lhs0[I][3] = 10
  // stmt1: lhs1[I][J] = 11,
  // the inputs would be
  // Extents: {{lhs0.extent[0],1},{lhs1.extent[0],lhs1.extent[1]}}
  // LhsIdxs: {{I,3}, {I,J}}
  // Stmts: {stmt0, stmt1}
  template <typename Extents, typename LhsIdxs, typename Stmts>
  static void realize(const Extents &extents,
		      const LhsIdxs &lhs_idxs,
		      Stmts stmts) {
    Entry::realize(tuple{} /*no iterators initially*/, extents, lhs_idxs, stmts);
  }

  template <typename Extents, typename Offsets, typename Stmts>
  static void realize_from_types(const Extents &extents, const Offsets &offsets, Stmts stmts) {
    Entry::realize_from_types(tuple{} /*no iters yet*/, extents, offsets, stmts);
  }  

};

}
