// -*-c++-*-

#pragma once

#include <vector>
#include <sstream>
#include "builder/dyn_var.h"
#include "builder/array.h"
#include "fwddecls.hpp"
#include "traits.hpp"
#include "functors.hpp"
#include "expr.hpp"

namespace shim {

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
  friend dyn_var<typename GetCoreT<T>::Core_T> dispatch_realize(T to_realize, const LhsIdxs &lhs_idxs, const dyn_arr<loop_type,N> &iters);

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
  dyn_var<typename BlockLike::Elem_T> realize(LhsIdxs lhs, 
					   const dyn_arr<loop_type,N> &iters);

  ///
  /// Helper method for realizing values for the Idxs of this ref
  template <int Iter, int IdxDepth, typename LhsIdxs, unsigned long N>
  void realize_each(LhsIdxs lhs, const dyn_arr<loop_type,N> &iters,
		    dyn_arr<loop_type,BlockLike::Rank_T> &out);
  
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
    dyn_var<loop_type> didx = idx;
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
    dyn_arr<loop_type,pad_amt> padded;
    for (static_var<int> i = 0; i < pad_amt; i=i+1) {
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
				       const dyn_arr<loop_type,N> &iters,
				       dyn_arr<loop_type,BlockLike::Rank_T> &out) {
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
dyn_var<typename BlockLike::Elem_T> Ref<BlockLike,Idxs>::realize(LhsIdxs lhs, 
							      const dyn_arr<loop_type,N> &iters) {
  // first realize each idx
  // use NUnfrozen_T instead of N b/c the rhs refs may be diff
  // dimensions than lhs
  dyn_arr<loop_type,BlockLike::Rank_T> arr; 
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
	dyn_var<loop_type> iter = std::get<depth>(idxs);
	realize_loop_nest(rhs, iters..., iter);
    } else {
      // loop
      if constexpr (BlockLike::IsBlock_T) {
	for (dyn_var<loop_type> iter = 0; iter < block_like.location().extents()[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      } else {
	for (dyn_var<loop_type> iter = 0; iter < block_like.vlocation().extents()[depth]; iter = iter + 1) {
	  realize_loop_nest(rhs, iters..., iter);
	}
      }
    }
  } else {
    // at the innermost level
    dyn_arr<loop_type,sizeof...(Iters)> arr{iters...};
    if constexpr (std::is_arithmetic<Rhs>() ||
		  is_dyn_like<Rhs>::value) {
      block_like.write(arr, rhs);
    } else {      
      block_like.write(arr, rhs.realize(idxs, arr));
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
