// -*-c++-*-

#pragma once

#include <vector>
#include <sstream>
#include "builder/dyn_var.h"
#include "builder/array.h"
#include "fwddecls.h"
#include "traits.h"
#include "functors.h"
#include "expr.h"
#include "staged_utils.h"
#include "staged_allocators.h"

namespace cola {

// Notes:
// 1. don't use operator() with Iter objects. use operator[] instead.
// so block[i][j] = other[i][j] => good
// so block[i][j] = other(i,j) => bad

// 2. The location information within Blocks and Views is immutable once set, so we
// don't have to worry about breaking dependencies with dyn_vars when passing
// them across blocks and views.

// 3. If we don't deepcopy, then the user could modify dyn_vars that they use for extents and such, which
// would affect the location of a given block/view. This is bad. It'd basically be like having a pointer
// represent the location information.

template <typename Elem>
struct PtrWrap {
  using P = Elem*;
};

template <typename Elem, int Rank, bool MultiDimPtr, int Depth=0>
auto ptr_wrap() {  
  if constexpr (Depth == Rank-1 || MultiDimPtr == true) {
    return PtrWrap<Elem>();
  } else {
    return PtrWrap<typename decltype(ptr_wrap<Elem,Rank,Depth+1>())::P>();
  }
}

template <int Rank, bool MultiDimPtr>
constexpr int physical() {
  if constexpr (MultiDimPtr) {
    return Rank;
  } else {
    return 1;
  }
}

template <typename Elem>
struct Peel { constexpr int operator()() { return 1; } };
template <typename Elem>
struct Peel<Elem*> { constexpr int operator()() { return 1 + Peel<Elem>()(); } };

template <typename Elem>
constexpr int peel() {
  return Peel<Elem>()();
}

///
/// A region of data with a location
/// If MultiDimPtr = true, the underlying array is a pointer to pointer to etc... (like Rank 2 = **).
/// This is only supported for user-side allocations
template <typename Elem, int Rank, bool MultiDimPtr=false>
struct Block {
  
  template <typename Elem2, int Rank2, bool MultiDimPtr2>
  friend struct View;

  using SLoc_T = Loc_T<Rank>;
  using Elem_T = Elem;
  static constexpr int Rank_T = Rank;
  static constexpr bool IsBlock_T = true;  

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, bool memset_data=false);

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents, bool memset_data=false);

  ///
  /// Create an internally-managed heap Block
  static Block<Elem,Rank,MultiDimPtr> heap(SLoc_T bextents);

  ///
  /// Create an internally-managed stack Block
  template <loop_type...Extents>
  static Block<Elem,Rank,MultiDimPtr> stack();

  ///
  /// Create a user-managed allocation
  static Block<Elem,Rank,MultiDimPtr> user(SLoc_T bextents, 
					   builder::dyn_var<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P> user);
  
  ///
  /// Read a single element at the specified coordinate
  template <unsigned long N>
  builder::dyn_var<Elem> inner_read(builder::dyn_arr<loop_type,N> &iters);

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  builder::dyn_var<Elem> operator()(Coords...coords);

  ///
  /// Read a single element at the specified linear index
  template <typename LIdx>
  builder::dyn_var<Elem> plidx(LIdx lidx);

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, typename...Coords>
  void write(ScalarElem val, std::tuple<Coords...> coords);

  ///
  /// Create a View over this whole block
  View<Elem,Rank,MultiDimPtr> view();

  ///
  /// Slice out a View over a portion of this Block
  template <typename...Slices>
  View<Elem,Rank,MultiDimPtr> view(Slices...slices);

  /// 
  /// Perform a lazy inline access on this Block
  template <typename Idx>
  Ref<Block<Elem,Rank,MultiDimPtr>,std::tuple<typename RefIdxType<Idx>::type>> operator[](Idx idx);

  /// 
  /// Generate code for printing a rank 1, 2, or 3 Block.
  template <typename T=Elem>
  void dump_data();

  std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;

