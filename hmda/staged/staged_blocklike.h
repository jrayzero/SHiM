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

template <typename Elem, int Rank>
struct Block {

  using Loc_T = Wrapped<Rank,loop_type>;
  using Elem_T = Elem;
  static constexpr int Rank_T = Rank;
  static constexpr bool IsBlock_T = true;  

  Block(Loc_T bextents, Loc_T bstrides, Loc_T borigin) :
    bextents(std::move(bextents)), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
    data(malloc_func(bextents.template reduce<MulFunctor>()))
  { 
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(bextents.template reduce<MulFunctor>() * elem_size);
    memset_func(data, 0, sz);
  }

  Block(Loc_T bextents) :
    bextents(std::move(bextents)),
    bstrides(WrappedRecContainer<Rank,Elem>(RecContainer<Elem>::template build_from<1,Rank,0>())),
    borigin(WrappedRecContainer<Rank,Elem>(RecContainer<Elem>::template build_from<0,Rank,0>())),
    data(malloc_func(bextents.template reduce<MulFunctor>())) 
  { 
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(bextents.template reduce<MulFunctor>() * elem_size);
    memset_func(data, 0, sz);
  }

  Block(Loc_T bextents, builder::dyn_var<Elem[]> values) :
    bextents(std::move(bextents)),
    bstrides(WrappedRecContainer<Rank,Elem>(RecContainer<Elem>::template build_from<1,Rank,0>())),
    borigin(WrappedRecContainer<Rank,Elem>(RecContainer<Elem>::template build_from<0,Rank,0>())),
    data(malloc_func(bextents.template reduce<MulFunctor>())) 
  {
    int elem_size = sizeof(Elem);
    builder::dyn_var<int> sz(bextents.template reduce<MulFunctor>() * elem_size);
    memcpy_func(data, values, sz);
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

  template <typename...Iters>
  void write(Elem val, std::tuple<Iters...> iters);

  template <typename...Iters>
  void write(builder::dyn_var<Elem> val, std::tuple<Iters...> iters);

  // Return a simple view over the whole block
  auto view();

  template <typename Idx>
  auto operator[](Idx idx);

  builder::dyn_var<Elem*> data;
  Loc_T bextents;
  Loc_T bstrides;
  Loc_T borigin;

};

template <typename Elem, int Rank>
struct View {

  using Loc_T = Wrapped<Rank,loop_type>;
  using Elem_T = Elem;
  static constexpr int Rank_T = Rank;
  static constexpr bool IsBlock_T = false;

  View(Loc_T bextents, Loc_T bstrides, Loc_T borigin,
       Loc_T vextents, Loc_T vstrides, Loc_T vorigin,
       builder::dyn_var<Elem*> data) :
    bextents(bextents.deepcopy()), bstrides(bstrides), 
    borigin(borigin), vextents(vextents), 
    vstrides(vstrides), vorigin(vorigin),
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

  template <typename...Iters>
  void write(Elem val, std::tuple<Iters...> iters);

  template <typename...Iters>
  void write(builder::dyn_var<Elem> val, std::tuple<Iters...> iters);

  template <typename...Slices>
  View<Elem,Rank> view(Slices...slices);

  template <typename Idx>
  auto operator[](Idx idx);

