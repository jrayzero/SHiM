#pragma once

#include "builder/dyn_var.h"

#include "base.h"

namespace hmda {

template <typename Elem, int Rank>
struct View;

// Unstaged version
template <typename Elem, int Rank>
struct Block : public BaseBlockLike<Elem,Rank> {

  using arr_type = std::array<loop_type,Rank>;
  
  Block(const arr_type &bextents,
	const arr_type &bstrides,
	const arr_type &borigin) :
    BaseBlockLike<Elem,Rank>(bextents, bstrides, borigin) {
    // TODO allocated ref counter buffer
  }
  
  Elem *data;

  // Access a single element
  template <typename...Iters>
  Elem &operator()(const std::tuple<Iters...> &);

  // Access a single element
  template <typename...Iters>
  Elem &operator()(Iters...iters);

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

};

template <typename Elem, int Rank>
struct View : public BaseView<Elem, Rank> {

  using arr_type = std::array<loop_type,Rank>;

  View(const arr_type &bextents,
       const arr_type &bstrides,
       const arr_type &borigin,
       const arr_type &vextents,
       const arr_type &vstrides,
       const arr_type &vorigin,
       Elem *data) :
    BaseView<Elem, Rank>(bextents, bstrides, borigin, vextents, vstrides, vorigin),
    data(data) { }

  // TODO ref counted
  Elem *data;

  // Access a single element
  template <typename...Iters>
  Elem &operator()(const std::tuple<Iters...> &);

  // Access a single element
  template <typename...Iters>
  Elem &operator()(Iters...iters);

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
template <typename...Iters>
Elem &Block<Elem,Rank>::operator()(const std::tuple<Iters...> &iters) {
  // TODO verify that no Iters in here (only loop_type)
  auto lidx = this->template linearize<0>(iters);
  return data[lidx];
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &Block<Elem,Rank>::operator()(Iters...iters) {
  // TODO verify that no Iters in here (only loop_type)
  return this->operator()(std::tuple{iters...});
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
			  this->vextents, this->vstrides, this->vorigin.
			  data);
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &View<Elem,Rank>::operator()(const std::tuple<Iters...> &iters) {
  // TODO verify that no Iters in here (only loop_type)
  // the iters are relative to the View, so first make them relative to the block
  // bi0 = vi0 * vstride0 + vorigin0
  auto biters = this->template compute_block_relative_iters<0>(iters);
  // then linearize with respect to the block
  auto lidx = this->template linearize<0>(this->bextents, biters);
  return data[lidx];
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &View<Elem,Rank>::operator()(Iters...iters) {
  // TODO verify that no Iters in here (only loop_type)
  return this->operator()(std::tuple{iters...});
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