private:

  ///
  /// Create a Block with the specifed Allocation
  Block(SLoc_T bextents, std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator, bool memset_data=false);

};

///
/// A region of data with a location that shares its underlying data with another block or view
template <typename Elem, int Rank, bool MultiDimPtr=false>
struct View {

  using SLoc_T = builder::dyn_var<loop_type[Rank]>;
  using Elem_T = Elem;
  static constexpr int Rank_T = Rank;
  static constexpr bool IsBlock_T = false;

  /// 
  /// Create a View from the specified location information
  View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
       SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin,
       std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator) :
    bextents(deepcopy(bextents)), bstrides(deepcopy(bstrides)), 
    borigin(deepcopy(borigin)), vextents(deepcopy(vextents)), 
    vstrides(deepcopy(vstrides)), vorigin(deepcopy(vorigin)),
    allocator(allocator) { }

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  builder::dyn_var<Elem> operator()(std::tuple<Coords...>);

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  builder::dyn_var<Elem> operator()(Coords...iters);

  ///
  /// Read a single element at the specified linear index
  template <typename LIdx>
  builder::dyn_var<Elem> plidx(LIdx lidx);

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, typename...Coords>
  void write(ScalarElem val, std::tuple<Coords...> iters);

  ///
  /// Slice out a View over a portion of this View
  template <typename...Slices>
  View<Elem,Rank,MultiDimPtr> view(Slices...slices);

  /// 
  /// Perform a lazy inline access on this View
  template <typename Idx>
  Ref<View<Elem,Rank,MultiDimPtr>,std::tuple<typename RefIdxType<Idx>::type>> operator[](Idx idx);

  /// 
  /// Generate code for printing a rank 1, 2, or 3 View
  template <typename T=Elem>
  void dump_data();

  // print out the location info
  void dump_loc();

  std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;
  SLoc_T vextents;
  SLoc_T vstrides;
  SLoc_T vorigin;

private:
  
  ///
  /// Compute the indices into the underlying block.
  template <int Depth, typename...Coords>
  auto compute_block_relative_iters(const std::tuple<Coords...> &viters);

};

///
/// Represents a lazily evaluated access into a block, view, or other ref. 
/// Resolves when assigned to something.
/// Lets you do things like
/// a[i][j][k] = b[j][i][k+2]
template <typename BlockLike, typename Idxs>
struct Ref : public Expr<Ref<BlockLike,Idxs>> {

  Ref(BlockLike block_like, Idxs idxs) : block_like(block_like), idxs(idxs) { }

  /// Perform a lazy inline access on this Ref
  template <typename Idx>
  Ref<BlockLike,typename TupleTypeCat<typename RefIdxType<Idx>::type,Idxs>::type> operator[](Idx idx);
   
  ///
  /// Trigger evaluation (i.e. loop construction) of this access.
  /// Need this one for a case like so
  /// Block<int,2> block{...}
  /// Block<int,2> block2{...}  
  /// Iter i
  /// block[i][3] = block2[i][4]
  /// block and block2 have the same type signature, and so do the Refs created <Iter,int>, 
  /// so it's a copy assignment
  Ref<BlockLike, Idxs> &operator=(Ref<BlockLike, Idxs> &rhs);
  
  ///
  /// Trigger evaluation (i.e. loop construction) of this access.
  void operator=(typename BlockLike::Elem_T x);

  ///
  /// Trigger evaluation (i.e. loop construction) of this access.
  template <typename Rhs>
  void operator=(Rhs rhs);

  ///
  /// Realize the value represented by this Ref
  template <typename LhsIdxs, typename...Iters>
  auto realize(LhsIdxs lhs, std::tuple<Iters...> iters); 

  Idxs idxs;
  BlockLike block_like;

private:

  ///
  /// Helper method for realizing values for the Idxs of this ref
  template <int Depth, typename LhsIdxs, typename...Iters>
  auto realize_each(LhsIdxs lhs, std::tuple<Iters...> iters);
  
