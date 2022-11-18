#pragma once

#include "loops.h"

namespace hmda {

namespace schedule {

template <typename Loop, typename Stmts>
struct SchedulableHandle {

  static constexpr int nstmts = std::tuple_size<Stmts>();

  using Loop_T = Loop;

  // Should be tuple of LhsRhs objects (or potentially just rhs if introduce exprstmts)
  Stmts stmts;

  explicit SchedulableHandle(Stmts stmts) : stmts(stmts) { }

  // this applies a loop-based scheduled, so the stmts stay the same
  template <typename Schedule>
  auto apply() {
    using new_loop = decltype(Schedule::template apply_to<Loop>());
    return SchedulableHandle<new_loop, Stmts>(stmts);
  }

  auto eval() {
    // first, we need to gather all the pieces we need for executing the loop
    auto extents = get_extents();
    auto offsets = get_offsets();
    // now we can run the loop
    // sanity check
    static_assert(std::tuple_size<decltype(extents)>() == nstmts);    
    static_assert(std::tuple_size<decltype(offsets)>() == nstmts);
    Loop::realize_from_types(extents, offsets, stmts);    
  }

private:

  // will generate a tuple of tuples 
  template <int Depth=0>
  auto get_extents() {
    if constexpr (Depth == nstmts) {
      return std::tuple{};
    } else {
      auto stmt = std::get<Depth>(stmts);
      if constexpr (is_any_block<decltype(stmt.lhs.block_like)>()) {
	auto extents = stmt.lhs.template get_extents<0>(stmt.lhs.block_like.bextents);
	std::tuple<decltype(extents)> textents = std::tuple{extents};
	return std::tuple_cat(textents, get_extents<Depth+1>());
      } else {
	auto extents = stmt.lhs.template get_extents<0>(stmt.lhs.block_like.vextents);
	std::tuple<decltype(extents)> textents = std::tuple{extents};	
	return std::tuple_cat(textents, get_extents<Depth+1>());
      }
    }
  }

  // will generate a tuple of tuples
  template <int Depth=0>
  auto get_offsets() {
    if constexpr (Depth == nstmts) {
      return std::tuple{};
    } else {
      auto stmt = std::get<Depth>(stmts);
      auto offsets = stmt.lhs.template get_offsets<0>();
      std::tuple<decltype(offsets)> toffsets = std::tuple{offsets};
      return std::tuple_cat(toffsets, get_offsets<Depth+1>());
    }
  }  
  
};

  
struct NullSchedule {

  // a pure type
  NullSchedule() = delete;
  
  template <typename Loop>
  static auto apply_to() {
    return Loop(); // does nothing
  }
  
};

template <typename MaybeLoopLevel>
struct IsLoopLevel {
  constexpr bool operator()() { return false; }
};

template <int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct IsLoopLevel<LoopLevel<DFSOrder, Nested, Next, ExtentExpr>> {
  constexpr bool operator()() { return true; }
};

template <typename MaybeLoopLevel>
constexpr bool is_loop_level() {
  return IsLoopLevel<MaybeLoopLevel>()();
}

template <typename A>
struct IsNullLevel {
  constexpr bool operator()() { return false; }
};

template <>
struct IsNullLevel<NullLevel> {
  constexpr bool operator()() { return true; }
};

template <typename A>
constexpr bool is_null_level() {
  return IsNullLevel<A>()();
}

template <typename Loop>
struct IsPerfectlyNested { };

template <>
struct IsPerfectlyNested<NullLevel> {
  constexpr bool operator()() { return true; }
};

template <typename A, typename B>
struct IsPerfectlyNested<TransientLevel<A,B>> {
  constexpr bool operator()() {
    return IsPerfectlyNested<A>()() && IsPerfectlyNested<B>()();
  }
};

template <int Which, typename Next, typename...Exprs>
struct IsPerfectlyNested<StmtLevel<Which,Next,Exprs...>> {
  constexpr bool operator()() {
    return IsPerfectlyNested<Next>()();
  }
};

template <int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct IsPerfectlyNested<LoopLevel<DFSOrder, Nested, Next, ExtentExpr>> {
  constexpr bool operator()() {
    if constexpr (!is_null_level<Next>()) {
      return false;
    } else {
      return IsPerfectlyNested<Nested>()();
    }
  }
};

template <typename Loop>
constexpr bool is_perfectly_nested() {
  return IsPerfectlyNested<Loop>()();
}

// Get the max WhichStmt in LoopStructure.
template <typename A>
struct GetMaxWhichStmt { };

template <int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct GetMaxWhichStmt<LoopLevel<DFSOrder, Nested, Next, ExtentExpr>> {

  constexpr int operator()() {
    constexpr int a = GetMaxWhichStmt<Nested>()();
    constexpr int b = GetMaxWhichStmt<Next>()();
    constexpr int c = GetMaxWhichStmt<ExtentExpr>()();
    if constexpr (a > b) {
      if constexpr (a > c) return a;
      else return c;
    }
    else {
      if constexpr (b > c) return b;
      else return c;
    }
  }
  
};

template <typename A, typename B>
struct GetMaxWhichStmt<TransientLevel<A, B>> {

