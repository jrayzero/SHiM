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

namespace cola {

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

// 4. Interpolation factors logically increase the extent of the hmda.
// For example, if this block is 10x10 and the interpolation factors are 3x2,
// we can access the resulting view as if it were a 30x20 region.
// Note that interpolation factors don't impact the physical size, so if you
// took that 10x10 interpolated view and did inline indexing with it, it would
// iterate over the 10x10 region, not 30x20.
// When you read/write/slice from the interpolated hmda, it divides the accessors by the 
// interpolation factors. 
// When you call colocate on an interpolated hmda, it still uses the parameters from the 
// thing you are interpolating with, but it DIVIDES THE OTHER OBJECTS EXTENT AND LOCATION by the interpolation
// factors. Otherwise your physical size would be very wrong. If the other object has an interpolation,
// this is undefined.

///
/// A region of data with a location
/// If MultiDimPtr = true, the underlying array is a pointer to pointer to etc... (like Rank 2 = **).
/// This is only supported for user-side allocations
template <typename Elem, unsigned long Rank, bool MultiDimPtr=false>
struct Block {
  
  template <typename Elem2, unsigned long Rank2, bool MultiDimPtr2>
  friend struct View;

  using SLoc_T = Loc_T<Rank>;
  using Elem_T = Elem;
  static constexpr unsigned long Rank_T = Rank;
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
  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,N> &iters);

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  builder::dyn_var<Elem> operator()(Coords...coords);

  ///
  /// Read a single element at the specified linear index
  template <typename LIdx>
  builder::dyn_var<Elem> plidx(LIdx lidx);

  ///
  /// Create a view on this' data using the location of block.
  template <typename Elem2, bool MultiDimPtr2>
  View<Elem,Rank,MultiDimPtr> colocate(Block<Elem2,Rank,MultiDimPtr2> &block);

  ///
  /// Create a view on this' data using the location of view (not its underlying block!)
  template <typename Elem2, bool MultiDimPtr2>
  View<Elem,Rank,MultiDimPtr> colocate(View<Elem2,Rank,MultiDimPtr2> &view);

  /// 
  /// Create interpolation factors that logically increase the extent of this block.
  template <typename...Factors>
  View<Elem,Rank,MultiDimPtr> logically_interpolate(Factors...factors);

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, unsigned long N>
  void write(ScalarElem val, builder::dyn_arr<loop_type,N> &iters);

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

  ///
  /// print out the location info
  void dump_loc();

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
template <typename Elem, unsigned long Rank, bool MultiDimPtr=false>
struct View {

  using SLoc_T = Loc_T<Rank>;
  using Elem_T = Elem;
  static constexpr unsigned long Rank_T = Rank;
  static constexpr bool IsBlock_T = false;

  /// 
  /// Create a View from the specified location information
  View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
       SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin,
       SLoc_T interpolation_factors,
       std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator) :
    bextents(bextents), bstrides(bstrides), borigin(borigin),
    vextents(vextents), vstrides(vstrides), vorigin(vorigin), 
    interpolation_factors(interpolation_factors),
    allocator(allocator) { }

  ///
  /// Read a single element at the specified coordinate
  template <unsigned long N>
  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,N> &coords);

  template <unsigned long N>
  void compute_coords(builder::dyn_arr<loop_type,N> &coords, builder::dyn_arr<loop_type,N> &out);

  ///
  /// Read a single element at the specified coordinate
  template <typename...Coords>
  builder::dyn_var<Elem> operator()(Coords...coords);

  ///
  /// Read a single element at the specified linear index
  template <typename LIdx>
  builder::dyn_var<Elem> plidx(LIdx lidx);

  ///
  /// Create a view on this' data using the location of block.
  template <typename Elem2, bool MultiDimPtr2>
  View<Elem,Rank,MultiDimPtr> colocate(Block<Elem2,Rank,MultiDimPtr2> &block);

  ///
  /// Create a view on this' data using the location of view (not its underlying block!)
  template <typename Elem2, bool MultiDimPtr2>
  View<Elem,Rank,MultiDimPtr> colocate(View<Elem2,Rank,MultiDimPtr2> &view);

  /// 
  /// Create interpolation factors that logically increase the extent of this block.
  template <typename...Factors>
  View<Elem,Rank,MultiDimPtr> logically_interpolate(Factors...factors);

  ///
  /// Reset interpolation factors back to 1
  View<Elem,Rank,MultiDimPtr> reset();

  ///
  /// Write a single element at the specified coordinate
  /// Prefer the operator[] write method over this.
  template <typename ScalarElem, unsigned long N>
  void write(ScalarElem val, builder::dyn_arr<loop_type,N> &coords);

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

  ///
  /// print out the location info
  void dump_loc();

  ///
  /// True if global coords computed from the input coords are >= 0.
  /// Does not guarantee that the data physically exists in the underlying block.  
  template <typename...Coords>
  builder::dyn_var<bool> logically_exists(Coords...coords);
  
  
  std::shared_ptr<Allocation<Elem,physical<Rank,MultiDimPtr>()>> allocator;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;
  SLoc_T vextents;
  SLoc_T vstrides;
  SLoc_T vorigin;  
  SLoc_T interpolation_factors;