  ///
  /// Create the loop nest for this ref and also evaluate the rhs.
  template <typename Rhs, typename...Iters>
  void realize_loop_nest(Rhs rhs, Iters...iters);

  ///
  /// Verify that the Idxs of this Ref are unadorned. This is used when this is 
  /// the lhs of a regular inline statement
  template <int Depth=0>
  void verify_unadorned();

  ///
  /// Verify that any Iter types used as indices are unique. This is used when this is
  /// the lhs of a regular inline statement.
  template <int Depth, char...Seen>
  void verify_unique();
  
};

template <typename Elem, int Rank, bool MultiDimPtr>
template <unsigned long N>
builder::dyn_var<Elem> Block<Elem,Rank,MultiDimPtr>::inner_read(builder::dyn_arr<loop_type,N> &iters) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    builder::dyn_arr<loop_type,Rank> arr;
    for (builder::static_var<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (builder::static_var<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = iters[i];
    }
    return this->inner_read(arr);
  } else {
    if constexpr (MultiDimPtr==true) {
      return allocator->read(iters);
    } else {
      builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, iters);
      builder::dyn_arr<loop_type,1> arr{lidx};
      return allocator->read(arr);
    }
  }
}

template <int Depth, unsigned long N, typename Iter, typename...Iters>
void splat(builder::dyn_arr<loop_type,N> &arr, Iter iter, Iters...iters) {
  arr[Depth] = iter;
  if constexpr (Depth < N - 1) {
    splat<Depth+1>(arr, iters...);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename...Iters>
builder::dyn_var<Elem> Block<Elem,Rank,MultiDimPtr>::operator()(Iters...iters) {
  builder::dyn_arr<loop_type,sizeof...(Iters)> arr{iters...};
  return this->inner_read(arr);
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename ScalarElem, typename...Iters>
void Block<Elem,Rank,MultiDimPtr>::write(ScalarElem val, std::tuple<Iters...> iters) {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    auto coord = std::tuple_cat(make_tup<loop_type,0,pad_amt>(), iters);
    this->write(val, coord);
  } else { // coordinate-based
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, iters);
    builder::dyn_arr<loop_type,physical<Rank,MultiDimPtr>()> idxs;
    idxs[0] = lidx;
    allocator->write(val, idxs);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename Idx>
Ref<Block<Elem,Rank,MultiDimPtr>,std::tuple<typename RefIdxType<Idx>::type>> Block<Elem,Rank,MultiDimPtr>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    builder::dyn_var<loop_type> didx = idx;
    return Ref<Block<Elem,Rank,MultiDimPtr>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    return Ref<Block<Elem,Rank,MultiDimPtr>,std::tuple<Idx>>(*this, std::tuple{idx});
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename LIdx>
builder::dyn_var<Elem> Block<Elem,Rank,MultiDimPtr>::plidx(LIdx lidx) {
  // don't need to delinearize here since the pseudo index is just a normal
  // index for a block
  builder::dyn_arr<loop_type,physical<Rank,MultiDimPtr>()> idxs;
  idxs[0] = lidx;
  return allocator->read(idxs);
}

template <typename Elem, int Rank, bool MultiDimPtr>
View<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::view() {
  return View<Elem,Rank,MultiDimPtr>(this->bextents, this->bstrides, this->borigin,
			 this->bextents, this->bstrides, this->borigin,
			 this->allocator);
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename...Slices>
View<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::view(Slices...slices) {
  // the block parameters stay the same, but we need to update the
  // view parameters  
  SLoc_T vstops;
  gather_stops<0,Rank>(vstops, this->bextents, slices...);
  SLoc_T vstrides;
  gather_strides<0,Rank>(vstrides, slices...);
  SLoc_T vorigin;
  gather_origin<0,Rank>(vorigin, slices...);
  // convert vstops into extents
  SLoc_T vextents = convert_stops_to_extents<Rank>(vorigin, vstops, vstrides);
  // now make everything relative to the prior block
  // new origin = old origin + vorigin * old strides
  SLoc_T origin = apply<AddFunctor,Rank>(this->borigin, apply<MulFunctor,Rank>(vorigin, this->bstrides));
  // new strides = old strides * new strides
  SLoc_T strides = apply<MulFunctor,Rank>(this->bstrides, vstrides);
  return View<Elem,Rank,MultiDimPtr>(this->bextents, this->bstrides, this->borigin,
			 vextents, strides, vorigin,
			 this->allocator);  
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename T>
void Block<Elem,Rank,MultiDimPtr>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports ranks 1, 2, and 3");
  if constexpr (Rank == 1) {
    for (builder::dyn_var<loop_type> i = 0; i < bextents[0]; i=i+1) {
      dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i)));
    }
  } else if constexpr (Rank == 2) {
    for (builder::dyn_var<loop_type> i = 0; i < bextents[0]; i=i+1) {
      for (builder::dyn_var<loop_type> j = 0; j < bextents[1]; j=j+1) {
	dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j)));
      }
      print_newline();
    }
  } else {
    for (builder::dyn_var<loop_type> i = 0; i < bextents[0]; i=i+1) {
      for (builder::dyn_var<loop_type> j = 0; j < bextents[1]; j=j+1) {
	for (builder::dyn_var<loop_type> k = 0; k < bextents[2]; k=k+1) {
	  dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j,k)));
	}
	print_newline();
      }
      print_newline();
      print_newline();
    }
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr>::Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, bool memset_data) :
  bextents(deepcopy(bextents)), bstrides(deepcopy(bstrides)), borigin(deepcopy(borigin)) {
  static_assert(!MultiDimPtr);
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
  if (memset_data) {
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(reduce<MulFunctor>(bextents) * elem_size);
    allocator->memset(sz);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr>::Block(SLoc_T bextents, bool memset_data) :
  bextents(std::move(bextents)) {
  static_assert(!MultiDimPtr);
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    bstrides[i] = 1;
    borigin[i] = 0;
  }
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
  if (memset_data) {
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(reduce<MulFunctor>(bextents) * elem_size);
    allocator->memset(sz);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr>::Block(SLoc_T bextents, 
				    std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator, 
				    bool memset_data) :
  bextents(std::move(bextents)), allocator(allocator) {
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    bstrides[i] = 1;
    borigin[i] = 0;
  }
  if (memset_data) {
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(reduce<MulFunctor>(bextents) * elem_size);
    allocator->memset(sz);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::heap(SLoc_T bextents) {
  return Block<Elem,Rank,MultiDimPtr>(bextents, false);
}    

template <typename Elem, int Rank, bool MultiDimPtr>
template <loop_type...Extents>
Block<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::stack() {
  static_assert(Rank == sizeof...(Extents));
  auto allocator = std::make_shared<StackAllocation<Elem,mul_reduce<Extents...>()>>();
  auto bextents = to_Loc_T<Extents...>();
  return Block<Elem,Rank,MultiDimPtr>(bextents, allocator);
}

template <typename Elem, int Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::user(SLoc_T bextents, 
								builder::dyn_var<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P> user) {  
  // make sure that the dyn_var passed in matches the actual storage type (seems like it's not caught otherwise?)
  static_assert((MultiDimPtr && peel<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P>() == Rank) || 
		(!MultiDimPtr && peel<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P>() == 1));
  if constexpr (MultiDimPtr) {
    auto allocator = std::make_shared<UserAllocation<Elem,typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P,Rank>>(user);
    return Block<Elem,Rank,MultiDimPtr>(bextents, allocator);
  } else {
    auto allocator = std::make_shared<UserAllocation<Elem,Elem*,1>>(user);
    return Block<Elem,Rank,MultiDimPtr>(bextents, allocator);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename...Iters>
builder::dyn_var<Elem> View<Elem,Rank,MultiDimPtr>::operator()(std::tuple<Iters...> iters) {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    builder::dyn_var<loop_type> zero = 0;
    auto coord = std::tuple_cat(make_tup<builder::dyn_var<loop_type>,zero,pad_amt>(), iters);
    return this->operator()(coord);
  } else {
    // the iters are relative to the View, so first make them relative to the block
    // bi0 = vi0 * vstride0 + vorigin0
    auto biters = compute_block_relative_iters<0>(iters);
    // then linearize with respect to the block
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, biters);
    // see block::operator for this I do it this way
    builder::dyn_arr<loop_type,physical<Rank,MultiDimPtr>()> idxs;
    idxs[0] = lidx;
    return allocator->read(idxs);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename...Iters>
builder::dyn_var<Elem> View<Elem,Rank,MultiDimPtr>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename ScalarElem, typename...Iters>
void View<Elem,Rank,MultiDimPtr>::write(ScalarElem val, std::tuple<Iters...> iters) {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    builder::dyn_var<loop_type> zero = 0;
    auto coord = std::tuple_cat(make_tup<builder::dyn_var<loop_type>,zero,pad_amt>(), iters);
    this->write(val, coord);
  } else {
    // the iters are relative to the View, so first make them relative to the block
    // bi0 = vi0 * vstride0 + vorigin0
    auto biters = compute_block_relative_iters<0>(iters);
    // then linearize with respect to the block
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, biters);
    builder::dyn_arr<loop_type,physical<Rank,MultiDimPtr>()> idxs;
    idxs[0] = lidx;
    allocator->write(val, idxs);
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename Idx>
Ref<View<Elem,Rank,MultiDimPtr>,std::tuple<typename RefIdxType<Idx>::type>> View<Elem,Rank,MultiDimPtr>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    builder::dyn_var<loop_type> didx = idx;
    return Ref<View<Elem,Rank,MultiDimPtr>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    return Ref<View<Elem,Rank,MultiDimPtr>,std::tuple<Idx>>(*this, std::tuple{idx});
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename LIdx>
builder::dyn_var<Elem> View<Elem,Rank,MultiDimPtr>::plidx(LIdx lidx) {
  // must delinearize relative to the view
  auto coord = delinearize<0,Rank>(lidx, this->vextents);
  return this->operator()(coord);
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename...Slices>
View<Elem,Rank,MultiDimPtr> View<Elem,Rank,MultiDimPtr>::view(Slices...slices) {
  // the block parameters stay the same, but we need to update the
  // view parameters  
  SLoc_T vstops;
  gather_stops<0,Rank>(vstops, this->vextents, slices...);
  SLoc_T vstrides;
  gather_strides<0,Rank>(vstrides, slices...);
  SLoc_T vorigin;
  gather_origin<0,Rank>(vorigin, slices...);
  // convert vstops into extents
  SLoc_T vextents = convert_stops_to_extents<Rank>(vorigin, vstops, vstrides);
  // now make everything relative to the prior view
  // new origin = old origin + vorigin * old strides
  SLoc_T origin = apply<AddFunctor,Rank>(this->vorigin, apply<MulFunctor,Rank>(vorigin, this->vstrides));
  // new strides = old strides * new strides
  SLoc_T strides = apply<MulFunctor,Rank>(this->vstrides, vstrides);
  return View<Elem,Rank,MultiDimPtr>(this->bextents, this->bstrides, this->borigin,
			 vextents, strides, origin,
			 this->allocator);
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <int Depth, typename...Coords>
auto View<Elem,Rank,MultiDimPtr>::compute_block_relative_iters(const std::tuple<Coords...> &viters) {
  if constexpr (Depth == sizeof...(Coords)) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{std::get<Depth>(viters) * vstrides[Depth] +
				     vorigin[Depth]}, 
      compute_block_relative_iters<Depth+1>(viters));
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
template <typename T>
void View<Elem,Rank,MultiDimPtr>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports ranks 1, 2, and 3");
  if constexpr (Rank == 1) {
    for (builder::dyn_var<loop_type> i = 0; i < vextents[0]; i=i+1) {
      dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i)));
    }
  } else if constexpr (Rank == 2) {
    for (builder::dyn_var<loop_type> i = 0; i < vextents[0]; i=i+1) {
      for (builder::dyn_var<loop_type> j = 0; j < vextents[1]; j=j+1) {
	dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j)));
      }
      print_newline();
    }
  } else {
    for (builder::dyn_var<loop_type> i = 0; i < vextents[0]; i=i+1) {
      for (builder::dyn_var<loop_type> j = 0; j < vextents[1]; j=j+1) {
	for (builder::dyn_var<loop_type> k = 0; k < vextents[2]; k=k+1) {
	  dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j,k)));
	}
	print_newline();
      }
      print_newline();
      print_newline();
    }
  }
}

