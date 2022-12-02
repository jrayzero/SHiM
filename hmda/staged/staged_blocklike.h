#pragma once

#include <vector>
#include <sstream>
#include "builder/dyn_var.h"
#include "functors.h"
#include "staged_utils.h"
#include "expr.h"

namespace hmda {

// Notes:
// don't use operator() with Iter objects. use operator[] instead.
// so block[i][j] = other[i][j] => good
// so block[i][j] = other(i,j) => bad

// The location information within Blocks and Views is immutable once set, so we
// don't have to worry about breaking dependencies with dyn_vars when passing
// them across blocks and views.

template <typename BlockLike, typename Idxs> 
struct Ref;

template <typename Elem, int Rank>
struct View;

// If we don't deepcopy, then the user could modify dyn_vars that they use for extents and such, which
// would affect the location of a given block/view. This is bad. It'd basically be like having a pointer
// represent the location information.
template <int Rank>
Loc_T<Rank> deepcopy(Loc_T<Rank> obj) {
  Loc_T<Rank> copy;
  for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
    copy[i] = obj[i];
  }
  return copy;
}

template <typename Elem, int Rank>
struct Block {

  using SLoc_T = Loc_T<Rank>;//builder::dyn_var<loop_type[Rank]>;//Wrapped<Rank,loop_type>;
  using Elem_T = Elem;
  static constexpr int Rank_T = Rank;
  static constexpr bool IsBlock_T = true;  

  Block(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin) :
    bextents(deepcopy(bextents)), bstrides(deepcopy(bstrides)), borigin(deepcopy(borigin)),
    data(malloc_func(reduce<MulFunctor>(bextents) * (int)sizeof(Elem))) { 
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(reduce<MulFunctor>(bextents) * elem_size);
    memset_func(data, 0, sz);
  }

  Block(SLoc_T bextents) :
    bextents(std::move(bextents)),
    data(malloc_func(reduce<MulFunctor>(bextents) * (int)sizeof(Elem))) { 
    for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
      bstrides[i] = 1;
      borigin[i] = 0;
    }
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(reduce<MulFunctor>(bextents) * elem_size);
    memset_func(data, 0, sz);
  }

  // Can use for either staging OR passing in an array of pre-set data
  Block(SLoc_T bextents, builder::dyn_var<Elem*> data) :
    bextents(std::move(bextents)), data(data) { 
    for (builder::static_var<int> i = 0; i < Rank; i=i+1) {
      bstrides[i] = 1;
      borigin[i] = 0;
    }
  }

  // Access a single element
  template <typename...Iters>
  builder::dyn_var<loop_type> operator()(std::tuple<Iters...>);

  // Access a single element
  template <typename...Iters>
  builder::dyn_var<loop_type> operator()(Iters...iters);

  // Access a single element with a pseudo-linear index
  template <typename LIdx>
  builder::dyn_var<loop_type> plidx(LIdx lidx);

  // ScalarElem is either Elem of dyn_var<Elem>
  template <typename ScalarElem, typename...Iters>
  void write(ScalarElem val, std::tuple<Iters...> iters);

  // Return a simple view over the whole block
  auto view();

  template <typename...Slices>
  View<Elem,Rank> view(Slices...slices);

  template <typename Idx>
  auto operator[](Idx idx);

  builder::dyn_var<Elem*> data;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;

};

template <typename Elem, int Rank>
struct View {

  using SLoc_T = builder::dyn_var<loop_type[Rank]>;
  using Elem_T = Elem;
  static constexpr int Rank_T = Rank;
  static constexpr bool IsBlock_T = false;

  // make private and have block be a friend
  View(SLoc_T bextents, SLoc_T bstrides, SLoc_T borigin,
       SLoc_T vextents, SLoc_T vstrides, SLoc_T vorigin,
       builder::dyn_var<Elem*> data) :
    bextents(deepcopy(bextents)), bstrides(deepcopy(bstrides)), 
    borigin(deepcopy(borigin)), vextents(deepcopy(vextents)), 
    vstrides(deepcopy(vstrides)), vorigin(deepcopy(vorigin)),
    data(data) { }

  // Access a single element
  template <typename...Iters>
  builder::dyn_var<loop_type> operator()(std::tuple<Iters...>);

  // Access a single element
  template <typename...Iters>
  builder::dyn_var<loop_type> operator()(Iters...iters);

  // Access a single element with a pseudo-linear index
  template <typename LIdx>
  builder::dyn_var<loop_type> plidx(LIdx lidx);