  constexpr int operator()() {
    constexpr int a = GetMaxWhichStmt<A>()();
    constexpr int b = GetMaxWhichStmt<B>()();
    if constexpr (a > b) return a;
    else return b;
  }
  
};

template <int WhichStmt, typename Next, typename...LoopIterExprs>
struct GetMaxWhichStmt<StmtLevel<WhichStmt, Next, LoopIterExprs...>> {

  template <typename A, typename...As>
  static constexpr int process() {
    constexpr int a = GetMaxWhichStmt<A>()();
    if constexpr (sizeof...(As) > 0) {
      constexpr int b = process<As...>();
      if constexpr (a > b) return a;
      else return b;
    } else {
      return a;
    }
  }
  
  constexpr int operator()() {
    constexpr int a = GetMaxWhichStmt<Next>()();
    constexpr int b = process<LoopIterExprs...>();
    if constexpr (a > b) {
      if constexpr (a > WhichStmt) return a;
      else return WhichStmt;
    } else {
      if constexpr (b > WhichStmt) return b;
      else return WhichStmt;
    }
  }
  
};

template <int WhichExtent, int WhichStmt>
struct GetMaxWhichStmt<Extent<WhichExtent, WhichStmt>> {
  constexpr int operator()() { return WhichStmt; }
};

template <int WhichExtent, int WhichStmt>
struct GetMaxWhichStmt<Offset<WhichExtent, WhichStmt>> {
  constexpr int operator()() { return WhichStmt; }
};

template <int DFSOrder>
struct GetMaxWhichStmt<LoopDFSIter<DFSOrder>> {
  constexpr int operator()() { return 0; }
};

template <>
struct GetMaxWhichStmt<NullLevel> {
  constexpr int operator()() { return 0; }
};

template <typename A>
constexpr int get_max_which_stmt() {
  return GetMaxWhichStmt<A>()();
}

// Update WhichStmt values
template <int Offset, typename A>
struct OffsetWhichStmt { };

template <int Offset, int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct OffsetWhichStmt<Offset, LoopLevel<DFSOrder, Nested, Next, ExtentExpr>> {

  auto operator()() {
    using Nested2 = decltype(OffsetWhichStmt<Offset, Nested>()());
    using Next2 = decltype(OffsetWhichStmt<Offset, Next>()());
    using ExtentExpr2 = decltype(OffsetWhichStmt<Offset, ExtentExpr>()());        
    return LoopLevel<DFSOrder, Nested2, Next2, ExtentExpr2>();
  }
  
};

template <int Offset, typename A, typename B>
struct OffsetWhichStmt<Offset, TransientLevel<A, B>> {

  auto operator()() {
    using A2 = decltype(OffsetWhichStmt<Offset,A>()());
    using B2 = decltype(OffsetWhichStmt<Offset,B>()());
    return TransientLevel<A2, B2>();
  }
  
};

template <int Offset, int WhichStmt, typename Next, typename...LoopIterExprs>
struct OffsetWhichStmt<Offset, StmtLevel<WhichStmt, Next, LoopIterExprs...>> {

  template <typename A, typename...As>
  static auto process() {
    auto A2 = OffsetWhichStmt<Offset, A>()();
    if constexpr (sizeof...(As) > 0) {
      return std::tuple_cat(std::tuple{A2}, process<As...>());
    } else {
      return std::tuple{A2};
    }
  }

  template <int WhichStmt2, typename Next2, typename...Ts>
  static auto merge(std::tuple<Ts...>) {
    return StmtLevel<WhichStmt2, Next2, Ts...>();
  }
  
