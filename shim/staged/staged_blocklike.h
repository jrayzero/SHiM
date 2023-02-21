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
#include "annotations.h"
#include "location.h"
#ifdef UNSTAGED
#include "runtime/cpp/heaparray.h"
#endif

namespace shim {

// TODO use within block and view to hold the info so I don't need to keep retyping it
// Also allowing just building one of these manually and passing into a block/view constructor

#ifndef UNSTAGED
template <typename Elem, int PhysicalRank>
using Allocation_T = std::shared_ptr<Allocation<Elem,PhysicalRank>>;
#else
template <typename Elem, int PhysicalRank>
using Allocation_T = HeapArray<Elem>;
#endif

// TODO Everything except the origin should really be an unsigned integer.

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

// 4. Unstaging only supports heap arrays. no stacks, multidimensional arrays, etc

template <typename Elem, unsigned long Rank, bool MultiDimRepr=false>
struct Block {
  
  template <typename Elem2, unsigned long Rank2, bool MultiDimRepr2>
  friend struct View;

  template <unsigned long Rank2>
  friend struct MeshLocation;

  template <typename BlockLike, typename Idxs>
  friend struct Ref;

  using SLoc_T = Loc_T<Rank>;
  static constexpr unsigned long Rank_T = Rank;
  using Elem_T = Elem;
  static constexpr bool IsBlock_T = true;  

  ///
  /// Create an internally-managed heap Block
  Block(MeshLocation<Rank> location);

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin);

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, 
	SLoc_T brefinement_factors, SLoc_T bcoarsening_factors);

  ///
  /// Create an internally-managed heap Block
  Block(SLoc_T bextents);

  ///
  /// Create an internally-managed heap Block
  static Block<Elem,Rank,MultiDimRepr> heap(SLoc_T bextents);

#ifndef UNSTAGED
  ///
  /// Create an internally-managed stack Block
  template <int...Extents>
  static Block<Elem,Rank,MultiDimRepr> stack();

  ///
  /// Create an internally-managed stack Block. 
  /// Must still specify extents here as a template parameter (any extents in location will
  /// be overriden).
  template <int...Extents>
  static Block<Elem,Rank,MultiDimRepr> stack(MeshLocation<Rank> location);

  ///
  /// Create an internally-managed stack Block
  template <int...Extents>
  static Block<Elem,Rank,false> stack(SLoc_T bstrides, SLoc_T borigin, 
				      SLoc_T brefinement_factors, SLoc_T bcoarsening_factors);
#endif

  ///
  /// Create a user-managed allocation
#ifndef UNSTAGED
  static Block<Elem,Rank,MultiDimRepr> user(SLoc_T bextents, 
					    dvar<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P> user);
  static Block<Elem,Rank,MultiDimRepr> user(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
					    SLoc_T brefinement_factors, SLoc_T bcoarsening_factors,
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
  View<Elem,Rank,MultiDimRepr> colocate(Block<Elem2,Rank,MultiDimRepr2> &block);

  ///
  /// Create a view on this' data using the location of view (not its underlying block!)
  template <typename Elem2, bool MultiDimRepr2>
  View<Elem,Rank,MultiDimRepr> colocate(View<Elem2,Rank,MultiDimRepr2> &view);

  /// 
  /// Create refinement factors that logically increase the extent of this block.
  template <typename...Factors>
  View<Elem,Rank,MultiDimRepr> virtually_refine(Factors...factors);

  /// 
  /// Create refinement factors that physically increase the extent of this block.
  template <typename...Factors>
  Block<Elem,Rank,false> physically_refine(Factors...factors);

  /// Create coarsening factors that physically decrease the extent of this block.
  template <typename...Factors>
  Block<Elem,Rank,false> physically_coarsen(Factors...factors);

  ///
  /// Print out the location of this block within the base mesh space
  void dump_base_mesh_space_location();

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, unsigned long N>
  void write(ScalarElem val, darr<loop_type,N> &iters);

  ///
  /// Create a View over this whole block
  View<Elem,Rank,MultiDimRepr> view();

  ///
  /// Slice out a View over a portion of this Block
  template <typename...Slices>
  View<Elem,Rank,MultiDimRepr> view(Slices...slices);

  /// 
  /// Perform a lazy inline access on this Block
  template <typename Idx>
  Ref<Block<Elem,Rank,MultiDimRepr>,std::tuple<typename RefIdxType<Idx>::type>> operator[](Idx idx);

  /// 
  /// Generate code for printing a rank 1, 2, or 3 Block.
  template <typename T=Elem>
  void dump_data();

  ///
  /// print out the location info
  void dump_loc();

  /// 
  /// Attach permutation
  template <typename T>
  View<Elem,Rank,MultiDimRepr> permute(std::initializer_list<T> value);
  View<Elem,Rank,MultiDimRepr> row_major();
  View<Elem,Rank,MultiDimRepr> col_major();

  // TODO make this private
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator;
/*  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;
  SLoc_T brefinement_factors;
  SLoc_T bcoarsening_factors;
  std::array<int,Rank> permuted_indices;*/

  MeshLocation<Rank> get_location() { return location; }

private:

  ///
  /// Create a Block with the specifed Allocation
  Block(SLoc_T bextents, Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator);

  ///
  /// Create a Block with the specifed Allocation
  Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
	SLoc_T brefinement_factors, SLoc_T bcoarsening_factors,
	Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator);

  MeshLocation<Rank> location;

};

///
/// A region of data with a location that shares its underlying data with another block or view
/// If MultiDimRepr=true, # physical dims == # logical dims
template <typename Elem, unsigned long Rank, bool MultiDimRepr=false>
struct View {

  template <unsigned long Rank2>
  friend struct MeshLocation;

  template <typename BlockLike, typename Idxs>
  friend struct Ref;

  using SLoc_T = Loc_T<Rank>;
  using Elem_T = Elem;
  static constexpr unsigned long Rank_T = Rank;
  static constexpr bool IsBlock_T = false;

  /// 
  /// Create a View from the specified location information
  View(MeshLocation<Rank> block_location, MeshLocation<Rank> view_location,
       Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator);


