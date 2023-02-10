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
#include "object.h"
#ifdef UNSTAGED
#include "runtime/cpp/heaparray.h"
#endif

namespace shim {


#ifndef UNSTAGED
template <typename Elem, int PhysicalRank>
using Allocation_T = std::shared_ptr<Allocation<Elem,PhysicalRank>>;
#else
template <typename Elem, int PhysicalRank>
using Allocation_T = HeapArray<Elem>;
#endif
 

// Notes:
// 1. don't use operator() with Iter objects (it won't compile). use operator[] instead.
// so block[i][j] = other[i][j] => good
// so block[i][j] = other(i,j) => bad

// 2. The location information within Blocks and Views is immutable once set, so we
// don't have to worry about breaking dependencies with dyn_vars when passing
// them across blocks and views.

// 3. If we don't deepcopy, then the user could modify dyn_vars that they use for extents and such, which
// would affect the location of a given block/view. This is bad. It'd basically be like having a pointer
// represent the location information.

// 4. Valid indexing schemes:
// - No frozen dims: you can specify 0 <= <nidxs> <= NLogical. If you specify less than NLogical, 0s are padded 
// to the front indices. So If NLogical = 3 and you specify obj(8), this would be padded to obj(0,0,8).
// - With frozen dims: you can specify
// a) the full NLogical dims: this overrides the frozen dims
// b) NUnfrozen dims: this combines the frozen dims with the indices specified
// c) < NUnfrozen dims: this combines the frozen dims, 0 padding, and the indices specified

// 5. Unstaging only supports heap arrays. no stacks, multidimensional arrays, etc

///
/// A region of data with a location
/// If MultiDimRepr = true, the underlying array is a pointer to pointer to etc... (like Rank 2 = **).
/// This is only supported for user-side allocations
/// Rank represents the logical number of dimensions (i.e. how many dimensions you can index)
template <typename Elem, unsigned long Rank, bool MultiDimRepr=false>
struct Block {
  
  template <typename Elem2, unsigned long Rank2, bool MultiDimRepr2, unsigned long Frozen>
  friend struct View;

  using SLoc_T = Loc_T<Rank>;
  using Elem_T = Elem;
//  static constexpr unsigned long n_logical_dims() { return Rank; }
  static constexpr unsigned long NLogical_T = Rank;
  static constexpr unsigned long NFrozen_T = 0;
  static constexpr unsigned long NUnfrozen_T = 0;
  static constexpr bool IsBlock_T = true;  

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin);

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents);

  ///
  /// Create an internally-managed heap Block
  static Block<Elem,Rank,MultiDimRepr> heap(SLoc_T bextents);

#ifndef UNSTAGED
  ///
  /// Create an internally-managed stack Block
  template <loop_type...Extents>
  static Block<Elem,Rank,MultiDimRepr> stack();
#endif

  ///
  /// Create a user-managed allocation
#ifndef UNSTAGED
  static Block<Elem,Rank,MultiDimRepr> user(SLoc_T bextents, 
					   dvar<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P> user);
#else
  static Block<Elem,Rank,MultiDimRepr> user(SLoc_T bextents, dvar<Elem*> user);
#endif

  ///
  /// Read a single element at the specified coordinate
  template <unsigned long N>
  dvar<Elem> read(darr<loop_type,N> &iters);

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  dvar<Elem> operator()(Coords...coords);

  ///
  /// Read a single element at the specified linear index
  template <typename LIdx>
  dvar<Elem> plidx(LIdx lidx);

  ///
  /// Create a view on this' data using the location of block.
  template <typename Elem2, bool MultiDimRepr2>
  View<Elem,Rank,MultiDimRepr,0> colocate(Block<Elem2,Rank,MultiDimRepr2> &block);

  ///
  /// Create a view on this' data using the location of view (not its underlying block!)
  template <typename Elem2, bool MultiDimRepr2>
  View<Elem,Rank,MultiDimRepr,0> colocate(View<Elem2,Rank,MultiDimRepr2,0> &view);

  /// 
  /// Create interpolation factors that logically increase the extent of this block.
  template <typename...Factors>
  View<Elem,Rank,MultiDimRepr,0> logically_interpolate(Factors...factors);

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, unsigned long N>
  void write(ScalarElem val, darr<loop_type,N> &iters);

  ///
  /// Create a View over this whole block
  View<Elem,Rank,MultiDimRepr,0> view();

  ///
  /// Slice out a View over a portion of this Block
  template <typename...Slices>
  View<Elem,Rank,MultiDimRepr,0> view(Slices...slices);

  /// 
  /// Perform a lazy inline access on this Block
  template <typename Idx>
  Ref<Block<Elem,Rank,MultiDimRepr>,std::tuple<typename RefIdxType<Idx>::type>> operator[](Idx idx);

  ///
  /// Partially apply the coordinates and return a View that can be indexed with less dimensions.
  template <typename...Coords>
  View<Elem,Rank-sizeof...(Coords),MultiDimRepr,sizeof...(Coords)> freeze(Coords...coords);  

  /// 
  /// Generate code for printing a rank 1, 2, or 3 Block.
  template <typename T=Elem>
  void dump_data();

  ///
  /// print out the location info
  void dump_loc();

  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;

private:

  ///
  /// Create a Block with the specifed Allocation
  Block(SLoc_T bextents, Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator);

};

///
/// A region of data with a location that shares its underlying data with another block or view
/// NUnfrozen = maximum number of dimensions you can index 
/// NFrozen = number of frozen dimensions
/// NUnfrozen + NFrozen = total number of logical dimensions. If MultiDimRepr=false, # physical dims == 1
/// If MultiDimRepr=true, # physical dims == # logical dims
template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr=false, unsigned long NFrozen=0>
struct View {

  using SLoc_T = Loc_T<NUnfrozen+NFrozen>;
  using Elem_T = Elem;
//  static constexpr unsigned long Rank_T = Rank;
//  static constexpr unsigned long NFrozen_T = Rank;
  static constexpr unsigned long NLogical_T = NUnfrozen + NFrozen;
  static constexpr unsigned long NFrozen_T = NFrozen;
  static constexpr unsigned long NUnfrozen_T = NUnfrozen;
  static constexpr bool IsBlock_T = false;
//  static constexpr unsigned long n_logical_dims() { return NUnfrozen + NFrozen; }