template <typename Elem, int Rank, bool MultiDimPtr>
void View<Elem,Rank,MultiDimPtr>::dump_loc() {
  print_string("View location info");
  print_newline();
  print_string("  BExtents:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print_string(" ");
    dispatch_print_elem<int>(bextents[r]);    
  }
  print_newline();
  print_string("  BStrides:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print_string(" ");
    dispatch_print_elem<int>(bstrides[r]);    
  }
  print_newline();
  print_string("  BOrigin:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print_string(" ");
    dispatch_print_elem<int>(borigin[r]);    
  }
  print_newline();
  print_string("  VExtents:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print_string(" ");
    dispatch_print_elem<int>(vextents[r]);    
  }
  print_newline();
  print_string("  VStrides:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print_string(" ");
    dispatch_print_elem<int>(vstrides[r]);    
  }
  print_newline();
  print_string("  VOrigin:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print_string(" ");
    dispatch_print_elem<int>(vorigin[r]);    
  }  
  print_newline();
}

///
/// Helper for verifying unique uses of Iter types
template <char C, char...Others>
struct VerifyUnique { constexpr bool operator()() { return true; } }; // only a single thing in this case

template <char C, char D, char...Others>
struct VerifyUnique<C, D, Others...> { 
  constexpr bool operator()() { 
    if constexpr (sizeof...(Others) > 0) {
      return VerifyUnique<C, Others...>()();
    } else {
      return true;
    }
  } 
};