  /// 
  /// Create a View from the specified location information
  View(MeshLocation<Rank> block_location, MeshLocation<Rank> view_location,
       Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator,
       std::array<int,Rank> permuted_indices);
  
  /// 
  /// Create a View from the specified location information
  View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, SLoc_T brefinement_factors, SLoc_T bcoarsening_factors,
       SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin, SLoc_T vrefinement_factors, SLoc_T vcoarsening_factors,
       Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator);
  
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
  View<Elem,Rank,MultiDimRepr> colocate(Block<Elem2,Rank,MultiDimRepr2> &block);

  ///
  /// Create a view on this' data using the location of view (not its underlying block!)
  template <typename Elem2, bool MultiDimRepr2>
  View<Elem,Rank,MultiDimRepr> colocate(View<Elem2,Rank,MultiDimRepr2> &view);

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, unsigned long N>
  void write(ScalarElem val, darr<loop_type,N> &coords);

  /// 
  /// Create refinement factors that logically increase the extent of this block.
  template <typename...Factors>
  View<Elem,Rank,MultiDimRepr> virtually_refine(Factors...factors);

  /// 
  /// Create refinement factors that physically increase the extent of this block.
  template <typename...Factors>
  Block<Elem,Rank,false> physically_refine(Factors...factors);

  /// Create coarsening factors that physically decrease the extent of this view.
  template <typename...Factors>
  Block<Elem,Rank,false> physically_coarsen(Factors...factors);

  ///
  /// Print out the location of this view within the base mesh space
  void dump_base_mesh_space_location();

  ///
  /// Slice out a View over a portion of this View
  View<Elem,Rank,MultiDimRepr> view();

  ///
  /// Slice out a View over a portion of this View
  template <typename...Slices>
  View<Elem,Rank,MultiDimRepr> view(Slices...slices);

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
  /// Attach permutation
  template <typename T>
  View<Elem,Rank,MultiDimRepr> permute(std::initializer_list<T> value);
  View<Elem,Rank,MultiDimRepr> row_major();
  View<Elem,Rank,MultiDimRepr> col_major();

  ///
  /// True if global coords computed from the input coords are >= 0.
  /// Does not guarantee that the data physically exists in the underlying block.  
  template <typename...Coords>
  dvar<bool> logically_exists(Coords...coords);
    
  MeshLocation<Rank> get_block_location() { return block_location; }

  MeshLocation<Rank> get_view_location() { return view_location; }

  // TODO make this private
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator;
  
private:
  
  ///
  /// Compute the absolute indices of the coordinates. Can only be called once frozen dims are prepended.
  template <int Depth>
  void compute_block_mesh_space_location(const darr<loop_type,Rank> &coords,
					 darr<loop_type,Rank> &out);

  MeshLocation<Rank> block_location;
  MeshLocation<Rank> view_location;
  std::array<int,Rank> permuted_indices;
  
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
		    darr<loop_type,BlockLike::Rank_T> &out);
  
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
    // apply permutations
    darr<loop_type,Rank> permuted;
    for (int i = 0; i < Rank; i++) {
      permuted[i] = coords[i];
    }
    if constexpr (MultiDimRepr==true) {
      return allocator->read(permuted);
    } else {
      dvar<loop_type> lidx = linearize<0,Rank>(location.get_extents(), permuted);
      darr<loop_type,1> arr{lidx};
      return allocator->read(arr);
    }
#else
    darr<loop_type,Rank> permuted;
    for (int i = 0; i < Rank; i++) {
      permuted[i] = coords[i];
    }
    dvar<loop_type> lidx = linearize<0,Rank>(location.get_extents(), permuted);
    return allocator[lidx];
#endif
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
      dvar<loop_type> lidx = linearize<0,Rank>(location.get_extents(), coords);
      darr<loop_type,1> arr{lidx};
      allocator->write(val, arr);
    }
