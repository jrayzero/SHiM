#pragma once

#include <sstream>

#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "builder/dyn_var.h"

#include "base.h"
#include "loops.h"
#include "schedule.h"

namespace hmda {

// Notes
// - realize functions are not const for staged since they may lead to a call to Block::operator(), which
// would update the dyn_var for data when you return it (b/c it needs to record that dyn_var does something)
//   - this is also why the Rhs arguments are not const

template <typename BlockLike, typename Idxs>
struct BlockLikeRef;

template <typename Elem, int Rank>
struct SView;

// Staged version
template <typename Elem, int Rank>
struct SBlock : public BaseBlockLike<Elem,Rank> {

  template <typename BlockLike, typename Idxs>
  friend struct BlockLikeRef;

  using arr_type = std::array<loop_type,Rank>;

  builder::dyn_var<Elem*> data;

  SBlock(const arr_type &bextents,
	 const arr_type &bstrides,
	 const arr_type &borigin,
	 const builder::dyn_var<Elem*> &data) :
    BaseBlockLike<Elem,Rank>(bextents, bstrides, borigin), data(data) { }
  
  // Use the square brackets when you want inline indexing
  template <typename Idx>
  auto operator[](Idx idx);
  
  // TODO make this private and have block ref be a friend
  template <typename Rhs, typename...Iters>
  void assign(Rhs rhs, Iters...iters);

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "SBlock<" << Rank << ">" << std::endl;
    ss << "  Extents: " << join(this->bextents) << std::endl;
    ss << "  Strides: " << join(this->bstrides) << std::endl;
    ss << "  Origin:  " << join(this->borigin) << std::endl;
    return ss.str();
  }

private:
  
  // just for BlockLikeRef
  template <typename...Iters>
  builder::dyn_var<Elem> operator()(const std::tuple<Iters...> &);

  // just for BlockLikeRef
  template <typename...Iters>
  builder::dyn_var<Elem> operator()(Iters...iters);

};

// Staged version
template <typename Elem, int Rank>
struct SView : public BaseView<Elem, Rank> {

  template <typename BlockLike, typename Idxs>
  friend struct BlockLikeRef;

  using arr_type = std::array<loop_type,Rank>;
  
  builder::dyn_var<Elem*> data;

  SView(const arr_type &bextents,
	const arr_type &bstrides,
	const arr_type &borigin,
	const arr_type &vextents,
	const arr_type &vstrides,
	const arr_type &vorigin,
	const builder::dyn_var<Elem*> data) :
    BaseView<Elem, Rank>(bextents, bstrides, borigin,
			 vextents, vstrides, vorigin),
    data(data) { }
  
  // Use the square brackets when you want inline indexing
  template <typename Idx>
  auto operator[](Idx idx);

  // TODO make this private and have block ref be a friend
  template <typename Rhs, typename...Iters>
  void assign(Rhs rhs, Iters...iters);

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "SView<" << Rank << ">" << std::endl;
    ss << "  VExtents: " << join(this->vextents) << std::endl;
    ss << "  VStrides: " << join(this->vstrides) << std::endl;
    ss << "  VOrigin:  " << join(this->vorigin) << std::endl;
    ss << "  BExtents: " << join(this->bextents) << std::endl;
    ss << "  BStrides: " << join(this->bstrides) << std::endl;
    ss << "  BOrigin:  " << join(this->borigin) << std::endl; 
    return ss.str();
  }

private:

  // just for BlockLikeRef
  template <typename...Iters>
  builder::dyn_var<Elem> operator()(const std::tuple<Iters...> &);

  // just for BlockLikeRef
  template <typename...Iters>
  builder::dyn_var<Elem> operator()(Iters...iters);


};

template <typename Lhs, typename Rhs>
struct LhsRhs {

  // lhs should be a ref and rhs some type of Expr!
  LhsRhs(Lhs lhs, const Rhs &rhs) : lhs(lhs), rhs(rhs) { }

  template <typename Iters>
  void realize(const Iters &iters) {
    // need lhs.idxs incase the rhs contains any Iter types.
    auto realized_rhs = dispatch_realize(rhs, lhs.idxs, iters);
    // do the write!
    lhs.block_like.assign(realized_rhs, iters);
  }

  Lhs lhs;
  Rhs rhs;
  
};