  auto operator()() {
    constexpr int WhichStmt2 = WhichStmt + Offset;
    using Next2 = decltype(OffsetWhichStmt<Offset, Next>()());
    auto tup = process<LoopIterExprs...>();
    return merge<WhichStmt2, Next2>(tup);
  }
  
};

template <int Offset, int WhichExtent, int WhichStmt>
struct OffsetWhichStmt<Offset, Extent<WhichExtent, WhichStmt>> {
  auto operator()() { return Extent<WhichExtent, WhichStmt+Offset>(); }
};

template <int OffsetZ, int WhichExtent, int WhichStmt>
struct OffsetWhichStmt<OffsetZ, Offset<WhichExtent, WhichStmt>> {
  auto operator()() { return Offset<WhichExtent, WhichStmt+OffsetZ>(); }  
};

template <int Offset, int DFSOrder>
struct OffsetWhichStmt<Offset, LoopDFSIter<DFSOrder>> {
  auto operator()() { return LoopDFSIter<DFSOrder>(); }
};

template <int Offset>
struct OffsetWhichStmt<Offset, NullLevel> {
  auto operator()() { return NullLevel(); }
};

template <int Offset, typename Loop>
auto offset_which_stmt() {
  return OffsetWhichStmt<Offset,Loop>()();
}

// get the max dfs order
template <typename A>
struct GetMaxDFSOrder { };

template <int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct GetMaxDFSOrder<LoopLevel<DFSOrder, Nested, Next, ExtentExpr>> {

  template <int A, int B>
  static constexpr int m() {
    if constexpr (A > B) return A;
    return B;
  }
  
  constexpr int operator()() {
    constexpr int a = GetMaxDFSOrder<Nested>()();
    constexpr int b = GetMaxDFSOrder<Next>()();
    constexpr int c = GetMaxDFSOrder<ExtentExpr>()();
    constexpr int m0 = m<a,b>();
    constexpr int m1 = m<m0,c>();
    constexpr int m2 = m<m1,DFSOrder>();
    return m2;
  }
  
};

template <typename A, typename B>
struct GetMaxDFSOrder<TransientLevel<A, B>> {

  constexpr int operator()() {
    constexpr int a = GetMaxDFSOrder<A>()();
    constexpr int b = GetMaxDFSOrder<B>()();
    if constexpr (a > b) return a;
    else return b;
  }
  
};

template <int WhichStmt, typename Next, typename...LoopIterExprs>
struct GetMaxDFSOrder<StmtLevel<WhichStmt, Next, LoopIterExprs...>> {

  template <typename A, typename...As>
  static constexpr int process() {
    constexpr int a = GetMaxDFSOrder<A>()();
    if constexpr (sizeof...(As) > 0) {
      constexpr int b = process<As...>();
      if constexpr (a > b) return a;
      else return b;
    } else {
      return a;
    }
  }
  
  constexpr int operator()() {
    constexpr int a = GetMaxDFSOrder<Next>()();
    constexpr int b = process<LoopIterExprs...>();
    if constexpr (a > b) {
      return a;
    } else {
      return b;
    }
  }
  
};

template <int WhichExtent, int WhichStmt>
struct GetMaxDFSOrder<Extent<WhichExtent, WhichStmt>> {
  constexpr int operator()() { return 0; }
};

template <int WhichExtent, int WhichStmt>
struct GetMaxDFSOrder<Offset<WhichExtent, WhichStmt>> {
  constexpr int operator()() { return 0; }
};

template <int DFSOrder>
struct GetMaxDFSOrder<LoopDFSIter<DFSOrder>> {
  constexpr int operator()() { return 0; } // don't actually care about this one b/c we get it from the looplevel
};

template <>
struct GetMaxDFSOrder<NullLevel> {
  constexpr int operator()() { return 0; }
};

template <typename A>
constexpr int get_max_dfs_order() {
  return GetMaxDFSOrder<A>()();
}

// Update DFSOrder values
template <int Offset, typename A>
struct OffsetDFSOrder { };

template <int Offset, int DFSOrder, typename Nested, typename Next, typename ExtentExpr>
struct OffsetDFSOrder<Offset, LoopLevel<DFSOrder, Nested, Next, ExtentExpr>> {

  auto operator()() {
    using Nested2 = decltype(OffsetDFSOrder<Offset, Nested>()());
    using Next2 = decltype(OffsetDFSOrder<Offset, Next>()());
    using ExtentExpr2 = decltype(OffsetDFSOrder<Offset, ExtentExpr>()());        
    return LoopLevel<DFSOrder+Offset, Nested2, Next2, ExtentExpr2>();
  }
  
};

template <int Offset, typename A, typename B>
struct OffsetDFSOrder<Offset, TransientLevel<A, B>> {

  auto operator()() {
    using A2 = decltype(OffsetDFSOrder<Offset,A>()());
    using B2 = decltype(OffsetDFSOrder<Offset,B>()());
    return TransientLevel<A2, B2>();
  }
  
};

template <int Offset, int WhichStmt, typename Next, typename...LoopIterExprs>
struct OffsetDFSOrder<Offset, StmtLevel<WhichStmt, Next, LoopIterExprs...>> {