template <char C, char...Others>
struct VerifyUnique<C, C, Others...> {
  constexpr bool operator()() { return false; }
};

template <typename BlockLike, typename Idxs>
template <typename Idx>
Ref<BlockLike,typename TupleTypeCat<typename RefIdxType<Idx>::type,Idxs>::type> Ref<BlockLike,Idxs>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    builder::dyn_var<loop_type> didx = idx;
    using DIdx = decltype(didx);
    typename TupleTypeCat<DIdx,Idxs>::type args = std::tuple_cat(idxs, std::tuple{didx});
    return Ref<BlockLike,decltype(args)>(block_like, args);
  } else {
    typename TupleTypeCat<Idx,Idxs>::type args = std::tuple_cat(idxs, std::tuple{idx});
    return Ref<BlockLike,decltype(args)>(block_like, args);
  }
}

template <typename BlockLike, typename Idxs>
Ref<BlockLike, Idxs> &Ref<BlockLike,Idxs>::operator=(Ref<BlockLike, Idxs> &rhs) {
  // force it to the not copy-assignment operator=
  *this = BlockLike::Elem_T(1) * rhs;    
  return *this;
}
  
template <typename BlockLike, typename Idxs>
void Ref<BlockLike,Idxs>::operator=(typename BlockLike::Elem_T x) {
  this->verify_unadorned();
  this->verify_unique<0>();
  // TODO can potentially memset this whole thing
  static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
  realize_loop_nest(x);
}