private:
  
  ///
  /// Compute the absolute indices of the coordinates
  template <int Depth, unsigned long N>
  void compute_absolute_location(const builder::dyn_arr<loop_type,N> &coords,
				    builder::dyn_arr<loop_type,N> &out);


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
  friend builder::dyn_var<typename GetCoreT<T>::Core_T> dispatch_realize(T to_realize, const LhsIdxs &lhs_idxs, const builder::dyn_arr<loop_type,N> &iters);

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
  /// Realize the value represented by this Ref
  template <typename LhsIdxs, unsigned long N>
  builder::dyn_var<typename BlockLike::Elem_T> realize(LhsIdxs lhs, 
						       const builder::dyn_arr<loop_type,N> &iters);

  ///
  /// Helper method for realizing values for the Idxs of this ref
  template <int Depth, typename LhsIdxs, unsigned long N>
  void realize_each(LhsIdxs lhs, const builder::dyn_arr<loop_type,N> &iters,
		    builder::dyn_arr<loop_type,BlockLike::Rank_T> &out);
  
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <unsigned long N>
builder::dyn_var<Elem> Block<Elem,Rank,MultiDimPtr>::read(builder::dyn_arr<loop_type,N> &coords) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    builder::dyn_arr<loop_type,Rank> arr;
    for (builder::static_var<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (builder::static_var<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    return this->read(arr);
  } else {
    if constexpr (MultiDimPtr==true) {
      return allocator->read(coords);
    } else {
      builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, coords);
      builder::dyn_arr<loop_type,1> arr{lidx};
      return allocator->read(arr);
    }
  }
}