// Expression template for lazy eval of inline functions
template <typename BlockLike, typename Idxs>
struct BlockLikeRef : public Expr<BlockLikeRef<BlockLike,Idxs>> {

  template <typename Loop, typename Stmts>
  friend struct schedule::SchedulableHandle;
  
  // Underlying block/view used to create this BlockLikeRef
  BlockLike block_like;
  // Indices used to define this BlockLikeRef
  Idxs idxs;

  BlockLikeRef(BlockLike block_like, Idxs idxs) :
    block_like(std::move(block_like)), idxs(std::move(idxs)) { }

  template <typename Idx>
  auto operator[](Idx idx);

  void operator=(typename BlockLike::elem x) {
    // TODO if we have a Block = Block2, we can just do a memset on the whole thing, or if 
    // we statically know that there is a stride of 1
    auto handle = lazy(x);
    handle.eval();
  }

  // some type of expression
  template <typename Rhs>
  void operator=(Rhs rhs) {
    auto handle = lazy(rhs);
    handle.eval();    
  }

  // This takes in the lhs_idxs of the containing LHS. We need this in
  // case the rhs contains any references to those iters
  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    // realize the individual indices and then use those to access block_like
    auto ridxs = realize_idxs<0>(lhs_idxs, iters);
    return block_like(ridxs);
  }

  template <typename Rhs>
  auto lazy(Rhs rhs) {
    // build the loop structure
    auto loop = build_loop_structure<BlockLike::rank>();
    using Loop = decltype(loop);
    auto stmt = std::tuple{LhsRhs(*this, rhs)};
    return schedule::SchedulableHandle<Loop, decltype(stmt)>(stmt);
  }

private:
  
  // build a loop and immeditately run (no scheduling)
  /*  template <typename Rhs>
  void basic_assign_loop(Rhs rhs) {
    auto loop = build_loop_structure<BlockLike::rank>();
    using Loop = decltype(loop);
    // wrap the statments
    auto stmts = tuple{LhsRhs(*this, rhs)};

    // we need to realize both the extents and the offsets for the stmt
    // for the extents, go through idxs and check if its an integral or
    // an Iter. If integral, the extent is one. If Iter, it's the extent
    // of the corresponding dimension.

    // Then, we need to compute offsets. This happens in a similar way
    // to getting the extents. If the idxs is integral, the offset is
    // the idx. If it's an Iter, the offset is 0.
  
    if constexpr (is_any_block<BlockLike>()) {
      // use bextents
      auto extents0 = get_extents<0>(block_like.bextents);
      auto extents1 = tuple<decltype(extents0)>{extents0};
      auto offsets0 = get_offsets<0>();
      auto offsets1 = tuple<decltype(offsets0)>{offsets0};      
      Loop::realize_from_types(extents1, offsets1, stmts);
    } else {
      // it's a view, use vextents
      auto extents0 = get_extents<0>(block_like.vextents);
      auto extents1 = tuple<decltype(extents0)>{extents0};
      auto offsets0 = get_offsets<0>();
      auto offsets1 = tuple<decltype(offsets0)>{offsets0};      
      Loop::realize_from_types(extents1, offsets1, stmts);
    }
    
  }*/

  template <int Depth, typename Extents>
  auto get_extents(const Extents &extents) {
    if constexpr (Depth == BlockLike::rank) {
      return std::tuple{};
    } else {
      if constexpr (std::is_integral<decltype(std::get<Depth>(idxs))>::value) {
	// extent is 1
	// TODO static var?
	builder::dyn_var<loop_type> x = 1;
	return std::tuple_cat(std::tuple{x}, get_extents<Depth+1>(extents));
      } else {
	// extent is whatever is in extents
	builder::dyn_var<loop_type> x = std::get<Depth>(extents);
	return std::tuple_cat(std::tuple{x}, get_extents<Depth+1>(extents));	
      }
    }
  }

  template <int Depth>
  auto get_offsets() {
    if constexpr (Depth == BlockLike::rank) {
      return std::tuple{};
    } else {
      if constexpr (std::is_integral<decltype(std::get<Depth>(idxs))>::value) {
	// offset is whatever the idx is
	// TODO static var?
	builder::dyn_var<loop_type> x = std::get<Depth>(idxs);
	return std::tuple_cat(std::tuple{x}, get_offsets<Depth+1>());
      } else {
	// offset is 0
	builder::dyn_var<loop_type> x = 0;
	return std::tuple_cat(std::tuple{x}, get_offsets<Depth+1>());	
      }
    }
  }

  template <int Depth, int MaxDepth, typename...DFSIters>
  auto build_stmt_level() {
    if constexpr (Depth == MaxDepth) {
      // last level
      return StmtLevel<0, NullLevel, DFSIters..., LoopDFSIter<Depth>>();
    } else {
      return build_stmt_level<Depth+1, MaxDepth, DFSIters..., LoopDFSIter<Depth>>();
    }
  }

  template <int Depth, int NLevels>
  auto build_loop_structure() {
    if constexpr (Depth == NLevels - 1) {
      // we are at the innermost level
      // build the stmt
      auto level = build_stmt_level<0, Depth>();
      // then wrap it in a loop level
      return LoopLevel<Depth, decltype(level), NullLevel, Extent<Depth, 0>>();
    } else {
      // another loop level   
      auto level = build_loop_structure<Depth+1, NLevels>();
      return LoopLevel<Depth, decltype(level), NullLevel, Extent<Depth, 0>>();
    }
  }

  template <int NLevels>
  auto build_loop_structure() {
    auto loop = build_loop_structure<0, NLevels>();
    return LoopStructure<decltype(loop)>();
  }

  
  template <int Depth, typename LhsIdxs, typename Iters>
  auto realize_idxs(const LhsIdxs &lhs_idxs, const Iters &iters) {
    if constexpr (Depth == BlockLike::rank) {
      return std::tuple{};
    } else {
      auto my_idx = std::get<Depth>(idxs);
      if constexpr (std::is_arithmetic<decltype(my_idx)>::value) {
	return std::tuple_cat(std::tuple{my_idx}, realize_idxs<Depth+1>(lhs_idxs, iters));
      } else {
	// realize this individual idx
	builder::dyn_var<loop_type> realized = my_idx.realize(lhs_idxs, iters);
	return std::tuple_cat(std::tuple{realized}, realize_idxs<Depth+1>(lhs_idxs, iters));	
      }
    }
  }

};