  /// 
  /// Create a View from the specified location information
  View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
       SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin,
       SLoc_T interpolation_factors,
       Allocation_T<Elem,physical<NLogical_T,MultiDimRepr>()> allocator) :
    bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
    vextents(std::move(vextents)), vstrides(std::move(vstrides)), vorigin(std::move(vorigin)), 
    interpolation_factors(std::move(interpolation_factors)),
    allocator(std::move(allocator)) { }
    
  /// 
  /// Create a View from the specified location information
  View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
       SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin,
       SLoc_T interpolation_factors,
       Allocation_T<Elem,physical<NLogical_T,MultiDimRepr>()> allocator,
       Loc_T<NFrozen> frozen) :
    bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
    vextents(std::move(vextents)), vstrides(std::move(vstrides)), vorigin(std::move(vorigin)), 
    interpolation_factors(std::move(interpolation_factors)),
    allocator(std::move(allocator)), frozen(std::move(frozen)) { }
  
  ///
  /// Read a single element at the specified coordinate
  template <unsigned long N>
  dvar<Elem> read(darr<loop_type,N> &coords);

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  dvar<Elem> operator()(Coords...coords);

  ///
  /// Read a single element at the specified linear index
  template <typename LIdx>
  dvar<Elem> plidx(LIdx lidx);

  ///
  /// Create a view on this' data using the location of block.
  template <typename Elem2, bool MultiDimRepr2>
  View<Elem,NUnfrozen,MultiDimRepr,0> colocate(Block<Elem2,NLogical_T,MultiDimRepr2> &block);

  ///
  /// Create a view on this' data using the location of view (not its underlying block!)
  template <typename Elem2, bool MultiDimRepr2>
  View<Elem,NUnfrozen,MultiDimRepr,0> colocate(View<Elem2,NLogical_T,MultiDimRepr2,0> &view);

  /// 
  /// Create interpolation factors that logically increase the extent of this block.
  template <typename...Factors>
  View<Elem,NUnfrozen,MultiDimRepr,0> logically_interpolate(Factors...factors);

  ///
  /// Reset interpolation factors back to 1
  View<Elem,NUnfrozen,MultiDimRepr,NFrozen> reset();

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, unsigned long N>
  void write(ScalarElem val, darr<loop_type,N> &coords);

  ///
  /// Partially apply the coordinates and return a View that can be indexed with less dimensions.
  template <typename...Coords>
  View<Elem,NUnfrozen-sizeof...(Coords),MultiDimRepr,NFrozen+sizeof...(Coords)> freeze(Coords...coords);  

  ///
  /// Slice out a View over a portion of this View
  template <typename...Slices>
  View<Elem,NUnfrozen,MultiDimRepr,NFrozen> view(Slices...slices);

  /// 
  /// Perform a lazy inline access on this View
  template <typename Idx>
  auto operator[](Idx idx);

  /// 
  /// Generate code for printing a rank 1, 2, or 3 View
  template <typename T=Elem>
  void dump_data();

  ///
  /// print out the location info
  void dump_loc();

  ///
  /// True if global coords computed from the input coords are >= 0.
  /// Does not guarantee that the data physically exists in the underlying block.  
  template <typename...Coords>
  dvar<bool> logically_exists(Coords...coords);
    
  Allocation_T<Elem,physical<NLogical_T,MultiDimRepr>()> allocator;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;
  SLoc_T vextents;
  SLoc_T vstrides;
  SLoc_T vorigin;  
  SLoc_T interpolation_factors;
  darr<loop_type,NFrozen> frozen; // prepended

private:
  
  ///
  /// Compute the absolute indices of the coordinates. Can only be called once frozen dims are prepended.
  template <int Depth>
  void compute_absolute_location(const darr<loop_type,NLogical_T> &coords,
				 darr<loop_type,NLogical_T> &out);

};

///
/// Represents a lazily evaluated access into a block, view, or other ref. 
/// Resolves when assigned to something.
/// Lets you do things like
/// a[i][j][k] = b[j][i][k+2]
template <typename BlockLike, typename Idxs>
struct Ref : public Expr<Ref<BlockLike,Idxs>> {

  template <typename A, typename B>
  friend struct Ref;

  template <typename T, typename LhsIdxs, unsigned long N>
  friend dvar<typename GetCoreT<T>::Core_T> dispatch_realize(T to_realize, const LhsIdxs &lhs_idxs, const darr<loop_type,N> &iters);

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
  /// Trigger evaluation (i.e. loop construction) of this access.
  /// Used with a dyn_var or builder, and forces builder to a dyn_var
//  template <typename Rhs, typename std::enable_if<is_dyn_like<Rhs>::value, int>::type=0>
  void operator=(builder::builder rhs);

  Idxs idxs;
  BlockLike block_like;

private:

  ///
  /// For adding padding and frozen dimensions to the lhs of an assignment
  template <typename Rhs>
  void internal_assign(Rhs rhs);

  ///
  /// Realize the value represented by this Ref
  template <typename LhsIdxs, unsigned long N>
  dvar<typename BlockLike::Elem_T> realize(LhsIdxs lhs, 
					   const darr<loop_type,N> &iters);

  ///
  /// Helper method for realizing values for the Idxs of this ref
  template <int Iter, int IdxDepth, typename LhsIdxs, unsigned long N>
  void realize_each(LhsIdxs lhs, const darr<loop_type,N> &iters,
		    darr<loop_type,BlockLike::NLogical_T> &out);
  
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

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long N>
dvar<Elem> Block<Elem,Rank,MultiDimRepr>::read(darr<loop_type,N> &coords) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    darr<loop_type,Rank> arr;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (svar<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    return this->read(arr);
  } else {
#ifndef UNSTAGED
    if constexpr (MultiDimRepr==true) {
      return allocator->read(coords);
    } else {
      dvar<loop_type> lidx = linearize<0,Rank>(this->bextents, coords);
      darr<loop_type,1> arr{lidx};
      return allocator->read(arr);
    }
#else
  dvar<loop_type> lidx = linearize<0,Rank>(this->bextents, coords);
  return allocator[lidx];
#endif
  }
}

template <int Depth, unsigned long N, typename Coord, typename...Coords>
void splat(darr<loop_type,N> &arr, Coord coord, Coords...coords) {
  arr[Depth] = coord;
  if constexpr (Depth < N - 1) {
    splat<Depth+1>(arr, coords...);
  }
}