#else
    dvar<loop_type> lidx = linearize<0,Rank>(location.get_extents(), coords);
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
    delinearize<0,Rank>(idxs, lidx, location.get_extents());
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
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::colocate(Block<Elem2,Rank,MultiDimRepr2> &block) {
  // convert parameterization to the base mesh space
  auto base_loc = block.get_location().compute_base_mesh_location();
  // now put into this's mesh space
  auto mesh_loc = base_loc.into(location);
  return {location, mesh_loc, allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::colocate(View<Elem2,Rank,MultiDimRepr2> &view) {
  // convert parameterization to the base mesh space
  auto base_loc = view.get_view_location().compute_base_mesh_location();
  // now put into this's mesh space
  auto mesh_loc = base_loc.into(location);
  return {location, mesh_loc, allocator};
} 

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::virtually_refine(Factors...factors) {
  SLoc_T rfactors{factors...};
/*  SLoc_T vextents;
  SLoc_T vorigin;  
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (this->bstrides[i] != 1 && rfactors[i] > 1) {
      print("Cannot refine block with non-unit stride %d in dimension %d.\\n", this->bstrides[i], i);
      hexit(-1);
    }
#endif
    vextents[i] = this->bextents[i] * rfactors[i];
    vorigin[i] = this->borigin[i] * rfactors[i];
    rfactors[i] = this->brefinement_factors[i] * rfactors[i];
  }
  return {this->bextents, this->bstrides, this->borigin, this->brefinement_factors,
    this->bcoarsening_factors,
    std::move(vextents), this->bstrides, std::move(vorigin), std::move(rfactors),
    this->bcoarsening_factors,
    this->allocator};*/
  return {location, location.refine(rfactors), allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
Block<Elem,Rank,false> Block<Elem,Rank,MultiDimRepr>::physically_refine(Factors...factors) {
  SLoc_T rfactors{factors...};
  return {location.refine(rfactors)};
/*  SLoc_T new_bextents;
  SLoc_T new_borigin;  
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (this->bstrides[i] != 1 && rfactors[i] > 1) {
      print("Cannot refine block with non-unit stride %d in dimension %d.\\n", this->bstrides[i], i);
      hexit(-1);
    }
#endif
    new_bextents[i] = this->bextents[i] * rfactors[i];
    new_borigin[i] = this->borigin[i] * rfactors[i];
    rfactors[i] = this->brefinement_factors[i] * rfactors[i];
  }
//  return {std::move(new_bextents), this->bstrides, std::move(new_borigin), std::move(rfactors),
//    this->bcoarsening_factors,
//    this->allocator};*/
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
Block<Elem,Rank,false> Block<Elem,Rank,MultiDimRepr>::physically_coarsen(Factors...factors) {
  SLoc_T cfactors{factors...};
/*  SLoc_T new_bextents;
  SLoc_T new_borigin;  
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (this->bstrides[i] != 1 && rfactors[i] > 1) {
      print("Cannot coarsen block with non-unit stride %d in dimension %d.\\n", this->bstrides[i], i);
      hexit(-1);
    }
#endif
    new_bextents[i] = this->bextents[i] /  cfactors[i];
    new_borigin[i] = this->borigin[i] / cfactors[i];
    cfactors[i] = this->bcoarsening_factors[i] * cfactors[i];
  }
  return {std::move(new_bextents), this->bstrides, std::move(new_borigin), 
    this->refinement_factors, std::move(cfactors),
    this->allocator};*/
  return {location.coarsen(cfactors)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::view() {
  return {location, location, allocator};
/*  return {this->bextents, this->bstrides, this->borigin, this->brefinement_factors,
    this->bcoarsening_factors,
    this->bextents, this->bstrides, this->borigin, this->brefinement_factors,    
    this->bcoarsening_factors,
    this->allocator};*/
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Slices>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::view(Slices...slices) {
  // the block parameters stay the same, but we need to update the
  // view parameters  
  SLoc_T permuted_extents;
  for (int i = 0; i < Rank; i++) {
    permuted_extents[i] = location.get_extents()[i];
  }
  SLoc_T vstops_p;
  gather_stops<0,Rank>(vstops_p, permuted_extents, slices...);
  SLoc_T vstrides_p;
  gather_strides<0,Rank>(vstrides_p, slices...);
  SLoc_T vorigin_p;
  gather_origin<0,Rank>(vorigin_p, slices...);
  // now permute all of the above to the canonical form
  SLoc_T vstops;
  SLoc_T vstrides;
  SLoc_T vorigin;
  for (int i = 0; i < Rank; i++) {
    vstops[i] = vstops_p[i];
    vstrides[i] = vstrides_p[i];
    vorigin[i] = vorigin_p[i];
  }
  
  // convert vstops into extents
  // just use the raw values from the slice parameters
  SLoc_T vextents;
  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  // now we need to update the strides and origin based on the block's parameters
  // new strides = old strides * new strides
  SLoc_T strides;
  apply<MulFunctor,Rank>(strides, location.get_strides(), vstrides);
  for (svar<int> i = 0; i < Rank; i=i+1) {
    vorigin[i] = vorigin[i] * location.get_strides()[i] + location.get_origin()[i];
  }
  return {
    location,
//    this->bextents, this->bstrides, this->borigin, this->brefinement_factors, this->bcoarsening_factors,
    LocationBuilder<Rank>().with_extents(vextents).with_strides(strides).with_origin(vorigin).with_refinement(location.get_refinement_factors()).with_coarsening(location.get_coarsening_factors()).to_loc(),
    this->allocator};
  //    std::move(vextents), std::move(strides), std::move(vorigin), this->brefinement_factors, this->bcoarsening_factors,
//    this->allocator};  
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename T>
void Block<Elem,Rank,MultiDimRepr>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports ranks 1, 2, and 3");
  if constexpr (Rank == 1) {
    for (dvar<loop_type> i = 0; i < location.get_extents()[0]; i=i+1) {
      dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i)));
    }
  } else if constexpr (Rank == 2) {
    for (dvar<loop_type> i = 0; i < location.get_extents()[0]; i=i+1) {
      for (dvar<loop_type> j = 0; j < location.get_extents()[1]; j=j+1) {
	dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j)));
      }
      print_newline();
    }
  } else {
    for (dvar<loop_type> i = 0; i < location.get_extents()[0]; i=i+1) {
      for (dvar<loop_type> j = 0; j < location.get_extents()[1]; j=j+1) {
	for (dvar<loop_type> k = 0; k < location.get_extents()[2]; k=k+1) {
	  dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j,k)));
	}
	print_newline();
      }
      print_newline();
      print_newline();
    }
  }
}

