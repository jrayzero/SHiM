#pragma once

#include "builder/dyn_var.h"

#include "arrays.h"
#include "base.h"

namespace hmda {

template <typename Elem, int Rank>
struct View;

// Unstaged version
// BLOCKS DELETE THEIR DATA WHEN GO OUT OF SCOPE. So don't copy them all willy-nilly
template <typename Elem, int Rank>
struct Block : public BaseBlockLike<Elem,Rank> {

  using arr_type = std::array<loop_type,Rank>;
  static constexpr int Rank_T = Rank;
  
  Block(const arr_type &bextents,
	const arr_type &bstrides,
	const arr_type &borigin) :
    BaseBlockLike<Elem,Rank>(bextents, bstrides, borigin),
    data(reduce<MulFunctor>(bextents)) { 
  }

  Block(const arr_type &bextents) :
    BaseBlockLike<Elem,Rank>(bextents, make_array<loop_type,1,Rank>(),
			     make_array<loop_type,1,Rank>()),
    data(reduce<MulFunctor>(bextents)) { 
  }

  // Create a Block from the specified raw array
  Block(const arr_type &bextents, Elem values[]) : Block(bextents) {
    int sz = reduce<MulFunctor>(bextents);
    memcpy(data.base->data, values, sz * sizeof(Elem));
  }  

  Block<Elem,Rank>& operator=(const Block<Elem,Rank>&) = delete;

  template <typename Elem2>
  Block(const View<Elem2,Rank> &other); 

  HeapArray<Elem> data;

  void destroy() {
    delete[] data;
  }

  ~Block() { }

  // Be careful with this--lose the ref counting ability
  operator Elem*() {
    return data.base->data;
  }

  // Access a single element
  template <typename...Iters>
  Elem &operator()(const std::tuple<Iters...> &) const;

  // Access a single element
  template <typename...Iters>
  Elem &operator()(Iters...iters) const;

  // Access a single element with a pseudo-linear index
  Elem &plidx(loop_type lidx) const;

  // Take an unstaged block and create one with Staged = true, as well
  // as a dyn_var buffer. This must only be called within the staging context!
  auto stage(builder::dyn_var<Elem*> data);

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "Block<" << Rank << ">" << std::endl;
    ss << "  Extents: " << join(this->bextents) << std::endl;
    ss << "  Strides: " << join(this->bstrides) << std::endl;
    ss << "  Origin:  " << join(this->borigin) << std::endl;
    return ss.str();
  }

  // Return a simple view over the whole block
  auto view();

  // Return a slice based on the slice parameters
  template <typename...Slices>
  auto view(Slices...slices);

  // Return a colocated view on this corresponding to block
  // The returned view contains:
  // this buffer
  // this b{extents,strides,origin}
  // block's b{extents,strides,origin} for the v{extents,strides,origin}
  template <typename Elem2>  
  auto colocate(const Block<Elem2,Rank> &block);

  // Return a colocated view on this corresponding to view
  // The returned view contains:
  // this buffer
  // this b{extents,strides,origin}
  // view's v{extents,strides,origin} for the v{extents,strides,origin}
  template <typename Elem2>  
  auto colocate(const View<Elem2,Rank> &view);

  std::string dump_data() const {
    std::stringstream ss;
    int sz = reduce<MulFunctor>(this->bextents);
    for (int i = 0; i < sz; i++) {
      if (i > 0) ss << ",";
      ss << (*this)(i);
    }
    return ss.str();
  }

};

template <typename Elem, int Rank>
struct View : public BaseView<Elem, Rank> {

  using arr_type = std::array<loop_type,Rank>;
  static constexpr int Rank_T = Rank;

  ~View() { }

  View(const arr_type &bextents,
       const arr_type &bstrides,
       const arr_type &borigin,
       const arr_type &vextents,
       const arr_type &vstrides,
       const arr_type &vorigin,
       HeapArray<Elem> data) :
    BaseView<Elem, Rank>(bextents, bstrides, borigin, vextents, vstrides, vorigin),
    data(data) { }

  HeapArray<Elem> data;

  // Be careful with this--lose the ref counting ability
  operator Elem*() {
    return data.base->data;
  }

  // Access a single element
  template <typename...Iters>
  Elem &operator()(const std::tuple<Iters...> &) const;

  // Access a single element
  template <typename...Iters>
  Elem &operator()(Iters...iters) const;

  // Access a single element with a pseudo-linear index
  Elem &plidx(loop_type lidx) const;

  // Take an unstaged view and create one with Staged = true, as well
  // as a dyn_var buffer. This must only be called within the staging context!
  auto stage(builder::dyn_var<Elem*> data);

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "View<" << Rank << ">" << std::endl;
    ss << "  VExtents: " << join(this->vextents) << std::endl;
    ss << "  VStrides: " << join(this->vstrides) << std::endl;
    ss << "  VOrigin:  " << join(this->vorigin) << std::endl;
    ss << "  BExtents: " << join(this->bextents) << std::endl;
    ss << "  BStrides: " << join(this->bstrides) << std::endl;
    ss << "  BOrigin:  " << join(this->borigin) << std::endl;    
    return ss.str();
  }
  
  // Return copy of self
  auto view();

  // Return a slice based on the slice parameters
  template <typename...Slices>
  auto view(Slices...slices);

};

template <int Rank, int Depth=0>
auto tuplefy(const std::array<loop_type, Rank> &arr) {
  if constexpr (Rank == Depth) {
    return std::tuple{};
  } else {
    return tuple_cat(std::tuple{std::get<Depth>(arr)}, tuplefy<Rank,Depth+1>(arr));
  }
}