template <typename T, typename...Ts>
void validate_idx_types(T t, Ts...ts) {
  static_assert(std::is_integral<T>() || is_dyn_like<T>::value);
  if constexpr (sizeof...(Ts) > 0) {
    validate_idx_types(ts...);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Coords>
dvar<Elem> Block<Elem,Rank,MultiDimRepr>::operator()(Coords...coords) {
  // it's possible to pass a dyn_arr in here and have it not complain (but screw everything up).
  // so verify that indices are valid.
  // can be either loop type or dyn_var<loop_type>
  validate_idx_types(coords...);
  darr<loop_type,sizeof...(Coords)> arr{coords...};
  return this->read(arr);
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename ScalarElem, unsigned long N>
void Block<Elem,Rank,MultiDimRepr>::write(ScalarElem val, darr<loop_type,N> &coords) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    darr<loop_type,Rank> arr;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (svar<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    this->write(val, arr);
  } else {
#ifndef UNSTAGED
    if constexpr (MultiDimRepr==true) {
      allocator->write(val, coords);
    } else {
      dvar<loop_type> lidx = linearize<0,Rank>(this->bextents, coords);
      darr<loop_type,1> arr{lidx};
      allocator->write(val, arr);
    }
#else
    dvar<loop_type> lidx = linearize<0,Rank>(this->bextents, coords);
    allocator.write(lidx, val);
#endif
  }
}


template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Idx>
Ref<Block<Elem,Rank,MultiDimRepr>,std::tuple<typename RefIdxType<Idx>::type>> Block<Elem,Rank,MultiDimRepr>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    dvar<loop_type> didx = idx;
    return Ref<Block<Elem,Rank,MultiDimRepr>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    return Ref<Block<Elem,Rank,MultiDimRepr>,std::tuple<Idx>>(*this, std::tuple{idx});
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename LIdx>
dvar<Elem> Block<Elem,Rank,MultiDimRepr>::plidx(LIdx lidx) {
#ifndef UNSTAGED
  if constexpr (MultiDimRepr == true) {
    SLoc_T idxs;
    delinearize<0,Rank>(idxs, lidx, this->bextents);
    return allocator->read(idxs);
  } else {
    darr<loop_type,physical<Rank,MultiDimRepr>()> idxs{lidx};
    return allocator->read(idxs);
  }
#else
  return allocator[lidx];
#endif
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,Rank,MultiDimRepr,0> Block<Elem,Rank,MultiDimRepr>::colocate(Block<Elem2,Rank,MultiDimRepr2> &block) {
  SLoc_T interpolation_factors;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return {this->bextents, this->bstrides, this->borigin, 
    block.bextents, block.bstrides, block.borigin,  
    interpolation_factors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,Rank,MultiDimRepr,0> Block<Elem,Rank,MultiDimRepr>::colocate(View<Elem2,Rank,MultiDimRepr2,0> &view) {
  return {this->bextents, this->bstrides, this->borigin, 
    view.vextents, view.vstrides, view.vorigin, 
    view.interpolation_factors,
    this->allocator};
} 

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
View<Elem,Rank,MultiDimRepr,0> Block<Elem,Rank,MultiDimRepr>::logically_interpolate(Factors...factors) {
  SLoc_T ifactors{factors...};
  SLoc_T vextents;
  SLoc_T vorigin;
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
    vextents[i] = this->bextents[i] * ifactors[i];
    vorigin[i] = this->borigin[i] * ifactors[i];
  }
  return {this->bextents, this->bstrides, this->borigin,
    std::move(vextents), this->bstrides, std::move(vorigin),
    ifactors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr,0> Block<Elem,Rank,MultiDimRepr>::view() {
  SLoc_T interpolation_factors;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return {this->bextents, this->bstrides, this->borigin,
    this->bextents, this->bstrides, this->borigin,
    interpolation_factors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Slices>
View<Elem,Rank,MultiDimRepr,0> Block<Elem,Rank,MultiDimRepr>::view(Slices...slices) {
  // the block parameters stay the same, but we need to update the
  // view parameters  
  SLoc_T vstops;
  gather_stops<0,Rank>(vstops, this->bextents, slices...);
  SLoc_T vstrides;
  gather_strides<0,Rank>(vstrides, slices...);
  SLoc_T vorigin;
  gather_origin<0,Rank>(vorigin, slices...);
  // convert vstops into extents
  // just use the raw values from the slice parameters
  SLoc_T vextents;
  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  // now we need to update the strides and origin based on the block's parameters
  // new strides = old strides * new strides
  SLoc_T strides;
  apply<MulFunctor,Rank>(strides, this->bstrides, vstrides);
  for (svar<int> i = 0; i < Rank; i=i+1) {
    vorigin[i] = vorigin[i] * this->bstrides[i] + this->borigin[i];
  }
  SLoc_T interpolation_factors;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return {this->bextents, this->bstrides, this->borigin,
    std::move(vextents), std::move(strides), std::move(vorigin),
    std::move(interpolation_factors),
    this->allocator};  
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Coords>
View<Elem,Rank-sizeof...(Coords),MultiDimRepr,sizeof...(Coords)> Block<Elem,Rank,MultiDimRepr>::freeze(Coords...coords) {
  Loc_T<sizeof...(Coords)> frozen{coords...};
  SLoc_T interpolation_factors;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return {this->bextents, this->bstrides, this->borigin,
    this->bextents, this->bstrides, this->borigin,
    std::move(interpolation_factors),
    this->allocator,
    std::move(frozen)};  
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename T>
void Block<Elem,Rank,MultiDimRepr>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports ranks 1, 2, and 3");
  if constexpr (Rank == 1) {
    for (dvar<loop_type> i = 0; i < bextents[0]; i=i+1) {
#ifndef UNSTAGED
      dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i)));
#else
      std::cout << (Elem)(this->operator()(i)) << " ";
#endif
    }
  } else if constexpr (Rank == 2) {
    for (dvar<loop_type> i = 0; i < bextents[0]; i=i+1) {
      for (dvar<loop_type> j = 0; j < bextents[1]; j=j+1) {
#ifndef UNSTAGED
	dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j)));
#else
	std::cout << (Elem)(this->operator()(i,j)) << " ";
#endif
      }
#ifndef UNSTAGED
      print_newline();
#else
      std::cout << std::endl;
#endif
    }
  } else {
    for (dvar<loop_type> i = 0; i < bextents[0]; i=i+1) {
      for (dvar<loop_type> j = 0; j < bextents[1]; j=j+1) {
	for (dvar<loop_type> k = 0; k < bextents[2]; k=k+1) {
#ifndef UNSTAGED
	  dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j,k)));
#else
	  std::cout << (Elem)(this->operator()(i,j,k)) << " ";
#endif
	}
#ifndef UNSTAGED
	print_newline();
#else
	std::cout << std::endl;
#endif
      }
#ifndef UNSTAGED
      print_newline();
      print_newline();
#else
      std::cout << std::endl;
      std::cout << std::endl;
#endif
    }
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void Block<Elem,Rank,MultiDimRepr>::dump_loc() {
#ifndef UNSTAGED
  print("Block location info");
  print_newline();
  print("  Underlying data structure: ");
  print(ElemToStr<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>::str);
#else
  std::cout << ("Block location info") << std::endl;
  std::cout << ("  Underlying data structure: ");
  std::cout << (ElemToStr<Elem*>::str);
#endif
#ifndef UNSTAGED
  print_newline();
  print("  BExtents:");
#else
  std::cout << std::endl << "  BExtents:";
#endif
  for (svar<int> r = 0; r < Rank; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(bextents[r]);    
#else
    std::cout << " " << (int)(bextents[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
  print("  BStrides:");
#else
  std::cout << std::endl << "  BStrides:";
#endif
  for (svar<int> r = 0; r < Rank; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(bstrides[r]);    
#else
    std::cout << " " << (int)(bstrides[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
  print("  BOrigin:");
#else
  std::cout << std::endl << "  BOrigin:";
#endif
  for (svar<int> r = 0; r < Rank; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(borigin[r]);    
#else
    std::cout << " " << (int)(borigin[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
#else
  std::cout << std::endl;
#endif
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin) :
  bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)) {
  static_assert(!MultiDimRepr);
#ifndef UNSTAGED
    this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
#else
  this->allocator = HeapArray<Elem>(reduce<MulFunctor>(bextents));
#endif
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents) :
  bextents(std::move(bextents)) {
  static_assert(!MultiDimRepr);
  for (svar<int> i = 0; i < Rank; i=i+1) {
    bstrides[i] = 1;
    borigin[i] = 0;
  }
#ifndef UNSTAGED
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
#else
  this->allocator = HeapArray<Elem>(reduce<MulFunctor>(std::move(bextents)));
#endif
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents, 
				    Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) :
  bextents(std::move(bextents)), allocator(std::move(allocator)) {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    bstrides[i] = 1;
    borigin[i] = 0;
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::heap(SLoc_T bextents) {
#ifdef UNSTAGED
  static_assert(!MultiDimRepr);
#endif
  return Block<Elem,Rank,MultiDimRepr>(std::move(bextents));
}    

#ifndef UNSTAGED
template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <loop_type...Extents>
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::stack() {
  static_assert(Rank == sizeof...(Extents));
  auto allocator = std::make_shared<StackAllocation<Elem,mul_reduce<Extents...>()>>();
  SLoc_T bextents{Extents...};// = to_Loc_T<Extents...>();
  return Block<Elem,Rank,MultiDimRepr>(bextents, allocator);
}
#endif

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
#ifndef UNSTAGED
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::user(SLoc_T bextents, 
								dvar<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P> user) {  
#else
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::user(SLoc_T bextents, 
								dvar<Elem*> user) {  
#endif
#ifdef UNSTAGED
  static_assert(!MultiDimRepr);
#else
  // make sure that the dyn_var passed in matches the actual storage type (seems like it's not caught otherwise?)
  static_assert((MultiDimRepr && peel<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>() == Rank) || 
		(!MultiDimRepr && peel<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>() == 1));
#endif
#ifndef UNSTAGED
  if constexpr (MultiDimRepr) {
    auto allocator = std::make_shared<UserAllocation<Elem,typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P,Rank>>(user);
    return Block<Elem,Rank,MultiDimRepr>(std::move(bextents), allocator);
  } else {
#endif
#ifndef UNSTAGED
    auto allocator = std::make_shared<UserAllocation<Elem,Elem*,1>>(user);
#else
    auto allocator = HeapArray<Elem>(user);
#endif
    return Block<Elem,Rank,MultiDimRepr>(std::move(bextents), std::move(allocator));
#ifndef UNSTAGED
  }
#endif
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <unsigned long N>
dvar<Elem> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::read(darr<loop_type,N> &coords) {
  if constexpr (N < NUnfrozen) {
    // we need padding at the front
    constexpr int pad_amt = NUnfrozen-N;
    darr<loop_type,NUnfrozen> arr;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (svar<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    return this->read(arr);
  } else {
    if constexpr (N == NUnfrozen) {
      // for my own sanity
      static_assert(N+NFrozen == NLogical_T, "Jess, your sanity failed!");
      // add on any frozen dims    
      darr<loop_type,NLogical_T> full_coords;
      for (svar<int> i = 0; i < NFrozen; i=i+1) {
	full_coords[i] = frozen[i];
      }
      for (svar<int> i = 0; i < N; i=i+1) {
	full_coords[i+NFrozen] = coords[i];
      }
      // first get the global location
      // bi0 = vi0 * vstride0 + vorigin0
      darr<loop_type,NLogical_T> bcoords;
      compute_absolute_location<0>(full_coords, bcoords);
      // now adjust to make it relative to the block
      for (svar<int> i = 0; i < N; i=i+1) {
	bcoords[i] = (bcoords[i] - borigin[i]) / bstrides[i];
      }
      // then linearize with respect to the block
#ifndef UNSTAGED
      if constexpr (MultiDimRepr==true) {
	return allocator->read(bcoords);
      } else {
	dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
	darr<loop_type,1> arr{lidx};
	return allocator->read(arr);
      }
#else
      dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
      return allocator[lidx];
#endif
    } else {
      static_assert(N == NLogical_T);
      // manually specified all the dimensions (can happen with ref reads)
      // first get the global location
      // bi0 = vi0 * vstride0 + vorigin0
      darr<loop_type,NLogical_T> bcoords;
      compute_absolute_location<0>(coords, bcoords);
      // now adjust to make it relative to the block
      for (svar<int> i = 0; i < N; i=i+1) {
	bcoords[i] = (bcoords[i] - borigin[i]) / bstrides[i];
      }
      // then linearize with respect to the block
#ifndef UNSTAGED
      if constexpr (MultiDimRepr==true) {
	return allocator->read(bcoords);
      } else {
	dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
	darr<loop_type,1> arr{lidx};
	return allocator->read(arr);
      }
#else
      dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
      return allocator[lidx];
#endif
    }
  }
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename...Coords>
dvar<Elem> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::operator()(Coords...coords) {
  // it's possible to pass a dyn_arr in here and have it not complain (but screw everything up).
  // so verify that indices are valid.
  // can be either loop type or dyn_var<loop_type>
  validate_idx_types(coords...);
  darr<loop_type,sizeof...(Coords)> arr{coords...};
  return this->read(arr);
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename ScalarElem, unsigned long N>
void View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::write(ScalarElem val, darr<loop_type,N> &coords) {
  if constexpr (N < NUnfrozen) {
    // we need padding at the front
    constexpr int pad_amt = NUnfrozen-N;
    darr<loop_type,NUnfrozen> arr;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (svar<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    this->write(val, std::move(arr));
  } else {
    if constexpr (N == NUnfrozen) {
      // have to add on the frozen parameters
      // for my own sanity
      static_assert(N+NFrozen == NLogical_T, "Jess, your sanity failed!");
      // add on any frozen dims    
      darr<loop_type,NLogical_T> full_coords;
      for (svar<int> i = 0; i < NFrozen; i=i+1) {
	full_coords[i] = frozen[i];
      }
      for (svar<int> i = 0; i < N; i=i+1) {
	full_coords[i+NFrozen] = coords[i];
      }
      // the iters are relative to the View, so first make them relative to the block
      // bi0 = vi0 * vstride0 + vorigin0
      darr<loop_type,NLogical_T> bcoords;
      compute_absolute_location<0>(full_coords, bcoords);
      // now adjust to make it relative to the block
      for (svar<int> i = 0; i < N; i=i+1) {
	bcoords[i] = (bcoords[i] - borigin[i]) / bstrides[i];
      }
      // then linearize with respect to the block
#ifndef UNSTAGED
      if constexpr (MultiDimRepr==true) {
	allocator->write(val, bcoords);
      } else {
	dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
	darr<loop_type,1> arr{lidx};
	allocator->write(val, arr);
      }
#else
      dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
      allocator.write(lidx, val);
#endif
    } else {
      static_assert(N == NLogical_T);
      // manually specified all the dimensions (can happen with ref writes)
      darr<loop_type,NLogical_T> bcoords;
      compute_absolute_location<0>(coords, bcoords);
      // now adjust to make it relative to the block
      for (svar<int> i = 0; i < N; i=i+1) {
	bcoords[i] = (bcoords[i] - borigin[i]) / bstrides[i];
      }
      // then linearize with respect to the block
#ifndef UNSTAGED
      if constexpr (MultiDimRepr==true) {
	allocator->write(val, bcoords);
      } else {
	dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
	darr<loop_type,1> arr{lidx};
	allocator->write(val, arr);
      }
#else
      dvar<loop_type> lidx = linearize<0,NLogical_T>(this->bextents, bcoords);
      allocator.write(lidx, val);
#endif
    }
  }
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename Idx>
auto View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    dvar<loop_type> didx = idx;
    // include any frozen dims here 
    return Ref<View<Elem,NUnfrozen,MultiDimRepr,NFrozen>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    // include any frozen dims here
    return Ref<View<Elem,NUnfrozen,MultiDimRepr,NFrozen>,std::tuple<decltype(idx)>>(*this, std::tuple{idx});
  }
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename LIdx>
dvar<Elem> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::plidx(LIdx lidx) {
  // must delinearize relative to the view, but need to only do it for the unfrozen part
  darr<loop_type, NUnfrozen> unfrozen_extents;  
  darr<loop_type, NUnfrozen> unfrozen_coords;
  for (svar<int> i = NFrozen; i < NLogical_T; i=i+1) {
    unfrozen_extents[i-NFrozen] = vextents[i];
  }
  delinearize<0,NUnfrozen>(unfrozen_coords, lidx, std::move(unfrozen_extents));
  return this->read(unfrozen_coords);
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,NUnfrozen,MultiDimRepr,0> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::colocate(Block<Elem2,NLogical_T,MultiDimRepr2> &block) {
  static_assert(NFrozen==0, "Cannot do colocation on a View with frozen dimensions");
  // adjust by the factors
  SLoc_T extents;
  SLoc_T origin;
  for (svar<int> i = 0; i < NLogical_T; i=i+1) {
    extents[i] = block.bextents[i] * this->interpolation_factors[i];
    origin[i] = block.borigin[i] * this->interpolation_factors[i];
  }
  return {this->bextents, this->bstrides, this->borigin, 
    std::move(extents), block.bstrides, std::move(origin), 
    this->interpolation_factors, 
    this->allocator};
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,NUnfrozen,MultiDimRepr,0> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::colocate(View<Elem2,NLogical_T,MultiDimRepr2,0> &view) {
  static_assert(NFrozen==0, "Cannot do colocation on a View with frozen dimensions");
  // no adjustment by interpolation factors needed since you assume the interpolation factors make it 
  // so you are relative to the view param already! 
  // example: view=a macroblock and this=predication status. you have one intraprediction per macroblock,
  // so we'd assume this has already been interpolated by 16x16 so it has the same granularity as the
  // macroblock.
  return {this->bextents, this->bstrides, this->borigin,
    view.vextents, view.vstrides, view.vorigin, 
    this->interpolation_factors, 
    this->allocator};
} 

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename...Factors>
View<Elem,NUnfrozen,MultiDimRepr,0> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::logically_interpolate(Factors...factors) {
  static_assert(NFrozen==0, "Cannot do logical interpolation on a View with frozen dimensions.");
  SLoc_T ifactors{factors...};
  SLoc_T new_vextents;
  SLoc_T new_vorigin;
  for (svar<int> i = 0; i < NLogical_T; i=i+1) {
    new_vextents[i] = this->vextents[i] * ifactors[i];
    new_vorigin[i] = this->vorigin[i] * ifactors[i];
    ifactors[i] = ifactors[i] * this->interpolation_factors[i];
  }  
  return {this->bextents, this->bstrides, this->borigin,
    new_vextents, this->vstrides, new_vorigin,
    ifactors,
    this->allocator};
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
View<Elem,NUnfrozen,MultiDimRepr,NFrozen> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::reset() {
  SLoc_T ones;
  SLoc_T adjusted_vextents;
  SLoc_T adjusted_vorigin;
  for (svar<int> i = 0; i < NLogical_T; i=i+1) {
    ones[i] = 1;
    adjusted_vextents[i] = vextents[i] / this->interpolation_factors[i];
    adjusted_vorigin[i] = vorigin[i] / this->interpolation_factors[i];
  }
  return {this->bextents, this->bstrides, this->borigin,
    std::move(adjusted_vextents), this->vstrides, std::move(adjusted_vorigin),
    std::move(ones),
    this->allocator};
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename...Slices>
View<Elem,NUnfrozen,MultiDimRepr,NFrozen> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::view(Slices...slices) {
  static_assert(sizeof...(Slices) == NUnfrozen, "Must specify slices for all unfrozen dimensions.");
  // the block parameters stay the same, but we need to update the
  // view parameters  
  // extract just the unfrozen extents
  using Unfrozen_T = Loc_T<NUnfrozen>;
  Unfrozen_T unfrozen_extents;
  for (svar<int> i = 0; i < NUnfrozen; i=i+1) {
    unfrozen_extents[i] = this->vextents[i+NFrozen];
  }
  Unfrozen_T unfrozen_vstops;
  gather_stops<0,NUnfrozen>(unfrozen_vstops, unfrozen_extents, slices...);
  Unfrozen_T unfrozen_vstrides;
  gather_strides<0,NUnfrozen>(unfrozen_vstrides, slices...);
  Unfrozen_T unfrozen_vorigin;
  gather_origin<0,NUnfrozen>(unfrozen_vorigin, slices...);
  // convert vstops into extents
  Unfrozen_T unfrozen_vextents;
  convert_stops_to_extents<NUnfrozen>(unfrozen_vextents, unfrozen_vorigin, 
				 unfrozen_vstops, unfrozen_vstrides);
  // stick on dummy frozen dimensions for everything
  SLoc_T vstrides;
  SLoc_T vorigin;
  SLoc_T vextents;
  for (svar<int> i = 0; i < NFrozen; i=i+1) {
    vorigin[i] = 0; // don't apply the frozen dimensions here because those carry along with this sliced out view
    vstrides[i] = 1;
    vextents[i] = 1;
  }
  for (svar<int> i = NFrozen; i < NLogical_T; i=i+1) {
    vorigin[i] = unfrozen_vorigin[i-NFrozen];
    vstrides[i] = unfrozen_vstrides[i-NFrozen];
    vextents[i] = unfrozen_vextents[i-NFrozen];
  }
  // now make everything relative to the prior view
  // new origin = old origin + vorigin * old strides
  SLoc_T origin;
  SLoc_T tmp;
  apply<MulFunctor,NUnfrozen>(tmp, vorigin, this->vstrides);
  apply<AddFunctor,NUnfrozen>(origin, this->vorigin, tmp);
  // new strides = old strides * new strides
  SLoc_T strides;
  apply<MulFunctor,NUnfrozen>(strides, this->vstrides, vstrides);
  return View<Elem,NUnfrozen,MultiDimRepr,NFrozen>(this->bextents, this->bstrides, this->borigin,
					    std::move(vextents), std::move(strides), std::move(origin),
					    this->interpolation_factors,
					    this->allocator,
					    this->frozen);
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <int Depth>
void View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::compute_absolute_location(const darr<loop_type,NLogical_T> &coords,
									 darr<loop_type,NLogical_T> &out) {
  if constexpr (Depth == NLogical_T) {
  } else {
    out[Depth] = (coords[Depth] * vstrides[Depth] + vorigin[Depth]) / this->interpolation_factors[Depth];
    compute_absolute_location<Depth+1>(coords, out);
  }
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename...Coords>
View<Elem,NUnfrozen-sizeof...(Coords),MultiDimRepr,NFrozen+sizeof...(Coords)> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::freeze(Coords...coords) {
  Loc_T<sizeof...(Coords)> these_frozen{coords...};
  Loc_T<sizeof...(Coords)+NFrozen> frozen;
  for (svar<int> i = 0; i < NFrozen; i=i+1) {
    frozen[i] = this->frozen[i];
  }
  for (svar<int> i = 0; i < sizeof...(Coords); i=i+1) {
    frozen[i+NFrozen] = these_frozen[i];
  }
  return {this->bextents, this->bstrides, this->borigin,
    this->vextents, this->vstrides, this->vorigin,
    this->interpolation_factors,
    this->allocator,
    std::move(frozen)
  };
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename T>
void View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(NUnfrozen<=3, "dump_data only supports up to 3 unfrozen dimensions for printing.");
  if constexpr (NUnfrozen == 1) {
    for (dvar<loop_type> i = 0; i < vextents[NFrozen]; i=i+1) {
#ifndef UNSTAGED
      dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i)));
#else
      std::cout << (Elem)(this->operator()(i)) << " ";
#endif
    }
  } else if constexpr (NUnfrozen == 2) {
    for (dvar<loop_type> i = 0; i < vextents[NFrozen]; i=i+1) {
      for (dvar<loop_type> j = 0; j < vextents[NFrozen+1]; j=j+1) {
#ifndef UNSTAGED
	dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j)));
#else
	std::cout << (Elem)(this->operator()(i,j)) << " ";
#endif
      }
#ifndef UNSTAGED
      print_newline();
#else
      std::cout << std::endl;
#endif
    }
  } else {
    for (dvar<loop_type> i = 0; i < vextents[NFrozen]; i=i+1) {
      for (dvar<loop_type> j = 0; j < vextents[NFrozen+1]; j=j+1) {
	for (dvar<loop_type> k = 0; k < vextents[NFrozen+2]; k=k+1) {
#ifndef UNSTAGED
	  dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j,k)));
#else
	std::cout << (Elem)(this->operator()(i,j,k)) << " ";
#endif
	}
#ifndef UNSTAGED
	print_newline();
#else
	std::cout << std::endl;
#endif
      }
#ifndef UNSTAGED
      print_newline();
      print_newline();
#else
      std::cout << std::endl << std::endl;
#endif
    }
  }
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
void View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::dump_loc() {
#ifndef UNSTAGED
  print("View location info");
  print_newline();
  print("  Underlying data structure: ");
  std::string x = ElemToStr<typename decltype(ptr_wrap<Elem,NUnfrozen,MultiDimRepr>())::P>::str;
  print(x);
  print_newline();
#else
  std::cout << ("View location info") << std::endl;
  std::cout << ("  Underlying data structure: ");
  std::string x = ElemToStr<Elem*>::str;
  std::cout << x << std::endl;
#endif
  std::string rank = std::to_string(NUnfrozen);
  std::string nfrozen = std::to_string(NFrozen);
#ifndef UNSTAGED
  print("  Unfrozen dimensions: ");
  print(rank);
  print_newline();
  print("  NFrozen dimensions: ");
  print(nfrozen);
  print_newline();
  print("  BExtents:");
#else
  std::cout << "  Unfrozen dimensions: " << rank << std::endl;
  std::cout << "  NFrozen dimensions: " << nfrozen << std::endl;
  std::cout << "  BExtents";
#endif
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(bextents[r]);    
#else
    std::cout << "  " << (int)(bextents[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
  print("  BStrides:");
#else
  std::cout << "  BStrides:";
#endif
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(bstrides[r]);    
#else
    std::cout << "  " << (int)(bstrides[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
  print("  BOrigin:");
#else
  std::cout << std::endl << "  BOrigin:";
#endif
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(borigin[r]);    
#else
    std::cout << " " << (int)(borigin[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
  print("  Interpolated VExtents:");
#else
  std::cout << std::endl << "  Interpolated VExtents:";
#endif
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
#ifndef UNSTAGED
    print(" ");
    dispatch_print_elem<int>(vextents[r]);    
#else
    std::cout << " " << (int)(vextents[r]);
#endif
  }
#ifndef UNSTAGED
  print_newline();
  print("  UnInterpolated VExtents:");
#else
  std::cout << std::endl << "  UnInterpolated VExtents:";
#endif
// TODO do the rest of this. I'm lazy right now
#ifndef UNSTAGED
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vextents[r] / interpolation_factors[r]);    
  }
  print_newline();
  print("  VStrides:");
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vstrides[r]);    
  }
  print_newline();
  print("  VOrigin:");
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vorigin[r]);    
  }  
  print_newline();
  print("  UnInterpolated VOrigin:");
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vorigin[r] / interpolation_factors[r]);    
  }  
  print_newline();
  print("  Interpolation factors:");
  for (svar<int> r = 0; r < NLogical_T; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(interpolation_factors[r]);    
  }  
  print_newline();
  print("  NFrozen dimensions:");
  for (svar<int> r = 0; r < NFrozen; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(frozen[r]);    
  }  
  print_newline();
#endif
}

template <typename Elem, unsigned long NUnfrozen, bool MultiDimRepr, unsigned long NFrozen>
template <typename...Coords>
dvar<bool> View<Elem,NUnfrozen,MultiDimRepr,NFrozen>::logically_exists(Coords...coords) {
  static_assert(sizeof...(coords) == NUnfrozen, "No default padding allowed for logically_exists");
  // stick on the frozen coordinates
  Loc_T<NUnfrozen> unfrozen_coords{coords...};
  Loc_T<NLogical_T> full_coords;
  for (svar<int> i = 0; i < NFrozen; i=i+1) {
    full_coords[i] = frozen[i];
  }
  for (svar<int> i = 0; i < NUnfrozen; i=i+1) {
    full_coords[i+NFrozen] = unfrozen_coords[i];
  }
  Loc_T<NLogical_T> rel;
  compute_absolute_location<0>(full_coords, rel);
  for (svar<loop_type> i = 0; i < NLogical_T; i++) {
    if (rel[i] < 0) {
      return false; 
    }
  }
  return true;
}

//
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
    dvar<loop_type> didx = idx;
    using DIdx = decltype(didx);
    typename TupleTypeCat<DIdx,Idxs>::type args = std::tuple_cat(idxs, std::tuple{didx});
    return Ref<BlockLike,decltype(args)>(block_like, std::move(args));
  } else {
    typename TupleTypeCat<Idx,Idxs>::type args = std::tuple_cat(idxs, std::tuple{idx});
    return Ref<BlockLike,decltype(args)>(block_like, std::move(args));
  }
}

template <typename BlockLike, typename Idxs>
Ref<BlockLike, Idxs> &Ref<BlockLike,Idxs>::operator=(Ref<BlockLike, Idxs> &rhs) {
  // force it to the not copy-assignment operator=
  *this = BlockLike::Elem_T(1) * rhs;    
  return *this;
}

template <typename BlockLike, typename Idxs>
template <typename Rhs>
void Ref<BlockLike,Idxs>::internal_assign(Rhs rhs) {
  if constexpr (/*is_view<BlockLike>::value && */std::tuple_size<Idxs>() < BlockLike::NLogical_T) {
    // need frozen dimensions and/or padding
    // get frozen dimensions
    auto frozen = dyn_arr_to_tuple<BlockLike::NFrozen_T,0>(block_like.frozen);
    // get any padding dimensions
    constexpr int pad_amt = BlockLike::NUnfrozen_T - std::tuple_size<Idxs>();
    darr<loop_type,pad_amt> padded;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      padded[i] = 0;
    }
    // now combine everything
    static_assert(BlockLike::NFrozen_T + pad_amt + std::tuple_size<Idxs>() == BlockLike::NLogical_T); // sanity check
    auto new_idxs = std::tuple_cat(frozen, dyn_arr_to_tuple<pad_amt,0>(std::move(padded)), this->idxs);
    auto new_ref = Ref<BlockLike, decltype(new_idxs)>(this->block_like, std::move(new_idxs));
    new_ref.realize_loop_nest(rhs);
  } else {
    // no padding or unfreezing needed (you may have frozen dimensions, but they would be overridden in this case)
    realize_loop_nest(rhs);
  }
}

template <unsigned long NIdxs, unsigned long NLogical, unsigned long NFrozen, unsigned long NUnfrozen>
constexpr bool verify_n_idxs() {
  if constexpr (NIdxs == NLogical) {
    // specified all the idxs, no freezing or padding needed
    return true;
  } else if constexpr (NIdxs > NLogical) {
    // too many
    return false;
  } else {
    // if you specify less, first the frozen are added, and then the padding
    constexpr unsigned long pad_amt = NUnfrozen - NIdxs;
    constexpr unsigned long padded_frozen = NFrozen + pad_amt;
    if constexpr (padded_frozen + NIdxs != NLogical) {
      return false;
    } else {
      return true;
    }
  }
}
  
// raw value
template <typename BlockLike, typename Idxs>
void Ref<BlockLike,Idxs>::operator=(typename BlockLike::Elem_T x) {
  this->verify_unadorned();
  this->verify_unique<0>();
  // TODO can potentially memset this whole thing
  constexpr unsigned long s = std::tuple_size<Idxs>();
  static_assert(verify_n_idxs<s, BlockLike::NLogical_T, BlockLike::NFrozen_T, BlockLike::NUnfrozen_T>(),
		"Invalid number of indices specified. Please see staged_blocklike.h for valid indexing schemes.");
//  static_assert(std::tuple_size<Idxs>() <= BlockLike::NLogical_T);
  internal_assign(x);
}

template <typename BlockLike, typename Idxs>
template <typename Rhs>
void Ref<BlockLike,Idxs>::operator=(Rhs rhs) {
  this->verify_unadorned();
  this->verify_unique<0>();
  constexpr unsigned long s = std::tuple_size<Idxs>();
  static_assert(verify_n_idxs<s, BlockLike::NLogical_T, BlockLike::NFrozen_T, BlockLike::NUnfrozen_T>(),
		"Invalid number of indices specified. Please see staged_blocklike.h for valid indexing schemes.");
//  static_assert(std::tuple_size<Idxs>() <= BlockLike::NLogical_T);
  internal_assign(rhs);
}

template <typename BlockLike, typename Idxs>
//template <typename Rhs, typename std::enable_if<is_dyn_like<Rhs>::value, int>::type>
void Ref<BlockLike,Idxs>::operator=(builder::builder rhs) {
  // in the even Rhs is a builder::builder, force that do dyn_var! things go very wrong otherwise
  dvar<typename BlockLike::Elem_T> rhs2 = rhs;
  this->verify_unadorned();
  this->verify_unique<0>();
  constexpr unsigned long s = std::tuple_size<Idxs>();
  static_assert(verify_n_idxs<s, BlockLike::NLogical_T, BlockLike::NFrozen_T, BlockLike::NUnfrozen_T>(),
		"Invalid number of indices specified. Please see staged_blocklike.h for valid indexing schemes.");
//  static_assert(std::tuple_size<Idxs>() <= BlockLike::NLogical_T);
  internal_assign(rhs2);
}

template <typename BlockLike, typename Idxs>
template <int Iter, int IdxDepth, typename LhsIdxs, unsigned long N>
void Ref<BlockLike,Idxs>::realize_each(LhsIdxs lhs, 
				       const darr<loop_type,N> &iters,
				       darr<loop_type,BlockLike::NLogical_T> &out) {
  constexpr unsigned long s = std::tuple_size<Idxs>();
  static_assert(verify_n_idxs<s, BlockLike::NLogical_T, BlockLike::NFrozen_T, BlockLike::NUnfrozen_T>(),
		"Invalid number of indices specified. Please see staged_blocklike.h for valid indexing schemes.");
  if constexpr (Iter == 0 && is_view<BlockLike>::value && std::tuple_size<Idxs>() < BlockLike::NLogical_T) {
    // add frozen dimensions
    for (svar<int> i = 0; i < BlockLike::NFrozen_T; i=i+1) {
      out[i] = block_like.frozen[i];
    }
    // add any padding dimensions
    constexpr int pad_amt = BlockLike::NUnfrozen_T - std::tuple_size<Idxs>();
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      out[i+BlockLike::NFrozen_T] = 0;
    }
    // skip ahead 
    realize_each<BlockLike::NFrozen_T+pad_amt,IdxDepth>(lhs, iters, out);
  } else if constexpr (Iter == 0 && std::tuple_size<Idxs>() < BlockLike::NLogical_T) { 
    // need padding
    constexpr int pad_amt = BlockLike::NLogical_T - std::tuple_size<Idxs>();
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      out[i] = 0;
    }
    // skip ahead 
    realize_each<pad_amt,IdxDepth>(lhs, iters, out);
  } else {
    auto i = std::get<IdxDepth>(idxs); // these are the rhs indexes!
    if constexpr (is_dyn_like<decltype(i)>::value ||
		  std::is_same<decltype(i),loop_type>()) {
      // this is just a plain value--use it directly
      if constexpr (Iter < BlockLike::NLogical_T - 1) {
	out[Iter] = i;
	realize_each<Iter+1,IdxDepth+1>(lhs, iters, out);
      } else {
	out[Iter] = i;
      }
    } else {
      auto r = i.realize(lhs, iters);
      if constexpr (Iter < BlockLike::NLogical_T - 1) {
	out[Iter] = r;
	realize_each<Iter+1,IdxDepth+1>(lhs, iters, out);
      } else {
	out[Iter] = r;
      }
    }
  }
}
  
template <typename BlockLike, typename Idxs>
template <typename LhsIdxs, unsigned long N>
dvar<typename BlockLike::Elem_T> Ref<BlockLike,Idxs>::realize(LhsIdxs lhs, 
									  const darr<loop_type,N> &iters) {
  // first realize each idx
  // use NUnfrozen_T instead of N b/c the rhs refs may be diff
  // dimensions than lhs
  darr<loop_type,BlockLike::NLogical_T> arr; 
  realize_each<0,0>(lhs, iters, arr);
  // then access the thing  
  return block_like.read(arr);
}

template <typename BlockLike, typename Idxs>
template <typename Rhs, typename...Iters>
void Ref<BlockLike,Idxs>::realize_loop_nest(Rhs rhs, Iters...iters) {
  // the lhs indices can be either an Iter or an integer.
  // Iter -> loop over whole extent
  // integer -> single iteration loop
  constexpr int rank = BlockLike::NLogical_T;
  constexpr int depth = sizeof...(Iters);
  if constexpr (depth < rank) {
    auto dummy = std::get<depth>(idxs);
    if constexpr (std::is_integral<decltype(dummy)>() ||
		  is_dyn_like<decltype(dummy)>::value) {
	// single iter
	dvar<loop_type> iter = std::get<depth>(idxs);
	realize_loop_nest(rhs, iters..., iter);
    } else {
      // loop
      if constexpr (BlockLike::IsBlock_T) {
	for (dvar<loop_type> iter = 0; iter < block_like.bextents[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      } else {
	for (dvar<loop_type> iter = 0; iter < block_like.vextents[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      }
    }
  } else {
    // at the innermost level
    darr<loop_type,sizeof...(Iters)> arr{iters...};
    if constexpr (std::is_arithmetic<Rhs>() ||
		  is_dyn_like<Rhs>::value) {
      block_like.write(rhs, arr);
    } else {      
      block_like.write(rhs.realize(idxs, arr), arr);
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
  if constexpr (Depth < std::tuple_size<Idxs>() - 1) {
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
    if constexpr (Depth < std::tuple_size<Idxs>() - 1) {
      verify_unique<Depth+1, Seen..., T::Ident_T>();
    }
  } else {
    if constexpr (Depth < std::tuple_size<Idxs>()- 1) {
      verify_unique<Depth+1, Seen...>();
    }
  }
}  


}