template <typename BlockLike, typename Idxs>
template <typename Rhs>
void Ref<BlockLike,Idxs>::operator=(Rhs rhs) {
  this->verify_unadorned();
  this->verify_unique<0>();
  static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
  realize_loop_nest(rhs);
}

template <typename BlockLike, typename Idxs>
template <int Depth, typename LhsIdxs, typename...Iters>
auto Ref<BlockLike,Idxs>::realize_each(LhsIdxs lhs, std::tuple<Iters...> iters) {
  auto i = std::get<Depth>(idxs);
  if constexpr (is_dyn_like<decltype(i)>::value ||
		std::is_same<decltype(i),loop_type>()) {
    if constexpr (Depth < sizeof...(Iters) - 1) {
      return std::tuple_cat(std::tuple{i}, realize_each<Depth+1>(lhs, iters));
    } else {
      return std::tuple{i};	  
    }
  } else {
    auto r = i.realize(lhs, iters);
    if constexpr (Depth < sizeof...(Iters) - 1) {
      return std::tuple_cat(std::tuple{r}, realize_each<Depth+1>(lhs, iters));
    } else {
      return std::tuple{r};
    }
  }
}
  
template <typename BlockLike, typename Idxs>
template <typename LhsIdxs, typename...Iters>
auto Ref<BlockLike,Idxs>::realize(LhsIdxs lhs, std::tuple<Iters...> iters) {
  // first realize each idx
  auto realized = realize_each<0>(lhs, iters);
  // then access the thing
  return block_like(realized);
}