  builder::dyn_var<Elem*> data;
  Loc_T bextents;
  Loc_T bstrides;
  Loc_T borigin;
  Loc_T vextents;
  Loc_T vstrides;
  Loc_T vorigin;

};

struct Foo { };

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
    // TODO can potentially memset this whole thing
    static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
    realize_loop_nest(x);
  }

  template <typename Rhs>
  void operator=(Rhs rhs) {
    static_assert(std::tuple_size<Idxs>() == BlockLike::Rank_T);
    realize_loop_nest(rhs);
  }
  
  template <int Depth, typename LhsIdxs, typename...Iters>
  auto realize_each(LhsIdxs lhs, std::tuple<Iters...> iters) {
    auto i = std::get<Depth>(idxs);
    if constexpr (std::is_same<decltype(i),builder::dyn_var<loop_type>>() ||
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

  template <typename Rhs, typename...Iters>
  void realize_loop_nest(Rhs rhs, Iters...iters) {
    // the lhs indices can be either an Iter or an integer.
    // Iter -> loop over whole extent
    // integer -> single iteration loop
    constexpr int rank = BlockLike::Rank_T;
    constexpr int depth = sizeof...(Iters);
    if constexpr (depth < rank) {
      auto dummy = std::get<depth>(idxs);
      if constexpr (std::is_integral<decltype(dummy)>()) {
	// single iter
	builder::dyn_var<loop_type> iter = std::get<depth>(idxs);//block_like.primary_extents.get<depth>();
	realize_loop_nest(rhs, iters..., iter);
      } else {
	// loop
	if constexpr (BlockLike::IsBlock_T) {
	  auto extent = block_like.bextents.template get<depth>();	  
	  for (builder::dyn_var<loop_type> iter = 0; iter < extent; iter = iter + 1) {
	    realize_loop_nest(rhs, iters..., iter);
	  }
	} else {
	  auto extent = block_like.vextents.template get<depth>();
	  for (builder::dyn_var<loop_type> iter = 0; iter < extent; iter = iter + 1) {
	    realize_loop_nest(rhs, iters..., iter);
	  }
	}
      }
    } else {
      // at the innermost level
      if constexpr (std::is_same<Rhs, typename BlockLike::Elem_T>() ||
		    std::is_same<Rhs, builder::dyn_var<typename BlockLike::Elem_T>>()) {
	block_like.write(rhs, std::tuple{iters...});
      } else {
	block_like.write(rhs.realize(idxs, std::tuple{iters...}), std::tuple{iters...});	  
      }
    }
  }
  
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
template <typename...Iters>
void Block<Elem,Rank>::write(Elem val, std::tuple<Iters...> iters) {
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
template <typename...Iters>
void Block<Elem,Rank>::write(builder::dyn_var<Elem> val, std::tuple<Iters...> iters) {
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
template <typename...Iters>
void View<Elem,Rank>::write(Elem val, std::tuple<Iters...> iters) {
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
template <typename...Iters>
void View<Elem,Rank>::write(builder::dyn_var<Elem> val, std::tuple<Iters...> iters) {
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
  auto vstops = Loc_T(gather_stops<0,Rank>(this->vextents, slices...));
  auto vstrides = Loc_T(gather_strides<0,Rank>(slices...));
  auto vorigin = Loc_T(gather_origin<0,Rank>(slices...));
  // convert vstops into extents
  auto vextents = convert_stops_to_extents<Rank>(vorigin, vstops, vstrides);
  // now make everything relative to the prior view
  // new origin = old origin + vorigin * old strides
  auto origin = apply<AddFunctor,Rank>(this->vorigin, apply<MulFunctor,Rank>(vorigin, this->vstrides));
  auto strides = apply<MulFunctor,Rank>(this->vstrides, vstrides);
  // new strides = old strides * new strides
  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			 vextents, strides, origin,
			 this->data);
}

/*
  template <typename Elem, int Rank>
  auto View<Elem,Rank>::view() {
  return *this;
  }

  //struct End { }; 
  //template <typename Start, typename Stop, typename Stride>
  //struct Slice {
  //  Start start;
  //  Stop stop;
  //  Stride stride;
  //  Slice(Start start, Stop stop, Stride stride) : start(start),
  //						 stop(stop),
  //						 stride(stride) { }
  //};
  //
  //template <typename Start, typename Stop, typename Stride>
  //auto slice(Start start, Stop stop, Stride stride) {
  //  return Slice(start, stop, stride);
  //}
  //
  //template <typename MaybeSlice>
  //struct IsSlice { constexpr bool operator()() { return false; } };
  //
  //template <typename A, typename B, typename C>
  //struct IsSlice<Slice<A,B,C>> { constexpr bool operator()() { return true; } };
  //
  //template <typename MaybeSlice>
  //constexpr bool is_slice() {
  //  return IsSlice<MaybeSlice>()();
  //}
  //
  //template <typename Functor, int Rank, int Depth>
  //void apply(std::vector<builder::dyn_var<loop_type>> &vec,
  //	   const std::vector<builder::dyn_var<loop_type>> &vec0, const std::vector<builder::dyn_var<loop_type>> &vec1) {
  //  if constexpr (Depth < Rank) {
  //    vec.push_back(Functor()(vec0[Depth], vec1[Depth]));
  //    apply<Functor,Rank,Depth+1>(vec, vec0, vec1);
  //  }
  //}
  //
  //template <typename Functor, int Rank>
  //std::vector<builder::dyn_var<loop_type>> apply(const std::vector<builder::dyn_var<loop_type>> &vec0, 
  //					       const std::vector<builder::dyn_var<loop_type>> &vec1) {
  //  std::vector<builder::dyn_var<loop_type>> vec;
  //  apply<Functor,Rank,0>(vec, vec0, vec1);
  //  return vec;
  //}
  //
  //template <int Idx, int Rank, typename Arg, typename...Args>
  //void gather_origin(std::vector<builder::dyn_var<loop_type>> &vec, Arg arg, Args...args) {
  //  if constexpr (is_slice<Arg>()) {
  //    // need to extract the Astart
  //    builder::dyn_var<loop_type> start = arg.start;
  //    static_assert(!std::is_same<End, decltype(start)>::value, "End only valid for the Stop parameter of Slice");
  //    vec.push_back(start);
  //    if constexpr (sizeof...(Args) > 0) {
  //      gather_origin<Idx+1,Rank>(vec, args...);
  //    }    
  //  } else {
  //    // the start is just the value
  //    builder::dyn_var<loop_type> start = arg;
  //    vec.push_back(start);
  //    if constexpr (sizeof...(Args) > 0) {
  //      gather_origin<Idx+1,Rank>(vec, args...);
  //    }    
  //  }
  //}
  //
  //template <int Idx, int Rank, typename Arg, typename...Args>
  //void gather_strides(std::vector<builder::dyn_var<loop_type>> &vec, Arg arg, Args...args) {
  //  if constexpr (is_slice<Arg>()) {
  //    // need to extract the stride
  //    builder::dyn_var<loop_type> stride = arg.stride;
  //    static_assert(!std::is_same<End, decltype(stride)>::value, "End only valid for the Stop parameter of Slice");
  //    vec.push_back(stride);
  //    if constexpr (sizeof...(Args) > 0) {
  //      gather_strides<Idx+1,Rank>(vec, args...);
  //    }     
  //  } else {
  //    // the stride is just one
  //    builder::dyn_var<loop_type> stride = 1;
  //    vec.push_back(stride);
  //    if constexpr (sizeof...(Args) > 0) {
  //      gather_strides<Idx+1,Rank>(vec, args...);
  //    }    
  //  }
  //}
  //
  //template <int Idx, int Rank, typename BlockLikeExtents, typename Arg, typename...Args>
  //void gather_stops(std::vector<builder::dyn_var<loop_type>> &vec, BlockLikeExtents extents, Arg arg, Args...args) {
  //  if constexpr (is_slice<Arg>()) {
  //    // need to extract the stop
  //    builder::dyn_var<loop_type> stop = arg.stop;
  //    if constexpr (std::is_same<End, decltype(stop)>::value) {
  //      vec.push_back(extents[Idx]);
  //      if constexpr (sizeof...(Args) > 0) {
  //	gather_stops<Idx+1,Rank>(vec, extents, args...);
  //      }      
  //    } else {
  //      vec.push_back(stop);
  //      if constexpr (sizeof...(Args) > 0) {
  //	gather_stops<Idx+1,Rank>(vec, extents, args...);
  //      }
  //    }
  //  } else {
  //    // the stop is just the value
  //    vec.push_back(arg);
  //    if constexpr (sizeof...(Args) > 0) {
  //      gather_stops<Idx+1,Rank>(vec, extents, args...);
  //    }    
  //  }
  //}
  //
  //template <int Depth, int Rank, typename Starts, typename Stops, typename Strides>
  //void convert_stops_to_extents(std::vector<builder::dyn_var<loop_type>> &vec, Starts starts, Stops stops, Strides strides) {
  //  builder::dyn_var<int(int)> floor_func("floor");
  //  if constexpr (Depth < Rank) {
  ////    builder::dyn_var<loop_type> extent = floor((stops[Depth] - starts[Depth] - (loop_type)1) / strides[Depth]) + (loop_type)1;
  //    builder::dyn_var<loop_type> extent = floor_func((stops[Depth] - starts[Depth] - (loop_type)1) / strides[Depth]) + (loop_type)1;
  //    vec.push_back(extent);
  //    convert_stops_to_extents<Depth+1,Rank>(vec, starts, stops, strides);
  //  }
  //}
  //
  //template <int Rank, typename Starts, typename Stops, typename Strides>
  //void convert_stops_to_extents(std::vector<builder::dyn_var<loop_type>> &vec, Starts starts, Stops stops, Strides strides) {
  //  convert_stops_to_extents<0,Rank>(vec, starts, stops, strides);
  //}
  //
  //template <typename T, T Val, int Rank>
  //std::vector<builder::dyn_var<T>> make_dyn_vec() {
  //  std::vector<builder::dyn_var<T>> vec;
  //  for (int i = 0; i < Rank; i++) {
  //    vec.push_back(Val);
  //  }
  //  return vec;
  //}
  //
  //template <int Depth, int Rank, typename Extents, typename...TupleTypes>
  //static builder::dyn_var<loop_type> linearize(const Extents &extents, const std::tuple<TupleTypes...> &coord) {
  //  builder::dyn_var<loop_type> c = std::get<Rank-1-Depth>(coord);
  //  if constexpr (Depth == Rank - 1) {
  //    return c;
  //  } else {
  //    return c + extents[Rank-1-Depth] * linearize<Depth+1,Rank>(extents, coord);
  //  }
  //}
  //
  //template <typename T, T Val, int N>
  //auto make_tup() {
  //  if constexpr (N == 0) {
  //    return std::tuple{};
  //  } else {
  //    return std::tuple_cat(std::tuple{Val}, make_tup<T, Val, N-1>());
  //  }
  //}
  //
  //template <int Depth, typename...Iters>
  //auto compute_block_relative_iters(const std::vector<builder::dyn_var<loop_type>> &vstrides, 
  //				  const std::vector<builder::dyn_var<loop_type>> &vorigin,
  //				  const std::tuple<Iters...> &viters) {
  //  if constexpr (Depth == sizeof...(Iters)) {
  //    return std::tuple{};
  //  } else {
  //    return std::tuple_cat(std::tuple{std::get<Depth>(viters) * vstrides[Depth] +
  //				     vorigin[Depth]}, 
  //      compute_block_relative_iters<Depth+1>(vstrides, vorigin, viters));
  //  }
  //}
  //
  //template <typename Elem, int Rank>
  //struct View;
  //
  //// Staged version
  //template <typename Elem, int Rank>
  //struct Block {
  //
  //  using vec_type = builder::dyn_var<loop_type[Rank]>;
  //  static constexpr int Rank_T = Rank;
  //
  //  // TODO need to implement some type of malloc wrapper over dyn_var Elem* array if you
  //  // make a block within the staged function
  //  // TODO would need to implement some type of memcpy wrapper over dyn_vars for building from array
  //
  //  Block(vec_type &bextents,
  //	vec_type &bstrides,
  //	vec_type &borigin) {
  //    // I want independent copies of the inputs
  //    for (int i = 0; i < Rank; i++) {
  //      this->bextents[i] = bextents[i];
  //      this->bstrides[i] = bstrides[i];
  //      this->borigin[i] = borigin[i];
  //    }
  //  }
  //
  ////  Block(vec_type bextents) {
  ////    for (int i = 0; i < Rank; i++) {
  ////      this->bextents.push_back(bextents[i]);
  ////    }
  ////    this->bstrides = make_dyn_vec<loop_type,1,Rank>();
  ////    this->borigin = make_dyn_vec<loop_type,0,Rank>();
  ////  }
  //
  //  // Create a Block from the specified raw array
  ////  Block(const vec_type &bextents, Elem values[]) : Block(bextents) {
  ////    int sz = reduce<MulFunctor>(bextents);
  ////    memcpy(data.base->data, values, sz * sizeof(Elem));
  ////  }  
  //
  //  Block<Elem,Rank>& operator=(const Block<Elem,Rank>&) = delete;
  //
  //  // Need a malloc on block
  ////  template <typename Elem2>
  ////  Block(const View<Elem2,Rank> &other); 
  //  
  //  builder::dyn_var<Elem*> data;
  //  vec_type bextents;
  //  vec_type bstrides;
  //  vec_type borigin;
  //
  //  ~Block() { }
  //
  //  // Access a single element
  //  template <typename...Iters>
  //  builder::dyn_var<loop_type> operator()(std::tuple<Iters...>);
  //
  //  // Access a single element
  //  template <typename...Iters>
  //  builder::dyn_var<loop_type> operator()(Iters...iters);
  //
  //  // Access a single element with a pseudo-linear index
  //  template <typename LIdx>
  //  builder::dyn_var<loop_type> plidx(LIdx lidx);
  //
  //  // Return a simple view over the whole block
  //  auto view();
  //
  //  // Return a slice based on the slice parameters
  //  template <typename...Slices>
  //  auto view(Slices...slices);
  //
  //  // Return a colocated view on this corresponding to block
  //  // The returned view contains:
  //  // this buffer
  //  // this b{extents,strides,origin}
  //  // block's b{extents,strides,origin} for the v{extents,strides,origin}
  //  template <typename Elem2>  
  //  auto colocate(const Block<Elem2,Rank> &block);
  //
  //  // Return a colocated view on this corresponding to view
  //  // The returned view contains:
  //  // this buffer
  //  // this b{extents,strides,origin}
  //  // view's v{extents,strides,origin} for the v{extents,strides,origin}
  //  template <typename Elem2>  
  //  auto colocate(const View<Elem2,Rank> &view);
  //
  //};
  //
  //
  //template <typename Elem, int Rank>
  //struct View {
  //
  //  using vec_type = std::vector<builder::dyn_var<loop_type>>;
  //  static constexpr int Rank_T = Rank;
  //
  //  ~View() { }
  //
  //  // make this private and have block as a friend so only block/view can actually
  //  // use this constructor. 
  //  View(const vec_type &bextents,
  //       const vec_type &bstrides,
  //       const vec_type &borigin,
  //       const vec_type &vextents,
  //       const vec_type &vstrides,
  //       const vec_type &vorigin,
  //       // data is shared
  //       builder::dyn_var<Elem*> data) : data(std::move(data)) {
  //    assert(bextents.size() == Rank);
  //    assert(bstrides.size() == Rank);
  //    assert(borigin.size() == Rank);
  //    assert(vextents.size() == Rank);
  //    assert(vstrides.size() == Rank);
  //    assert(vorigin.size() == Rank);    
  //    for (int i = 0; i < Rank; i++) {
  //      this->bextents.push_back(bextents[i]);
  //      this->bstrides.push_back(bstrides[i]);
  //      this->borigin.push_back(borigin[i]);
  //      this->vextents.push_back(vextents[i]);
  //      this->vstrides.push_back(vstrides[i]);
  //      this->vorigin.push_back(vorigin[i]);
  //    }
  //  }
  //  
  //  View<Elem,Rank>& operator=(const View<Elem,Rank>&) = delete;
  //
  //  builder::dyn_var<Elem*> data;
  //  vec_type bextents;
  //  vec_type bstrides;
  //  vec_type borigin;
  //  vec_type vextents;
  //  vec_type vstrides;
  //  vec_type vorigin;
  //
  //  // Access a single element
  //  template <typename...Iters>
  //  builder::dyn_var<loop_type> operator()(std::tuple<Iters...>);
  //
  //  // Access a single element
  //  template <typename...Iters>
  //  builder::dyn_var<loop_type> operator()(Iters...iters);
  //
  //  // Access a single element with a pseudo-linear index
  //  template <typename LIdx>
  //  builder::dyn_var<loop_type> plidx(LIdx lidx);
  //
  //  // Return copy of self
  //  auto view();
  //
  //  // Return a slice based on the slice parameters
  //  template <typename...Slices>
  //  auto view(Slices...slices);
  //
  //};
  //
  //template <typename Elem, int Rank>
  //template <typename Elem2>
  //Block<Elem,Rank>::Block(const View<Elem2,Rank> &other) : 
  //  BaseBlockLike<Elem,Rank>(other.vextents,
  //			   other.vstrides,
  //			   other.vorigin), data(reduce<MulFunctor>(this->bextents)) {
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename...Iters>
  //builder::dyn_var<loop_type> Block<Elem,Rank>::operator()(std::tuple<Iters...> iters) {
  //  static_assert(sizeof...(Iters) <= Rank);
  //  if constexpr (sizeof...(Iters) < Rank) {
  //    // we need padding at the front
  //    constexpr int pad_amt = Rank-sizeof...(Iters);
  //    auto coord = std::tuple_cat(make_tup<loop_type,0,pad_amt>(), iters);
  //    return this->operator()(coord);
  //  } else { // coordinate-based
  //    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, iters);
  //    return data[lidx];
  //  }
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename...Iters>
  //builder::dyn_var<loop_type> Block<Elem,Rank>::operator()(Iters...iters) {
  //  return this->operator()(std::tuple{iters...});
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename LIdx>
  //builder::dyn_var<loop_type> Block<Elem,Rank>::plidx(LIdx lidx) {
  //  // don't need to delinearize here since the pseudo index is just a normal
  //  // index for a block
  //  return data[lidx];
  //}
  //
  //template <typename Elem, int Rank>
  //auto Block<Elem,Rank>::view() {
  //  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
  //			 this->bextents, this->bstrides, this->borigin,
  //			 this->data);
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename...Slices>
  //auto Block<Elem,Rank>::view(Slices...slices) {
  //  // the block parameters stay the same, but we need to update the
  //  // view parameters
  //  vec_type vstops;
  //  vec_type vstrides;
  //  vec_type vorigin;
  //  vec_type vextents;
  //  
  //  gather_stops<0,Rank>(vstops, this->bextents, slices...);
  //  gather_strides<0,Rank>(vstrides, slices...);  
  //  gather_origin<0,Rank>(vorigin, slices...);
  //  // convert vstops into extents
  //  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  //  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
  //			 vextents, vstrides, vorigin,
  //			 this->data);
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename Elem2>
  //auto Block<Elem,Rank>::colocate(const Block<Elem2,Rank> &block) {  
  //  auto bextents = block.bextents;
  //  auto bstrides = block.bstrides;
  //  auto borigin = block.borigin;
  //  auto vextents = bextents;
  //  auto vstrides = bstrides;
  //  auto vorigin = borigin;
  //  return View<Elem,Rank>(bextents, bstrides, borigin, vextents, vstrides, vorigin, this->data);
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename Elem2>
  //auto Block<Elem,Rank>::colocate(const View<Elem2,Rank> &view) {  
  //  auto bextents = view.bextents;
  //  auto bstrides = view.bstrides;
  //  auto borigin = view.borigin;
  //  auto vextents = view.vextents;
  //  auto vstrides = view.vstrides;
  //  auto vorigin = view.vorigin;
  //  return View<Elem,Rank>(bextents, bstrides, borigin, vextents, vstrides, vorigin, this->data);
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename...Iters>
  //builder::dyn_var<loop_type> View<Elem,Rank>::operator()(std::tuple<Iters...> iters) {
  //  static_assert(sizeof...(Iters) <= Rank);
  //  if constexpr (sizeof...(Iters) < Rank) {
  //    // we need padding at the front
  //    constexpr int pad_amt = Rank-sizeof...(Iters);
  //    builder::dyn_var<loop_type> zero = 0;
  //    auto coord = std::tuple_cat(make_tup<builder::dyn_var<loop_type>,zero,pad_amt>(), iters);
  //    return this->operator()(coord);
  //  } else {
  //    // the iters are relative to the View, so first make them relative to the block
  //    // bi0 = vi0 * vstride0 + vorigin0
  //    auto biters = compute_block_relative_iters<0>(this->vstrides, this->vorigin, iters);
  //    // then linearize with respect to the block
  //    builder::dyn_var<loop_type> lidx = linearize<0,Rank>(this->bextents, biters);
  //    return data[lidx];
  //  }
  //}
  //
  //template <typename Elem, int Rank>
  //template <typename...Iters>
  //builder::dyn_var<loop_type> View<Elem,Rank>::operator()(Iters...iters) {
  //  return this->operator()(std::tuple{iters...});
  //}
  //*/
//// perform a reduction across a range of something that works with std::get 
//template <typename Functor, int Begin, int End, int Depth, typename Obj>
//auto reduce_region(const Obj &obj) {
//  constexpr int tuple_sz = std::tuple_size<Obj>();
//  if constexpr (Depth >= Begin && Depth < End) {
//    builder::dyn_var<loop_type> item = obj[Depth];
//    if constexpr (Depth < End - 1) {
//      return Functor()(item, reduce_region<Functor,Begin,End,Depth+1>(obj));
//    } else {
//      return item;
//    }
//  } else {
//    // won't be hit if Depth >= End
//    return reduce_region<Functor,Begin,End,Depth+1>(obj);
//  }
//}
//
//template <typename Functor, int Begin, int End, typename Obj>
//auto reduce_region(const Obj &obj) {
//  constexpr int tuple_sz = std::tuple_size<Obj>();
//  static_assert(Begin < tuple_sz && End <= tuple_sz && Begin < End);
//  static_assert(tuple_sz > 0);
//  return reduce_region<Functor,Begin,End,0>(obj);
//}
//
//template <int Depth, typename LIdx, typename Extents>
//auto delinearize(LIdx lidx, const Extents &extents) {
//  constexpr int tuple_size = std::tuple_size<Extents>();
//  if constexpr (Depth+1 == tuple_size) {
//    return std::tuple{lidx};
//  } else {
//    builder::dyn_var<loop_type> m = reduce_region<MulFunctor, Depth+1, tuple_size>(extents);
//    builder::dyn_var<loop_type> c = lidx / m;
//    return std::tuple_cat(std::tuple{c}, delinearize<Depth+1>(lidx % m, extents));
//  }
//}
//
///*
//template <typename Elem, int Rank>
//template <typename LIdx>
//builder::dyn_var<loop_type> View<Elem,Rank>::plidx(LIdx lidx) {
//  // must delinearize relative to the view
//  auto coord = delinearize<0>(lidx, this->vextents);
//  return this->operator()(coord);
//}
//
//template <typename Elem, int Rank>
//auto View<Elem,Rank>::view() {
//  return *this;
//}
//
//template <typename Elem, int Rank>
//template <typename...Slices>
//auto View<Elem,Rank>::view(Slices...slices) {
//  // the block parameters stay the same, but we need to update the
//  // view parameters
//  vec_type vstops;
//  vec_type vstrides;
//  vec_type vorigin;
//  vec_type vextents;
//  gather_stops<0,Rank>(vstops, this->vextents, slices...);
//  gather_strides<0,Rank>(vstrides, slices...);  
//  gather_origin<0,Rank>(vorigin, slices...);
//  // convert vstops into extents
//  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
//  // now make everything relative to the prior view
//  // new origin = old origin + vorigin * old strides
//  auto origin = apply<AddFunctor,Rank>(this->vorigin, apply<MulFunctor,Rank>(vorigin, this->vstrides));
//  auto strides = apply<MulFunctor,Rank>(this->vstrides, vstrides);
//  // new strides = old strides * new strides
//  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
//			 vextents, strides, origin,
//			 this->data);
//}
//*/
}