template <typename Elem, int Rank>
template <typename Idx>
auto SBlock<Elem,Rank>::operator[](Idx idx) {
  BlockLikeRef<SBlock<Elem, Rank>, std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank>
template <typename Rhs, typename...Iters>
void SBlock<Elem,Rank>::assign(Rhs rhs, Iters...iters) {
  builder::dyn_var<loop_type> lidx = this->template linearize<0>(std::tuple{iters...});
  builder::dyn_var<Elem> erhs = rhs;
  data[lidx] = erhs;
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<Elem> SBlock<Elem,Rank>::operator()(const std::tuple<Iters...> &iters) {
  builder::dyn_var<loop_type> lidx = this->template linearize<0>(iters);
  return data[lidx];
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<Elem> SBlock<Elem,Rank>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank>
template <typename Idx>
auto SView<Elem,Rank>::operator[](Idx idx) {
  BlockLikeRef<SView<Elem,Rank>, std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank>
template <typename Rhs, typename...Iters>
void SView<Elem,Rank>::assign(Rhs rhs, Iters...iters) {
  // the iters are relative to the View, so first make them relative to the block
  // bi0 = vi0 * vstride0 + vorigin0
  auto biters = this->template compute_block_relative_iters<0>(std::tuple{iters...});
  // then linearize with respect to the block
  builder::dyn_var<loop_type> lidx = this->template linearize<0>(this->bextents, biters);
  builder::dyn_var<Elem> erhs = rhs;
  data[lidx] = erhs; 
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<Elem> SView<Elem,Rank>::operator()(const std::tuple<Iters...> &iters) {
  // the iters are relative to the View, so first make them relative to the block
  // bi0 = vi0 * vstride0 + vorigin0
  auto biters = this->template compute_block_relative_iters<0>(iters);
  // then linearize with respect to the block
  builder::dyn_var<loop_type> lidx = this->template linearize<0>(this->bextents, biters);
  return data[lidx];
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<Elem> SView<Elem,Rank>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename BlockLike, typename Idxs>
template <typename Idx>
auto BlockLikeRef<BlockLike, Idxs>::operator[](Idx idx) {
  auto merged = std::tuple_cat(idxs, std::tuple{idx});
  BlockLikeRef<BlockLike, decltype(merged)> ref(this->block_like, std::move(merged));
  return ref;
}

}