template <typename BlockLike, typename Idxs>
template <typename Rhs, typename...Iters>
void Ref<BlockLike,Idxs>::realize_loop_nest(Rhs rhs, Iters...iters) {
  // the lhs indices can be either an Iter or an integer.
  // Iter -> loop over whole extent
  // integer -> single iteration loop
  constexpr int rank = BlockLike::Rank_T;
  constexpr int depth = sizeof...(Iters);
  if constexpr (depth < rank) {
    auto dummy = std::get<depth>(idxs);
    if constexpr (std::is_integral<decltype(dummy)>() ||
		  is_dyn_like<decltype(dummy)>::value) {
	// single iter
	builder::dyn_var<loop_type> iter = std::get<depth>(idxs);
	realize_loop_nest(rhs, iters..., iter);
    } else {
      // loop
      if constexpr (BlockLike::IsBlock_T) {
	for (builder::dyn_var<loop_type> iter = 0; iter < block_like.bextents[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      } else {
	for (builder::dyn_var<loop_type> iter = 0; iter < block_like.vextents[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      }
    }
  } else {
    // at the innermost level
    if constexpr (std::is_arithmetic<Rhs>() ||
		  is_dyn_like<Rhs>::value) {
      block_like.write(rhs, std::tuple{iters...});
    } else {
      block_like.write(rhs.realize(idxs, std::tuple{iters...}), std::tuple{iters...});	  
    }
  }
}

template <typename BlockLike, typename Idxs>
template <int Depth>
void Ref<BlockLike,Idxs>::verify_unadorned() {
  // unadorned must be either an integral, Iter, or dyn_var<loop_type>
  auto i = std::get<Depth>(idxs);
  using T = decltype(i);
  static_assert(std::is_integral<T>() || 
		is_dyn_like<T>::value ||
		is_iter<T>::value, "LHS indices for inline write must be unadorned.");
  if constexpr (Depth < BlockLike::Rank_T - 1) {
    verify_unadorned<Depth+1>();
  }
}  

template <typename BlockLike, typename Idxs>
template <int Depth, char...Seen>
void Ref<BlockLike,Idxs>::verify_unique() {
  // unadorned must be either an integral, Iter, or dyn_var<loop_type>
  auto i = std::get<Depth>(idxs);
  using T = decltype(i);
  if constexpr (is_iter<T>::value) {
    static_assert(VerifyUnique<T::Ident_T, Seen...>()(), "LHS indices for inline write must be unique.");
    if constexpr (Depth < BlockLike::Rank_T - 1) {
      verify_unique<Depth+1, Seen..., T::Ident_T>();
    }
  } else {
    if constexpr (Depth < BlockLike::Rank_T - 1) {
      verify_unique<Depth+1, Seen...>();
    }
  }
}  


}