template <int Depth, unsigned long N, typename Coord, typename...Coords>
void splat(builder::dyn_arr<loop_type,N> &arr, Coord coord, Coords...coords) {
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename...Coords>
builder::dyn_var<Elem> Block<Elem,Rank,MultiDimPtr>::operator()(Coords...coords) {
  // it's possible to pass a dyn_arr in here and have it not complain (but screw everything up).
  // so verify that indices are valid.
  // can be either loop type or dyn_var<loop_type>
  validate_idx_types(coords...);
  builder::dyn_arr<loop_type,sizeof...(Coords)> arr{coords...};
  return this->read(arr);
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename ScalarElem, unsigned long N>
void Block<Elem,Rank,MultiDimPtr>::write(ScalarElem val, builder::dyn_arr<loop_type,N> &coords) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    builder::dyn_arr<loop_type,Rank> arr;
    for (builder::static_var<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (builder::static_var<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    this->write(val, arr);
  } else {
    if constexpr (MultiDimPtr==true) {
      allocator->write(val, coords);
    } else {
      builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, coords);
      builder::dyn_arr<loop_type,1> arr{lidx};
      allocator->write(val, arr);
    }
  }
}


template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename LIdx>
builder::dyn_var<Elem> Block<Elem,Rank,MultiDimPtr>::plidx(LIdx lidx) {
  if constexpr (MultiDimPtr == true) {
    SLoc_T idxs;
    delinearize<0,Rank>(idxs, lidx, this->bextents);
    return allocator->read(idxs);
  } else {
    builder::dyn_arr<loop_type,physical<Rank,MultiDimPtr>()> idxs{lidx};
    return allocator->read(idxs);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename Elem2, bool MultiDimPtr2>
View<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::colocate(Block<Elem2,Rank,MultiDimPtr2> &block) {
  SLoc_T interpolation_factors;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return {this->bextents, this->bstrides, this->borigin, 
    block.bextents, block.bstrides, block.borigin,  
    interpolation_factors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename Elem2, bool MultiDimPtr2>
View<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::colocate(View<Elem2,Rank,MultiDimPtr2> &view) {
//  SLoc_T interpolation_factors;
//  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
//    interpolation_factors[i] = 1;
//  }
  return {this->bextents, this->bstrides, this->borigin, 
    view.vextents, view.vstrides, view.vorigin, 
    view.interpolation_factors,
    this->allocator};
} 

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename...Factors>
View<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::logically_interpolate(Factors...factors) {
  SLoc_T ifactors{factors...};
  SLoc_T vextents;
  SLoc_T vorigin;
  for (builder::static_var<loop_type> i = 0; i < Rank; i=i+1) {
    vextents[i] = this->bextents[i] * ifactors[i];
    vorigin[i] = this->borigin[i] * ifactors[i];
  }
  return {this->bextents, this->bstrides, this->borigin,
    vextents, this->bstrides, vorigin,
    ifactors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
View<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::view() {
  SLoc_T interpolation_factors;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return {this->bextents, this->bstrides, this->borigin,
    this->bextents, this->bstrides, this->borigin,
    interpolation_factors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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
  SLoc_T vextents;
  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  // new strides = old strides * new strides
  SLoc_T strides;
  apply<MulFunctor,Rank>(strides, this->bstrides, vstrides);
  SLoc_T interpolation_factors;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    interpolation_factors[i] = 1;
  }
  return View<Elem,Rank,MultiDimPtr>(this->bextents, this->bstrides, this->borigin,
				     vextents, strides, vorigin,
				     interpolation_factors,
				     this->allocator);  
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
void Block<Elem,Rank,MultiDimPtr>::dump_loc() {
  print("Block location info");
  print_newline();
  print("  Underlying data structure: ");
  print(ElemToStr<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P>::str);
  print_newline();
  print("  BExtents:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bextents[r]);    
  }
  print_newline();
  print("  BStrides:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bstrides[r]);    
  }
  print_newline();
  print("  BOrigin:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(borigin[r]);    
  }
  print_newline();
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr>::Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin, bool memset_data) :
  bextents(deepcopy(bextents)), bstrides(deepcopy(bstrides)), borigin(deepcopy(borigin)) {
  static_assert(!MultiDimPtr);
  this->allocator = std::make_shared<HeapAllocation<Elem>>(reduce<MulFunctor>(bextents));
  if (memset_data) {
    int elem_size = sizeof(Elem);
    builder::dyn_var<loop_type> sz(reduce<MulFunctor>(bextents) * elem_size);
    allocator->memset(sz);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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
    builder::dyn_var<loop_type> sz(reduce<MulFunctor>(bextents) * elem_size);
    allocator->memset(sz);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
Block<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::heap(SLoc_T bextents) {
  return Block<Elem,Rank,MultiDimPtr>(bextents, false);
}    

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <loop_type...Extents>
Block<Elem,Rank,MultiDimPtr> Block<Elem,Rank,MultiDimPtr>::stack() {
  static_assert(Rank == sizeof...(Extents));
  auto allocator = std::make_shared<StackAllocation<Elem,mul_reduce<Extents...>()>>();
  SLoc_T bextents{Extents...};// = to_Loc_T<Extents...>();
  return Block<Elem,Rank,MultiDimPtr>(bextents, allocator);
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <unsigned long N>
builder::dyn_var<Elem> View<Elem,Rank,MultiDimPtr>::read(builder::dyn_arr<loop_type,N> &coords) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    builder::dyn_arr<loop_type,Rank> arr;
    for (builder::static_var<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (builder::static_var<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    return this->read(arr);
  } else {
    // first get the global location
    // bi0 = vi0 * vstride0 + vorigin0
    builder::dyn_arr<loop_type,N> bcoords;
    compute_absolute_location<0>(coords, bcoords);
    // then linearize with respect to the block
    if constexpr (MultiDimPtr==true) {
      return allocator->read(bcoords);
    } else {
      builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, bcoords);
      builder::dyn_arr<loop_type,1> arr{lidx};
      return allocator->read(arr);
    }
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <unsigned long N>
void View<Elem,Rank,MultiDimPtr>::compute_coords(builder::dyn_arr<loop_type,N> &coords,
						 builder::dyn_arr<loop_type,N> &out) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    builder::dyn_arr<loop_type,Rank> arr;
    for (builder::static_var<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (builder::static_var<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    this->compute_coords(arr);
  } else {
    compute_absolute_location<0>(coords, out);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename...Coords>
builder::dyn_var<Elem> View<Elem,Rank,MultiDimPtr>::operator()(Coords...coords) {
  // it's possible to pass a dyn_arr in here and have it not complain (but screw everything up).
  // so verify that indices are valid.
  // can be either loop type or dyn_var<loop_type>
  validate_idx_types(coords...);
  builder::dyn_arr<loop_type,sizeof...(Coords)> arr{coords...};
  return this->read(arr);
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename ScalarElem, unsigned long N>
void View<Elem,Rank,MultiDimPtr>::write(ScalarElem val, builder::dyn_arr<loop_type,N> &coords) {
  static_assert(N <= Rank);
  if constexpr (N < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-N;
    builder::dyn_arr<loop_type,Rank> arr;
    for (builder::static_var<int> i = 0; i < pad_amt; i=i+1) {
      arr[i] = 0;
    }
    for (builder::static_var<int> i = 0; i < N; i=i+1) {
      arr[i+pad_amt] = coords[i];
    }
    this->write(val, arr);
  } else {
    // the iters are relative to the View, so first make them relative to the block
    // bi0 = vi0 * vstride0 + vorigin0
    builder::dyn_arr<loop_type,N> bcoords;
    compute_absolute_location<0>(coords, bcoords);
    // then linearize with respect to the block
    if constexpr (MultiDimPtr==true) {
      allocator->write(val, bcoords);
    } else {
      builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, bcoords);
      builder::dyn_arr<loop_type,1> arr{lidx};
      allocator->write(val, arr);
    }
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename LIdx>
builder::dyn_var<Elem> View<Elem,Rank,MultiDimPtr>::plidx(LIdx lidx) {
  // must delinearize relative to the view
  SLoc_T coord;
  delinearize<0,Rank>(coord, lidx, vextents);
  return this->read(coord);
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename Elem2, bool MultiDimPtr2>
View<Elem,Rank,MultiDimPtr> View<Elem,Rank,MultiDimPtr>::colocate(Block<Elem2,Rank,MultiDimPtr2> &block) {
  // adjust by the factors
  SLoc_T extents;
  SLoc_T origin;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    extents[i] = block.bextents[i] * this->interpolation_factors[i];
    origin[i] = block.borigin[i] * this->interpolation_factors[i];
  }
  return {this->bextents, this->bstrides, this->borigin, 
    extents, block.bstrides, origin, 
    this->interpolation_factors, 
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename Elem2, bool MultiDimPtr2>
View<Elem,Rank,MultiDimPtr> View<Elem,Rank,MultiDimPtr>::colocate(View<Elem2,Rank,MultiDimPtr2> &view) {
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename...Factors>
View<Elem,Rank,MultiDimPtr> View<Elem,Rank,MultiDimPtr>::logically_interpolate(Factors...factors) {
  SLoc_T ifactors{factors...};
  SLoc_T new_vextents;
  SLoc_T new_vorigin;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    new_vextents[i] = this->vextents[i] * ifactors[i];
    new_vorigin[i] = this->vorigin[i] * ifactors[i];
    ifactors[i] = ifactors[i] * this->interpolation_factors[i];
  }  
  return {this->bextents, this->bstrides, this->borigin,
    new_vextents, this->vstrides, new_vorigin,
    ifactors,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
View<Elem,Rank,MultiDimPtr> View<Elem,Rank,MultiDimPtr>::reset() {
  SLoc_T ones;
  SLoc_T adjusted_vextents;
  SLoc_T adjusted_vorigin;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    ones[i] = 1;
    adjusted_vextents[i] = vextents[i] / this->interpolation_factors[i];
    adjusted_vorigin[i] = vorigin[i] / this->interpolation_factors[i];
  }
  return {this->bextents, this->bstrides, this->borigin,
    adjusted_vextents, this->vstrides, adjusted_vorigin,
    ones,
    this->allocator};
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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
  SLoc_T vextents;
  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  // now make everything relative to the prior view
  // new origin = old origin + vorigin * old strides
  SLoc_T origin;
  SLoc_T tmp;
  apply<MulFunctor,Rank>(tmp, vorigin, this->vstrides);
  apply<AddFunctor,Rank>(origin, this->vorigin, tmp);
  // new strides = old strides * new strides
  SLoc_T strides;
  apply<MulFunctor,Rank>(strides, this->vstrides, vstrides);
  return View<Elem,Rank,MultiDimPtr>(this->bextents, this->bstrides, this->borigin,
				     vextents, strides, origin,
				     this->interpolation_factors,
				     this->allocator);
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <int Depth, unsigned long N>
void View<Elem,Rank,MultiDimPtr>::compute_absolute_location(const builder::dyn_arr<loop_type,N> &coords,
							       builder::dyn_arr<loop_type,N> &out) {
  if constexpr (Depth == N) {
  } else {
    out[Depth] = (coords[Depth] * vstrides[Depth] + vorigin[Depth]) / this->interpolation_factors[Depth];
    compute_absolute_location<Depth+1>(coords, out);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
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

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
void View<Elem,Rank,MultiDimPtr>::dump_loc() {
  print("View location info");
  print_newline();
  print("  Underlying data structure: ");
  std::string x = ElemToStr<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr>())::P>::str;
  print(x);
  print_newline();
  print("  BExtents:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bextents[r]);    
  }
  print_newline();
  print("  BStrides:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(bstrides[r]);    
  }
  print_newline();
  print("  BOrigin:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(borigin[r]);    
  }
  print_newline();
  print("  Interpolated VExtents:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vextents[r]);    
  }
  print_newline();
  print("  UnInterpolated VExtents:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vextents[r] / interpolation_factors[r]);    
  }
  print_newline();
  print("  VStrides:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vstrides[r]);    
  }
  print_newline();
  print("  VOrigin:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vorigin[r]);    
  }  
  print_newline();
  print("  UnInterpolated VOrigin:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(vorigin[r] / interpolation_factors[r]);    
  }  
  print_newline();
  print("  Interpolation factors:");
  for (builder::static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(interpolation_factors[r]);    
  }  
  print_newline();
}

template <typename Elem, unsigned long Rank, bool MultiDimPtr>
template <typename...Coords>
builder::dyn_var<bool> View<Elem,Rank,MultiDimPtr>::logically_exists(Coords...coords) {
  static_assert(sizeof...(coords) == Rank);
  builder::dyn_arr<int,Rank> rel;
  compute_absolute_location<0>(builder::dyn_arr<int,Rank>{coords...}, rel);
  for (builder::static_var<loop_type> i = 0; i < Rank; i++) {
    if (rel[i] < 0) {
      return false; 
    }
  }
  return true;
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
  
// raw value
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
//template <typename Rhs, typename std::enable_if<is_dyn_like<Rhs>::value, int>::type>
void Ref<BlockLike,Idxs>::operator=(builder::builder rhs) {
  // in the even Rhs is a builder::builder, force that do dyn_var! things go very wrong otherwise
  builder::dyn_var<typename BlockLike::Elem_T> rhs2 = rhs;
  this->verify_unadorned();
  this->verify_unique<0>();
  static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
  realize_loop_nest(rhs2);
}

template <typename BlockLike, typename Idxs>
template <int IdxDepth, typename LhsIdxs, unsigned long N>
void Ref<BlockLike,Idxs>::realize_each(LhsIdxs lhs, 
				       const builder::dyn_arr<loop_type,N> &iters,
				       builder::dyn_arr<loop_type,BlockLike::Rank_T> &out) {
  auto i = std::get<IdxDepth>(idxs); // these are the rhs indexes!
  if constexpr (is_dyn_like<decltype(i)>::value ||
		std::is_same<decltype(i),loop_type>()) {
    // this is just a plain value--use it directly
    if constexpr (IdxDepth < BlockLike::Rank_T - 1) {
      out[IdxDepth] = i;
      realize_each<IdxDepth+1>(lhs, iters, out);
    } else {
      out[IdxDepth] = i;
    }
  } else {
    auto r = i.realize(lhs, iters);
    if constexpr (IdxDepth < BlockLike::Rank_T - 1) {
      out[IdxDepth] = r;
      realize_each<IdxDepth+1>(lhs, iters, out);
    } else {
      out[IdxDepth] = r;
    }
  }
}
  
template <typename BlockLike, typename Idxs>
template <typename LhsIdxs, unsigned long N>
builder::dyn_var<typename BlockLike::Elem_T> Ref<BlockLike,Idxs>::realize(LhsIdxs lhs, 
									  const builder::dyn_arr<loop_type,N> &iters) {
  // first realize each idx
  // use Rank_T instead of N b/c the rhs refs may be diff
  // dimensions than lhs
  builder::dyn_arr<loop_type,BlockLike::Rank_T> arr; 
  realize_each<0>(lhs, iters, arr);
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
    builder::dyn_arr<loop_type,sizeof...(Iters)> arr{iters...};
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