  template <typename ScalarElem, typename...Iters>
  void write(ScalarElem val, std::tuple<Iters...> iters);

  template <typename...Slices>
  View<Elem,Rank> view(Slices...slices);

  template <typename Idx>
  auto operator[](Idx idx);

  builder::dyn_var<Elem*> data;
  SLoc_T bextents;
  SLoc_T bstrides;
  SLoc_T borigin;
  SLoc_T vextents;
  SLoc_T vstrides;
  SLoc_T vorigin;

};

template <typename T>
struct IsDynVar { constexpr bool operator()() { return false; } };

template <typename T>
struct IsDynVar<builder::dyn_var<T>> { constexpr bool operator()() { return true; } };

template <typename T>
struct IsBuilder { constexpr bool operator()() { return false; } };

template <>
struct IsBuilder<builder::builder> { constexpr bool operator()() { return true; } };

template <typename T>
constexpr bool is_dyn_var() {
  return IsDynVar<T>()();
}

template <typename T>
constexpr bool is_dyn_like() {
  return IsDynVar<T>()() || IsBuilder<T>()();
}

template <typename T>
constexpr bool is_builder() {
  return IsBuilder<T>()();
}

template <typename BlockLike, typename Idxs>
struct Ref : public Expr<Ref<BlockLike,Idxs>> {

  Idxs idxs;
  BlockLike block_like;

  Ref(BlockLike block_like, Idxs idxs) : block_like(block_like), idxs(idxs) { }
  
  template <typename Idx>
  auto operator[](Idx idx) {
    auto args = std::tuple_cat(idxs, std::tuple{idx});
    return Ref<BlockLike,decltype(args)>(block_like, args);
  }
  
  void operator=(typename BlockLike::Elem_T x) {
    this->verify_unadorned();
    this->verify_unique<0>();
    // TODO can potentially memset this whole thing
    static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
    realize_loop_nest(x);
  }

  template <typename Rhs>
  void operator=(Rhs rhs) {
    this->verify_unadorned();
    this->verify_unique<0>();
    static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
    realize_loop_nest(rhs);
  }

  template <int Depth, typename LhsIdxs, typename...Iters>
  auto realize_each(LhsIdxs lhs, std::tuple<Iters...> iters) {
    auto i = std::get<Depth>(idxs);
    if constexpr (is_dyn_like<decltype(i)>() ||
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
  
  template <typename LhsIdxs, typename...Iters>
  auto realize(LhsIdxs lhs, std::tuple<Iters...> iters) {
    // first realize each idx
    auto realized = realize_each<0>(lhs, iters);
    // then access the thing
    return block_like(realized);
  }

private:

  // check that lhs idxs are unadorned
  template <int Depth=0>
  void verify_unadorned() {
    // unadorned must be either an integral, Iter, or dyn_var<loop_type>
    auto i = std::get<Depth>(idxs);
    using T = decltype(i);
    static_assert(std::is_integral<T>() || 
		  is_dyn_like<T>() ||
		  is_iter<T>(), "LHS indices for inline write must be unadorned.");
    if constexpr (Depth < BlockLike::Rank_T - 1) {
      verify_unadorned<Depth+1>();
    }
  }  

  // check that lhs idxs are unique
  template <int Depth, char...Seen>
  void verify_unique() {
    // unadorned must be either an integral, Iter, or dyn_var<loop_type>
    auto i = std::get<Depth>(idxs);
    using T = decltype(i);
    if constexpr (is_iter<T>()) {
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
  
  template <typename Rhs, typename...Iters>
  void realize_loop_nest(Rhs rhs, Iters...iters) {
    // the lhs indices can be either an Iter or an integer.
    // Iter -> loop over whole extent
    // integer -> single iteration loop
    constexpr int rank = BlockLike::Rank_T;
    constexpr int depth = sizeof...(Iters);
    if constexpr (depth < rank) {
      auto dummy = std::get<depth>(idxs);
      if constexpr (std::is_integral<decltype(dummy)>() ||
		    is_dyn_like<decltype(dummy)>()) {
	// single iter
	builder::dyn_var<loop_type> iter = std::get<depth>(idxs);
	realize_loop_nest(rhs, iters..., iter);
      } else {
	// loop
	if constexpr (BlockLike::IsBlock_T) {
	  auto extent = block_like.bextents[depth];	  
	  for (builder::dyn_var<loop_type> iter = 0; iter < extent; iter = iter + 1) {
	    realize_loop_nest(rhs, iters..., iter);
	  }
	} else {
	  auto extent = block_like.vextents[depth];
	  for (builder::dyn_var<loop_type> iter = 0; iter < extent; iter = iter + 1) {
	    realize_loop_nest(rhs, iters..., iter);
	  }
	}
      }
    } else {
      // at the innermost level
      if constexpr (std::is_same<Rhs, typename BlockLike::Elem_T>() ||
		    is_dyn_like<Rhs>()) {
	block_like.write(rhs, std::tuple{iters...});
      } else {
	block_like.write(rhs.realize(idxs, std::tuple{iters...}), std::tuple{iters...});	  
      }
    }
  }

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
  
};

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<loop_type> Block<Elem,Rank>::operator()(std::tuple<Iters...> iters) {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    auto coord = std::tuple_cat(make_tup<loop_type,0,pad_amt>(), iters);
    return this->operator()(coord);
  } else { // coordinate-based
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, iters);
    return data[lidx];
  }
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<loop_type> Block<Elem,Rank>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank>
template <typename ScalarElem, typename...Iters>
void Block<Elem,Rank>::write(ScalarElem val, std::tuple<Iters...> iters) {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    auto coord = std::tuple_cat(make_tup<loop_type,0,pad_amt>(), iters);
    this->write(val, coord);
  } else { // coordinate-based
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, iters);
    data[lidx] = val;
  }
}

template <typename Elem, int Rank>
template <typename Idx>
auto Block<Elem,Rank>::operator[](Idx idx) {
  return Ref<Block<Elem,Rank>,std::tuple<Idx>>(*this, std::tuple{idx});
}

template <typename Elem, int Rank>
template <typename LIdx>
builder::dyn_var<loop_type> Block<Elem,Rank>::plidx(LIdx lidx) {
  // don't need to delinearize here since the pseudo index is just a normal
  // index for a block
  return data[lidx];
}

template <typename Elem, int Rank>
auto Block<Elem,Rank>::view() {
  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			 this->bextents, this->bstrides, this->borigin,
			 this->data);
}