  template <typename A, typename...As>
  static auto process() {
    auto A2 = OffsetDFSOrder<Offset, A>()();
    if constexpr (sizeof...(As) > 0) {
      return std::tuple_cat(std::tuple{A2}, process<As...>());
    } else {
      return std::tuple{A2};
    }
  }

  template <int WhichStmt2, typename Next2, typename...Ts>
  static auto merge(std::tuple<Ts...>) {
    return StmtLevel<WhichStmt2, Next2, Ts...>();
  }
  
  auto operator()() {
    using Next2 = decltype(OffsetDFSOrder<Offset, Next>()());
    auto tup = process<LoopIterExprs...>();
    return merge<WhichStmt, Next2>(tup);
  }
  
};

template <int Offset, int WhichExtent, int WhichStmt>
struct OffsetDFSOrder<Offset, Extent<WhichExtent, WhichStmt>> {
  auto operator()() { return Extent<WhichExtent, WhichStmt>(); }
};

template <int OffsetZ, int WhichExtent, int WhichStmt>
struct OffsetDFSOrder<OffsetZ, Offset<WhichExtent, WhichStmt>> {
  auto operator()() { return Offset<WhichExtent, WhichStmt>(); }  
};

template <int Offset, int DFSOrder>
struct OffsetDFSOrder<Offset, LoopDFSIter<DFSOrder>> {
  auto operator()() {
    // If the specified DFSOrder==0, that means it is referencing the original
    // outer loop nest, so don't update that!
    if constexpr (DFSOrder == 0) {
      return LoopDFSIter<DFSOrder>();
    } else {
      return LoopDFSIter<DFSOrder+Offset>();
    }
  }
};

template <int Offset>
struct OffsetDFSOrder<Offset, NullLevel> {
  auto operator()() { return NullLevel(); }
};

template <int Offset, typename Loop>
auto offset_dfs_order() {
  return OffsetDFSOrder<Offset,Loop>()();
}

// This currently just fuses the outer level. Just want an example of how to try it
// Superrrr hacky
struct NaiveFuseSchedule {

  // a pure type
  NaiveFuseSchedule() = delete;

  template <typename LoopA, typename LoopB>
  static auto apply_to(const LoopStructure<LoopA> &loopA, const LoopStructure<LoopB> &loopB) {
    static_assert(is_loop_level<LoopA>() && is_loop_level<LoopB>(), "NaiveFuseSchedule requires LoopStructure entry to be a LoopLevel!");
    static_assert(is_perfectly_nested<LoopA>() && is_perfectly_nested<LoopB>(), "NaiveFuseSchedule requires perfectly nested loops!");
    // we will use LoopA's outer loop
    // get the components of each
    using LoopA_outer = LoopA;
    using LoopA_body = typename LoopA::Nested_T;
    using LoopB_body = typename LoopB::Nested_T;
    // update any thing with WhichStmt in LoopB to increment by the number of unique statements in LoopA
    constexpr int max_which = get_max_which_stmt<LoopA>();
    constexpr int offset = max_which + 1; // add this to each of the WhichStmts in LoopB_body
    // update the whichstmt
    using LoopB_body_2 = decltype(offset_which_stmt<offset,LoopB_body>());
    // do a similar thing witht the DFSOrder
/*    constexpr int max_dfs_order = get_max_dfs_order<LoopA>();
    constexpr int dfs_offset = max_dfs_order; // we don't add 1 here b/c we need to account for the fact that we peeled off the first iter, which would've had dfs order 0
    using LoopB_body_3 = decltype(offset_dfs_order<dfs_offset,LoopB_body_2>());    */
    // TOOD (need to count stmts in LoopA)
    // connect the bodies of each;
    auto bodies = TransientLevel<LoopA_body, LoopB_body_2>();
    // remake the outer loop and wrap in the loop structure
    return LoopStructure<LoopLevel<LoopA_outer::DFSOrder_T,
				   decltype(bodies),
				   NullLevel /*b/c perfectly nested*/,
				   typename LoopA_outer::ExtentExpr_T>>();
  }
 
};

template <typename HandleA, typename HandleB>
auto outer_fuse(HandleA &handleA, HandleB &handleB) {
  // combine the stmts
  auto stmts = std::tuple_cat(handleA.stmts, handleB.stmts);  
  // transform the loop
  auto loop = NaiveFuseSchedule::apply_to(typename HandleA::Loop_T(), typename HandleB::Loop_T());
  // make a new handle
  return SchedulableHandle<decltype(loop),decltype(stmts)>(std::move(stmts));
}

}

}