template <typename Elem, int Rank>
auto Block<Elem,Rank>::stage(builder::dyn_var<Elem*> data) {
  return SBlock<Elem, Rank>(this->bextents,
			    this->bstrides,
			    this->borigin,
			    data);
}

template <typename Elem, int Rank>
template <typename Elem2>
Block<Elem,Rank>::Block(const View<Elem2,Rank> &other) : 
  BaseBlockLike<Elem,Rank>(other.vextents,
			   other.vstrides,
			   other.vorigin), data(reduce<MulFunctor>(this->bextents)) {
}


template <typename Elem, int Rank>
template <typename...Iters>
Elem &Block<Elem,Rank>::operator()(const std::tuple<Iters...> &iters) const {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    auto coord = std::tuple_cat(make_tup<loop_type,0,pad_amt>(), iters);
    return this->operator()(coord);
  } else { // coordinate-based
    auto lidx = this->template linearize<0>(this->bextents, iters);
    return data[lidx];
  }
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &Block<Elem,Rank>::operator()(Iters...iters) const {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank>
Elem &Block<Elem,Rank>::plidx(loop_type lidx) const {
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
auto Block<Elem,Rank>::view(Slices...slices) {
  // the block parameters stay the same, but we need to update the
  // view parameters
  auto vstops = arr_type{};
  auto vstrides = arr_type{};
  auto vorigin = arr_type{};
  auto vextents = arr_type{};
  
  gather_stops<0,Rank>(vstops, this->bextents, slices...);
  gather_strides<0,Rank>(vstrides, slices...);  
  gather_origin<0,Rank>(vorigin, slices...);
  // convert vstops into extents
  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			 vextents, vstrides, vorigin,
			 this->data);
}

template <typename Elem, int Rank>
template <typename Elem2>
auto Block<Elem,Rank>::colocate(const Block<Elem2,Rank> &block) {  
  auto bextents = block.bextents;
  auto bstrides = block.bstrides;
  auto borigin = block.borigin;
  auto vextents = bextents;
  auto vstrides = bstrides;
  auto vorigin = borigin;
  return View<Elem,Rank>(bextents, bstrides, borigin, vextents, vstrides, vorigin, this->data);
}

template <typename Elem, int Rank>
template <typename Elem2>
auto Block<Elem,Rank>::colocate(const View<Elem2,Rank> &view) {  
  auto bextents = view.bextents;
  auto bstrides = view.bstrides;
  auto borigin = view.borigin;
  auto vextents = view.vextents;
  auto vstrides = view.vstrides;
  auto vorigin = view.vorigin;
  return View<Elem,Rank>(bextents, bstrides, borigin, vextents, vstrides, vorigin, this->data);
}

template <typename Elem, int Rank>
auto View<Elem,Rank>::stage(builder::dyn_var<Elem*> data) {
  return SView<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			  this->vextents, this->vstrides, this->vorigin,
			  data);
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &View<Elem,Rank>::operator()(const std::tuple<Iters...> &iters) const {
  static_assert(sizeof...(Iters) <= Rank);
  if constexpr (sizeof...(Iters) < Rank) {
    // we need padding at the front
    constexpr int pad_amt = Rank-sizeof...(Iters);
    auto coord = std::tuple_cat(make_tup<loop_type,0,pad_amt>(), iters);
    return this->operator()(coord);
  } else {
    // the iters are relative to the View, so first make them relative to the block
    // bi0 = vi0 * vstride0 + vorigin0
    auto biters = this->template compute_block_relative_iters<0>(iters);
    // then linearize with respect to the block
    auto lidx = this->template linearize<0>(this->bextents, biters);
    return data[lidx];
  }
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &View<Elem,Rank>::operator()(Iters...iters) const {
  return this->operator()(std::tuple{iters...});
}

template <int Depth, typename Extents>
auto delinearize(loop_type lidx, const Extents &extents) {
  constexpr int tuple_size = std::tuple_size<Extents>();
  if constexpr (Depth+1 == tuple_size) {
    return tuple{lidx};
  } else {
    loop_type m = reduce_region<MulFunctor, Depth+1, tuple_size>(extents);
    loop_type c = lidx / m;
    return tuple_cat(tuple{c}, delinearize<Depth+1>(lidx % m, extents));
  }
}

template <typename Elem, int Rank>
Elem &View<Elem,Rank>::plidx(loop_type lidx) const {
  // must delinearize relative to the view
  auto coord = delinearize<0>(lidx, this->vextents);
  return this->operator()(coord);
}

template <typename Elem, int Rank>
auto View<Elem,Rank>::view() {
  return *this;
}

template <typename Elem, int Rank>
template <typename...Slices>
auto View<Elem,Rank>::view(Slices...slices) {
  // the block parameters stay the same, but we need to update the
  // view parameters
  auto vstops = arr_type{};
  auto vstrides = arr_type{};
  auto vorigin = arr_type{};
  auto vextents = arr_type{};  
  gather_stops<0,Rank>(vstops, this->vextents, slices...);
  gather_strides<0,Rank>(vstrides, slices...);  
  gather_origin<0,Rank>(vorigin, slices...);
  // convert vstops into extents
  convert_stops_to_extents<Rank>(vextents, vorigin, vstops, vstrides);
  return View<Elem,Rank>(this->bextents, this->bstrides, this->borigin,
			 vextents, vstrides, vorigin,
			 this->data);
}

}