/*template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void Block<Elem,Rank,MultiDimRepr>::dump_base_mesh_space_location() {
  SLoc_T base_extents;
  SLoc_T base_origin;
  SLoc_T base_strides;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    base_extents[i] = this->bextents[i] / this->brefinement_factors[i] * this->bcoarsening_factors[i];
    base_origin[i] = this->borigin[i] / this->brefinement_factors[i] * this->bcoarsening_factors[i];
    base_strides[i] = this->bstrides[i] / this->brefinement_factors[i] * this->bcoarsening_factors[i];
  }
  print("Block base mesh space location info");
  print_newline();
  print("  Base extents: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(base_extents[r]);    
  }  
  print("  Base origin: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(base_origin[r]);    
  }  
  print("  Base strides: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(base_strides[r]);    
  }
}*/

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void Block<Elem,Rank,MultiDimRepr>::dump_loc() {
  print("Block location info");
  print_newline();
  print("  Underlying data structure: ");
  print(ElemToStr<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>::str);
  print_newline();
  location.dump_location();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename T>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::permute(std::initializer_list<T> value) {
//  auto viewobj = this->view();
//  attach_permute(viewobj, value, items...);
//  return viewobj;
  return {location, location, allocator, tuple_to_arr<int>(initlist_to_tuple<Rank>(value))};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::row_major() {
//  auto viewobj = this->view();
  std::array<int, Rank_T> arr;
  for (int i = 0; i < Rank_T; i++) {
    arr[i] = i;
  }
//  attach_row_major(viewobj);
  return {location, location, allocator, arr};
//  return viewobj;
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::col_major() {
  std::array<int, Rank_T> arr;
  for (int i = 0; i < Rank_T; i++) {
    arr[i] = Rank_T - i - 1;
  }
  return {location, location, allocator, arr};
//  auto viewobj = this->view();
//  attach_col_major(viewobj);
//  return viewobj;
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(MeshLocation<Rank> location) : location(location) { 
  static_assert(!MultiDimRepr);
#ifndef UNSTAGED
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(location.get_extents()));
#else
  this->allocator = HeapArray<Elem>(reduce<MulFunctor>(location.get_extents()));
#endif
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin) :
  location(LocationBuilder<Rank>().
	   with_extents(bextents).
	   with_strides(bstrides).
	   with_origin(borigin).to_loc()) { // :
//  bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)) {
  static_assert(!MultiDimRepr);
#ifndef UNSTAGED
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
#else
  this->allocator = HeapArray<Elem>(reduce<MulFunctor>(bextents));
#endif
/*  for (int i = 0; i < Rank; i=i+1) {
    permuted_indices[i] = i;
    brefinement_factors[i] = 1;
    bcoarsening_factors[i] = 1;
  }*/
//  this->location = LocationBuilder<Rank>().
//    with_extents(bextents).
//    with_strides(bstrides).
//    with_origin(borigin).to_loc();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, 
				     SLoc_T brefinement_factors, SLoc_T bcoarsening_factors) :
  location(LocationBuilder<Rank>().
	   with_extents(bextents).
	   with_strides(bstrides).
	   with_origin(borigin).
	   with_refinement(brefinement_factors).
	   with_coarsening(bcoarsening_factors).to_loc()) { //:
//  bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
//  brefinement_factors(std::move(brefinement_factors)), bcoarsening_factors(std::move(bcoarsening_factors)) {
  static_assert(!MultiDimRepr);
#ifndef UNSTAGED
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
#else
  this->allocator = HeapArray<Elem>(reduce<MulFunctor>(bextents));
#endif
//  for (int i = 0; i < Rank; i=i+1) {
//    permuted_indices[i] = i;
//  }
//  this->location = LocationBuilder<Rank>().
//    with_extents(bextents).
//    with_strides(bstrides).
//    with_origin(borigin).
//    with_refinement(brefinement_factors).
//    with_coarsening(bcoarsening_factors).to_loc();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents) : 
  location(LocationBuilder<Rank>().
	   with_extents(bextents).to_loc()) { //:
//  bextents(std::move(bextents)) {
  static_assert(!MultiDimRepr);
/*  for (svar<int> i = 0; i < Rank; i=i+1) {
    bstrides[i] = 1;
    borigin[i] = 0;
    brefinement_factors[i] = 1;
    bcoarsening_factors[i] = 1;
  }
  for (int i = 0; i < Rank; i=i+1) {
    permuted_indices[i] = i;
  }*/
#ifndef UNSTAGED
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
#else
  this->allocator = HeapArray<Elem>(reduce<MulFunctor>(std::move(bextents)));
#endif
//  this->location = LocationBuilder<Rank>().
//    with_extents(bextents).to_loc();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents, 
				     Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) : 
  location(LocationBuilder<Rank>().
	   with_extents(bextents).to_loc()), 
  allocator(std::move(allocator)) {
  //  bextents(std::move(bextents)), allocator(std::move(allocator)) {
/*  for (svar<int> i = 0; i < Rank; i=i+1) {
    bstrides[i] = 1;
    borigin[i] = 0;
    brefinement_factors[i] = 1;
    bcoarsening_factors[i] = 1;
  }
  for (int i = 0; i < Rank; i=i+1) {
    permuted_indices[i] = i;
  }*/
//  this->location = LocationBuilder<Rank>().
//    with_extents(bextents).to_loc();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr>::Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
				     SLoc_T brefinement_factors, SLoc_T bcoarsening_factors,
				     Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) :
  location(LocationBuilder<Rank>().
	   with_extents(bextents).
	   with_strides(bstrides).
	   with_origin(borigin).
	   with_refinement(brefinement_factors).
	   with_coarsening(bcoarsening_factors).to_loc()),
//  bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)), 
//  brefinement_factors(std::move(brefinement_factors)), bcoarsening_factors(std::move(bcoarsening_factors)),
  allocator(std::move(allocator)) {
//  for (int i = 0; i < Rank; i=i+1) {
//    permuted_indices[i] = i;
//  }
/*  this->location = LocationBuilder<Rank>().
    with_extents(bextents).
    with_strides(bstrides).
    with_origin(borigin).
    with_refinement(brefinement_factors).
    with_coarsening(bcoarsening_factors).to_loc();*/
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
template <int...Extents>
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::stack() {
  // This causes a buildit error if I do static_var (and also make the allocator take a static var)
  loop_type sz = mul_reduce<Extents...>();
  auto allocator = std::make_shared<StackAllocation<Elem>>(sz);
  SLoc_T extents{Extents...};
  return Block<Elem,Rank,MultiDimRepr>(extents, allocator);
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <int...Extents>
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::stack(MeshLocation<Rank> location) {
  // This causes a buildit error if I do static_var (and also make the allocator take a static var)
  loop_type sz = mul_reduce<Extents...>();
  auto allocator = std::make_shared<StackAllocation<Elem>>(sz);
  SLoc_T extents{Extents...};
  return Block<Elem,Rank,MultiDimRepr>(extents, location.get_strides(), location.get_origin(), 
				       location.get_refinement_factors(),
				       location.get_coarsening_factors(),
				       allocator);
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <int...Extents>
Block<Elem,Rank,false> Block<Elem,Rank,MultiDimRepr>::stack(SLoc_T bstrides, SLoc_T borigin, 
							    SLoc_T brefinement_factors, SLoc_T bcoarsening_factors) {
  // This causes a buildit error if I do static_var (and also make the allocator take a static var)
  loop_type sz = mul_reduce<Extents...>();
  auto allocator = std::make_shared<StackAllocation<Elem>>(sz);
  SLoc_T extents{Extents...};
  return Block<Elem,Rank,MultiDimRepr>(extents, std::move(bstrides), std::move(borigin),
				       std::move(brefinement_factors), std::move(bcoarsening_factors), allocator);
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

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank,MultiDimRepr> Block<Elem,Rank,MultiDimRepr>::user(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
								  SLoc_T brefinement_factors, SLoc_T bcoarsening_factors,
								  dvar<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P> user) {  
  // make sure that the dyn_var passed in matches the actual storage type (seems like it's not caught otherwise?)
  static_assert((MultiDimRepr && peel<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>() == Rank) || 
		(!MultiDimRepr && peel<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>() == 1));
  if constexpr (MultiDimRepr) {
    auto allocator = std::make_shared<UserAllocation<Elem,typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P,Rank>>(user);
    return Block<Elem,Rank,MultiDimRepr>(std::move(bextents), std::move(bstrides), std::move(borigin),
					 std::move(brefinement_factors), std::move(bcoarsening_factors), 
					 allocator);
  } else {
    auto allocator = std::make_shared<UserAllocation<Elem,Elem*,1>>(user);
    return Block<Elem,Rank,MultiDimRepr>(std::move(bextents), std::move(bstrides), std::move(borigin),
					 std::move(brefinement_factors), std::move(bcoarsening_factors), std::move(allocator));
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr>::View(MeshLocation<Rank> block_location,
				   MeshLocation<Rank> view_location, 
				   Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) : 
  block_location(block_location), view_location(view_location), allocator(allocator) { 
  for (int i = 0; i < Rank; i++) {
    this->permuted_indices[i] = i;
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr>::View(MeshLocation<Rank> block_location,
				   MeshLocation<Rank> view_location, 
				   Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator,
				   std::array<int,Rank> permuted_indices) : 
  block_location(block_location), view_location(view_location), allocator(allocator),
  permuted_indices(permuted_indices) { 
}

 template <typename Elem, unsigned long Rank, bool MultiDimRepr>
   View<Elem,Rank,MultiDimRepr>::View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, 
				      SLoc_T brefinement_factors, SLoc_T bcoarsening_factors,
				      SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin, 
				      SLoc_T vrefinement_factors, SLoc_T vcoarsening_factors,		     
				     Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) :
   block_location(LocationBuilder<Rank>().
    with_extents(bextents).
    with_strides(bstrides).
    with_origin(borigin).
    with_refinement(brefinement_factors).
    with_coarsening(bcoarsening_factors).to_loc()),
   view_location(LocationBuilder<Rank>().
    with_extents(vextents).
    with_strides(vstrides).
    with_origin(vorigin).
    with_refinement(vrefinement_factors).
    with_coarsening(vcoarsening_factors).to_loc()),
//  bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
//  brefinement_factors(std::move(brefinement_factors)), bcoarsening_factors(std::move(bcoarsening_factors)),
//  vextents(std::move(vextents)), vstrides(std::move(vstrides)), vorigin(std::move(vorigin)), 
//  vrefinement_factors(std::move(vrefinement_factors)), vcoarsening_factors(std::move(vcoarsening_factors)),
  allocator(std::move(allocator)) { 
  for (int i = 0; i < Rank; i++) {
    this->permuted_indices[i] = i;
  }
/*  this->block_location = LocationBuilder<Rank>().
    with_extents(bextents).
    with_strides(bstrides).
    with_origin(borigin).
    with_refinement(brefinement_factors).
    with_coarsening(bcoarsening_factors).to_loc();
  this->view_location = LocationBuilder<Rank>().
    with_extents(vextents).
    with_strides(vstrides).
    with_origin(vorigin).
    with_refinement(vrefinement_factors).
    with_coarsening(vcoarsening_factors).to_loc();*/
 }      

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long N>
dvar<Elem> View<Elem,Rank,MultiDimRepr>::read(darr<loop_type,N> &coords) {
  if constexpr (N < Rank) {
    for (int i = 0; i < Rank; i++) {
      if (permuted_indices[i] != i) {
	std::cerr << "Cannot use permuted indices with automatic padding!" << std::endl;
	exit(-1);
      }
    }
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
    static_assert(N == Rank);
    // manually specified all the dimensions (can happen with ref reads)
    // first get the global location
    // bi0 = vi0 * vstride0 + vorigin0
    darr<loop_type,Rank> permuted;
    for (int i = 0; i < Rank; i++) {
      permuted[permuted_indices[i]] = coords[i];
    }
    darr<loop_type,Rank> bcoords;
    compute_block_mesh_space_location<0>(permuted, bcoords);
    // now adjust to make it relative to the block
    for (svar<int> i = 0; i < N; i=i+1) {
      bcoords[i] = (bcoords[i] - block_location.get_origin()[i]) / block_location.get_strides()[i];
    }
    // then linearize with respect to the block
#ifndef UNSTAGED
    if constexpr (MultiDimRepr==true) {
      return allocator->read(bcoords);
    } else {
      dvar<loop_type> lidx = linearize<0,Rank>(block_location.get_extents(), bcoords);
      darr<loop_type,1> arr{lidx};
      return allocator->read(arr);
    }
#else
    dvar<loop_type> lidx = linearize<0,Rank>(block_location.get_extents(), bcoords);
    return allocator[lidx];
#endif
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Coords>
dvar<Elem> View<Elem,Rank,MultiDimRepr>::operator()(Coords...coords) {
  // it's possible to pass a dyn_arr in here and have it not complain (but screw everything up).
  // so verify that indices are valid.
  // can be either loop type or dyn_var<loop_type>
  validate_idx_types(coords...);
  darr<loop_type,sizeof...(Coords)> arr{coords...};
  return this->read(arr);
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename ScalarElem, unsigned long N>
void View<Elem,Rank,MultiDimRepr>::write(ScalarElem val, darr<loop_type,N> &coords) {
  if constexpr (N < Rank) {
    // we need padding at the front
    for (int i = 0; i < Rank; i++) {
      if (permuted_indices[i] != i) {
	std::cerr << "Cannot use permuted indices with automatic padding!" << std::endl;
	exit(-1);
      }
    }
    constexpr int pad_amt = Rank-N;
    darr<loop_type,Rank> arr;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (svar<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    this->write(val, std::move(arr));
  } else {
    static_assert(N == Rank);
    // manually specified all the dimensions (can happen with ref writes)
    darr<loop_type,Rank> permuted;
    for (int i = 0; i < Rank; i++) {
      permuted[permuted_indices[i]] = coords[i];
    }
    darr<loop_type,Rank> bcoords;
    compute_block_mesh_space_location<0>(permuted, bcoords);
    // now adjust to make it relative to the block
    for (svar<int> i = 0; i < N; i=i+1) {
      bcoords[i] = (bcoords[i] - block_location.get_origin()[i]) / block_location.get_strides()[i];
    }
    // then linearize with respect to the block
#ifndef UNSTAGED
    if constexpr (MultiDimRepr==true) {
      allocator->write(val, bcoords);
    } else {
      dvar<loop_type> lidx = linearize<0,Rank>(block_location.get_extents(), bcoords);
      darr<loop_type,1> arr{lidx};
      allocator->write(val, arr);
    }
#else
    dvar<loop_type> lidx = linearize<0,Rank>(block_location.get_extents(), bcoords);
    allocator.write(lidx, val);
#endif
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Idx>
auto View<Elem,Rank,MultiDimRepr>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    dvar<loop_type> didx = idx;
    // include any frozen dims here 
    return Ref<View<Elem,Rank,MultiDimRepr>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    // include any frozen dims here
    return Ref<View<Elem,Rank,MultiDimRepr>,std::tuple<decltype(idx)>>(*this, std::tuple{idx});
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename LIdx>
dvar<Elem> View<Elem,Rank,MultiDimRepr>::plidx(LIdx lidx) {
  // must delinearize relative to the view, but need to only do it for the unfrozen part
  darr<loop_type,Rank> coords;
  delinearize<0,Rank>(coords, lidx, view_location.get_extents());
  return this->read(coords);
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::colocate(Block<Elem2,Rank,MultiDimRepr2> &block) {
  // convert parameterization to the base mesh space
  auto base_loc = block.get_location().compute_base_mesh_location();
  // now put into this's mesh space
  auto mesh_loc = base_loc.into(view_location);
  return {block_location, mesh_loc, allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Elem2, bool MultiDimRepr2>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::colocate(View<Elem2,Rank,MultiDimRepr2> &view) {
  // convert parameterization to the base mesh space
  auto base_loc = view.get_view_location().compute_base_mesh_location();
  // now put into this's mesh space
  auto mesh_loc = base_loc.into(view_location);
  return {block_location, mesh_loc, allocator};
} 

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::virtually_refine(Factors...factors) {
  SLoc_T rfactors{factors...};
/*  SLoc_T vextents;
  SLoc_T vorigin;  
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (this->vstrides[i] != 1 && rfactors[i] > 1) {
      print("Cannot refine block with non-unit stride %d in dimension %d.\\n", this->bstrides[i], i);
      hexit(-1);
    }
#endif
    vextents[i] = this->vextents[i] * rfactors[i];
    vorigin[i] = this->vorigin[i] * rfactors[i];
    rfactors[i] = this->vrefinement_factors[i] * rfactors[i];
  }
  return {this->bextents, this->bstrides, this->borigin, this->brefinement_factors, this->bcoarsening_factors,
    std::move(vextents), this->vstrides, std::move(vorigin), std::move(rfactors), this->vcoarsening_factors,
    this->allocator};*/
  return {block_location, view_location.refine(rfactors), allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
Block<Elem,Rank,false> View<Elem,Rank,MultiDimRepr>::physically_refine(Factors...factors) {
  SLoc_T rfactors{factors...};
/*  SLoc_T new_bextents;
  SLoc_T new_borigin;  
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (this->vstrides[i] != 1 && rfactors[i] > 1) {
      print("Cannot refine block with non-unit stride %d in dimension %d.\\n", this->bstrides[i], i);
      hexit(-1);
    }
#endif
    new_bextents[i] = this->vextents[i] * rfactors[i];
    new_borigin[i] = this->vorigin[i] * rfactors[i];
    rfactors[i] = this->vrefinement_factors[i] * rfactors[i];
  }
  return {
    this->bextents, this->bstrides, this->borigin, this->brefinement_factors, this->bcoarsening_factors,
    std::move(new_bextents), this->vstrides, std::move(new_borigin), std::move(rfactors),
    this->vcoarsening_factors,
    this->allocator};*/
  return {view_location.refine(rfactors)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Factors>
Block<Elem,Rank,false> View<Elem,Rank,MultiDimRepr>::physically_coarsen(Factors...factors) {
  SLoc_T cfactors{factors...};
/*  SLoc_T new_bextents;
  SLoc_T new_borigin;  
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (this->vstrides[i] != 1 && cfactors[i] > 1) {
      print("Cannot coarsen block with non-unit stride %d in dimension %d.\\n", this->bstrides[i], i);
      hexit(-1);
    }
#endif
    new_bextents[i] = this->vextents[i] / cfactors[i];
    new_borigin[i] = this->vorigin[i] / cfactors[i];
    cfactors[i] = this->vcoarsening_factors[i] * cfactors[i];
  }
  return {
    this->bextents, this->bstrides, this->borigin, this->brefinement_factors, this->bcoarsening_factors,
    std::move(new_bextents), this->vstrides, std::move(new_borigin), this->refinement_factors,
    std::move(cfactors),
    this->allocator};*/
  return {view_location.coarsen(cfactors)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::view() {
  return {block_location, view_location};
//  return {
//    this->bextents, this->bstrides, this->borigin, this->brefinement_factors, this->bcoarsening_factors,
//    this->vextents, this->vstrides, this->vorigin, this->vrefinement_factors, this->vcoarsening_factors,
//    this->allocator
//  };
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Slices>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::view(Slices...slices) {
  static_assert(sizeof...(Slices) == Rank, "Must specify slices for all dimensions.");
  // the block parameters stay the same, but we need to update the
  // view parameters  
  SLoc_T permuted_extents;
  for (int i = 0; i < Rank; i++) {
    permuted_extents[i] = view_location.get_extents()[permuted_indices[i]];
  }
  SLoc_T vstops_p;
  gather_stops<0,Rank>(vstops_p, permuted_extents, slices...);
  SLoc_T vstrides_p;
  gather_strides<0,Rank>(vstrides_p, slices...);
  SLoc_T vorigin_p;
  gather_origin<0,Rank>(vorigin_p, slices...);
  // convert vstops into extents
  SLoc_T vstops;
  SLoc_T vstrides;
  SLoc_T vorigin;
  for (int i = 0; i < Rank; i++) {
    vstops[permuted_indices[i]] = vstops_p[i];
    vstrides[permuted_indices[i]] = vstrides_p[i];
    vorigin[permuted_indices[i]] = vorigin_p[i];
  }  
  SLoc_T vextents;
  convert_stops_to_extents<Rank>(vextents, vorigin, 
				 vstops, vstrides);
  // now make everything relative to the prior view
  // new origin = old origin + vorigin * old strides
  SLoc_T origin;
  SLoc_T tmp;
  apply<MulFunctor,Rank>(tmp, vorigin, view_location.get_strides());
  apply<AddFunctor,Rank>(origin, view_location.get_origin(), tmp);
  // new strides = old strides * new strides
  SLoc_T strides;
  apply<MulFunctor,Rank>(strides, view_location.get_strides(), vstrides);
  return {block_location, 
    LocationBuilder<Rank>().with_extents(vextents).
    with_strides(strides).
    with_origin(origin).
    with_refinement(view_location.get_refinement_factors()).
    with_coarsening(view_location.get_coarsening_factors()).
    to_loc(),
    allocator};
/*  return View<Elem,Rank,MultiDimRepr>(this->bextents, this->bstrides, this->borigin, this->brefinement_factors,
				      this->bcoarsening_factors,
				      std::move(vextents), std::move(strides), std::move(origin), this->vrefinement_factors,
				      this->vcoarsening_factors,
				      this->allocator);*/
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <int Depth>
  void View<Elem,Rank,MultiDimRepr>::compute_block_mesh_space_location(const darr<loop_type,Rank> &coords,
								       darr<loop_type,Rank> &out) {
  if constexpr (Depth == Rank) {
  } else {
    dvar<loop_type> rvidx = coords[Depth] * 
      view_location.get_strides()[Depth] + 
      view_location.get_origin()[Depth];
    dvar<loop_type> rbidx = rvidx * 
      view_location.get_coarsening_factors()[Depth] * 
      block_location.get_refinement_factors()[Depth] / 
      (block_location.get_coarsening_factors()[Depth] * 
       view_location.get_refinement_factors()[Depth]);
    out[Depth] = rbidx;
    compute_block_mesh_space_location<Depth+1>(coords, out);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename T>
void View<Elem,Rank,MultiDimRepr>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports up to 3 rank dimensions for printing.");
  if constexpr (Rank == 1) {
    for (dvar<loop_type> i = 0; i < view_location.get_extents()[0]; i=i+1) {
      dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i)));
    }
  } else if constexpr (Rank == 2) {
    for (dvar<loop_type> i = 0; i < view_location.get_extents()[0]; i=i+1) {
      for (dvar<loop_type> j = 0; j < view_location.get_extents()[1]; j=j+1) {
	dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j)));
      }
      print_newline();
    }
  } else {
    for (dvar<loop_type> i = 0; i < view_location.get_extents()[0]; i=i+1) {
      for (dvar<loop_type> j = 0; j < view_location.get_extents()[1]; j=j+1) {
	for (dvar<loop_type> k = 0; k < view_location.get_extents()[2]; k=k+1) {
	  dispatch_print_elem<Elem>(cast<Elem>(this->operator()(i,j,k)));
	}
	print_newline();
      }
      print_newline();
      print_newline();

    }
  }
}

/*template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void View<Elem,Rank,MultiDimRepr>::dump_base_mesh_space_location() {
  SLoc_T base_extents;
  SLoc_T base_origin;
  SLoc_T base_strides;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    base_extents[i] = this->vextents[i] / this->vrefinement_factors[i] * this->vcoarsening_factors[i];
    base_origin[i] = this->vorigin[i] / this->vrefinement_factors[i] * this->vcoarsening_factors[i];
    base_strides[i] = this->vstrides[i] / this->vrefinement_factors[i] * this->vcoarsening_factors[i];
  }
  print("View base mesh space location info");
  print_newline();
  print("  Base extents: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(base_extents[r]);    
  }  
  print("  Base origin: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(base_origin[r]);    
  }  
  print("  Base strides: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(base_strides[r]);    
  }  
}*/

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void View<Elem,Rank,MultiDimRepr>::dump_loc() {
  print("View location info");
  print_newline();
  print("  Underlying data structure: ");
  std::string x = ElemToStr<typename decltype(ptr_wrap<Elem,Rank,MultiDimRepr>())::P>::str;
  print(x);
  print_newline();
/*  std::string rank = std::to_string(Rank);
  print("  Rank: ");
  print(rank);
  print_newline();
  print("  BExtents:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bextents[r]);    
  }
  print_newline();
  print("  BStrides:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bstrides[r]);    
  }
  print_newline();
  print("  BOrigin:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(borigin[r]);    

  }
  print_newline();
  print("  BRefinement factors:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(brefinement_factors[r]);    
  }
  print_newline();
  print("  BCoarsening factors:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bcoarsening_factors[r]);    
  }
  print_newline();
  print("  VExtents:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vextents[r]);    

  }
  print_newline();
  print("  VStrides:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vstrides[r]);    
  }
  print_newline();
  print("  VOrigin:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vorigin[r]);    
  }  
  print_newline();
  print("  VRefinement factors:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vrefinement_factors[r]);    
  }  
  print_newline();
  print("  VCoarsening factors:");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vcoarsening_factors[r]);    
  }  
  print_newline();*/
  print("  Block:\\n");
  block_location.dump_location();
  print("  View:\\n");
  view_location.dump_location();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename T>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::permute(std::initializer_list<T> value) {
//  auto viewobj = this->view();
//  attach_permute(viewobj, value, items...);
//  return viewobj;
  return {block_location, view_location, allocator, tuple_to_arr<int>(initlist_to_tuple<Rank>(value))};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::row_major() {
  std::array<int, Rank_T> arr;
  for (int i = 0; i < Rank_T; i++) {
    arr[i] = i;
  }
  return {block_location, view_location, allocator, arr};
//  auto viewobj = this->view();
//  attach_row_major(viewobj);
//  return viewobj;
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank,MultiDimRepr> View<Elem,Rank,MultiDimRepr>::col_major() {
  std::array<int, Rank_T> arr;
  for (int i = 0; i < Rank_T; i++) {
    arr[i] = Rank_T - i - 1;
  }
  return {block_location, view_location, allocator, arr};
//  auto viewobj = this->view();
//  attach_col_major(viewobj);
//  return viewobj;
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Coords>
dvar<bool> View<Elem,Rank,MultiDimRepr>::logically_exists(Coords...coords) {
  static_assert(sizeof...(coords) == Rank, "No default padding allowed for logically_exists");
  Loc_T<Rank> rel;
  compute_block_mesh_space_location<0>(builder::dyn_arr<int,Rank>{coords...}, rel);
  for (svar<loop_type> i = 0; i < Rank; i++) {
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
template <typename Rhs>
void Ref<BlockLike,Idxs>::internal_assign(Rhs rhs) {
  // check that permuted indices are not used here
  if constexpr (!BlockLike::IsBlock_T) {
    for (int i = 0; i < BlockLike::Rank_T; i++) {
      if (block_like.permuted_indices[i] != i) {
	std::cerr << "Permuted indices cannot be used on the LHS of an inline index" << std::endl;
	exit(-1);
      }
    }
  }
  if constexpr (/*is_view<BlockLike>::value && */std::tuple_size<Idxs>() < BlockLike::Rank_T) {
    // get any padding dimensions
    constexpr int pad_amt = BlockLike::Rank_T - std::tuple_size<Idxs>();
    darr<loop_type,pad_amt> padded;
    for (svar<int> i = 0; i < pad_amt; i=i+1) {
      padded[i] = 0;
    }
    // now combine everything
    auto new_idxs = std::tuple_cat(dyn_arr_to_tuple<pad_amt,0>(std::move(padded)), this->idxs);
    auto new_ref = Ref<BlockLike, decltype(new_idxs)>(this->block_like, std::move(new_idxs));
    new_ref.realize_loop_nest(rhs);
  } else {
    // no padding or unfreezing needed (you may have frozen dimensions, but they would be overridden in this case)
    realize_loop_nest(rhs);
  }
}

template <typename BlockLike, typename Idxs>
Ref<BlockLike, Idxs> &Ref<BlockLike,Idxs>::operator=(Ref<BlockLike, Idxs> &rhs) {
  // force it to the not copy-assignment operator=
  *this = BlockLike::Elem_T(1) * rhs;    
  return *this;
}


// raw value
template <typename BlockLike, typename Idxs>
void Ref<BlockLike,Idxs>::operator=(typename BlockLike::Elem_T x) {
  this->verify_unadorned();
  this->verify_unique<0>();
  // TODO can potentially memset this whole thing
  constexpr unsigned long s = std::tuple_size<Idxs>();
//  realize_loop_nest(x);
  internal_assign(x);
}

template <typename BlockLike, typename Idxs>
template <typename Rhs>
void Ref<BlockLike,Idxs>::operator=(Rhs rhs) {
  this->verify_unadorned();
  this->verify_unique<0>();
  constexpr unsigned long s = std::tuple_size<Idxs>();
//  realize_loop_nest(rhs);
  internal_assign(rhs);
}

template <typename BlockLike, typename Idxs>
//template <typename Rhs, typename std::enable_if<is_dyn_like<Rhs>::value, int>::type>
void Ref<BlockLike,Idxs>::operator=(builder::builder rhs) {
  builder::dyn_var<typename BlockLike::Elem_T> rhs2 = rhs;
  this->verify_unadorned();
  this->verify_unique<0>();
//  static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
//  realize_loop_nest(rhs2);
  internal_assign(rhs2);
}

template <typename BlockLike, typename Idxs>
template <int Iter, int IdxDepth, typename LhsIdxs, unsigned long N>
void Ref<BlockLike,Idxs>::realize_each(LhsIdxs lhs, 
				       const darr<loop_type,N> &iters,
				       darr<loop_type,BlockLike::Rank_T> &out) {
  constexpr unsigned long s = std::tuple_size<Idxs>();
  auto i = std::get<IdxDepth>(idxs); // these are the rhs indexes!
  if constexpr (is_dyn_like<decltype(i)>::value ||
		std::is_same<decltype(i),loop_type>()) {
    // this is just a plain value--use it directly
    if constexpr (Iter < BlockLike::Rank_T - 1) {
      out[Iter] = i;
      realize_each<Iter+1,IdxDepth+1>(lhs, iters, out);
    } else {
      out[Iter] = i;
    }
  } else {
    auto r = i.realize(lhs, iters);
    if constexpr (Iter < BlockLike::Rank_T - 1) {
      out[Iter] = r;
      realize_each<Iter+1,IdxDepth+1>(lhs, iters, out);
    } else {
      out[Iter] = r;
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
  darr<loop_type,BlockLike::Rank_T> arr; 
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
  constexpr int rank = BlockLike::Rank_T;
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
	for (dvar<loop_type> iter = 0; iter < block_like.location.get_extents()[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      } else {
	for (dvar<loop_type> iter = 0; iter < block_like.view_location.get_extents()[depth]; iter = iter + 1) {
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