template <typename Elem, int Rank>
template <typename...Slices>
View<Elem,Rank> Block<Elem,Rank>::view(Slices...slices) {
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
  // now make everything relative to the prior block
  // new origin = old origin + vorigin * old strides
  SLoc_T origin = apply<AddFunctor,Rank>(this->borigin, apply<MulFunctor,Rank>(vorigin, this->bstrides));
  // new strides = old strides * new strides
  SLoc_T strides = apply<MulFunctor,Rank>(this->bstrides, vstrides);
  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			 vextents, strides, vorigin,
			 this->data);  
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<loop_type> View<Elem,Rank>::operator()(std::tuple<Iters...> iters) {
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
    auto biters = compute_block_relative_iters<Rank,0>(this->vstrides, this->vorigin, iters);
    // then linearize with respect to the block
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, biters);
    return data[lidx];
  }
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<loop_type> View<Elem,Rank>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank>
template <typename ScalarElem, typename...Iters>
void View<Elem,Rank>::write(ScalarElem val, std::tuple<Iters...> iters) {
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
    auto biters = compute_block_relative_iters<Rank,0>(this->vstrides, this->vorigin, iters);
    // then linearize with respect to the block
    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, biters);
    data[lidx] = val;
  }
}

template <typename Elem, int Rank>
template <typename Idx>
auto View<Elem,Rank>::operator[](Idx idx) {
  return Ref<View<Elem,Rank>,std::tuple<Idx>>(*this, std::tuple{idx});
}

template <typename Elem, int Rank>
template <typename LIdx>
builder::dyn_var<loop_type> View<Elem,Rank>::plidx(LIdx lidx) {
  // must delinearize relative to the view
  auto coord = delinearize<0>(lidx, this->vextents);
  return this->operator()(coord);
}

template <typename Elem, int Rank>
template <typename...Slices>
View<Elem,Rank> View<Elem,Rank>::view(Slices...slices) {
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
  SLoc_T origin = apply<AddFunctor,Rank>(this->vorigin, apply<MulFunctor,Rank>(vorigin, this->vstrides));
  // new strides = old strides * new strides
  SLoc_T strides = apply<MulFunctor,Rank>(this->vstrides, vstrides);
  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			 vextents, strides, origin,
			 this->data);
}

}
