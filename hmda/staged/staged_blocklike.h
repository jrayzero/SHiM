#pragma once

#include <sstream>

#include "builder/dyn_var.h"

#include "base.h"

namespace hmda {

// Notes
// - realize functions are not const for staged since they may lead to a call to Block::operator(), which
// would update the dyn_var for data when you return it (b/c it needs to record that dyn_var does something)
//   - this is also why the Rhs arguments are not const

template <typename BlockLike, typename Idxs>
struct BlockLikeRef;

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
struct SView;

// Staged version
template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
struct SBlock : public BaseBlockLike<Elem,Rank,BExtents,BStrides,BOrigin> {

  builder::dyn_var<Elem*> data;

  SBlock(const BExtents &bextents,
	 const BStrides &bstrides,
	 const BOrigin &borigin,
	 const builder::dyn_var<Elem*> &data) :
    BaseBlockLike<Elem,Rank,BExtents,BStrides,BOrigin>(bextents, bstrides, borigin), data(data) { }
  
  // Use the square brackets when you want inline indexing
  template <typename Idx>
  auto operator[](Idx idx);
  
  // Use these when you just want a single item
  template <typename...Iters>
  builder::dyn_var<Elem> operator()(const std::tuple<Iters...> &);

  template <typename...Iters>
  builder::dyn_var<Elem> operator()(Iters...iters);

  // TODO make this private and have block ref be a friend
  template <typename Rhs, typename...Iters>
  void assign(Rhs rhs, Iters...iters);

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "SBlock<" << Rank << ">" << std::endl;
    ss << "  Extents: " << join_tup(this->bextents) << std::endl;
    ss << "  Strides: " << join_tup(this->bstrides) << std::endl;
    ss << "  Origin:  " << join_tup(this->borigin) << std::endl;
    return ss.str();
  }

  auto view();

  // Return a slice based on the slice parameters
  template <typename...Slices>
  auto view(Slices...slices);  

};

// Staged version
template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
struct SView : public BaseView<Elem, Rank, BExtents, BStrides, BOrigin,
	  VExtents, VStrides, VOrigin> {
  
  builder::dyn_var<Elem*> data;

  SView(const BExtents &bextents,
	const BStrides &bstrides,
	const BOrigin &borigin,
	const VExtents &vextents,
	const VStrides &vstrides,
	const VOrigin &vorigin,
	const builder::dyn_var<Elem*> data) :
    BaseView<Elem, Rank, BExtents, BStrides, BOrigin, VExtents, VStrides, VOrigin>(bextents, bstrides, borigin,
										   vextents, vstrides, vorigin),
    data(data) { }

  // Use the square brackets when you want inline indexing
  template <typename Idx>
  auto operator[](Idx idx);

  // Use these when you just want a single item
  template <typename...Iters>
  builder::dyn_var<Elem> operator()(const std::tuple<Iters...> &);

  template <typename...Iters>
  builder::dyn_var<Elem> operator()(Iters...iters);

  // TODO make this private and have block ref be a friend
  template <typename Rhs, typename...Iters>
  void assign(Rhs rhs, Iters...iters);

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "SView<" << Rank << ">" << std::endl;
    ss << "  VExtents: " << join_tup(this->vextents) << std::endl;
    ss << "  VStrides: " << join_tup(this->vstrides) << std::endl;
    ss << "  VOrigin:  " << join_tup(this->vorigin) << std::endl;
    ss << "  BExtents: " << join_tup(this->bextents) << std::endl;
    ss << "  BStrides: " << join_tup(this->bstrides) << std::endl;
    ss << "  BOrigin:  " << join_tup(this->borigin) << std::endl; 
    return ss.str();
  }

  auto view();

  // Return a slice based on the slice parameters
  template <typename...Slices>
  auto view(Slices...slices);  

};

// Expression template for lazy eval of inline functions
template <typename BlockLike, typename Idxs>
struct BlockLikeRef {

  // Underlying block/view used to create this BlockLikeRef
  BlockLike block_like;
  // Indices used to define this BlockLikeRef
  Idxs idxs;

  BlockLikeRef(BlockLike block_like, Idxs idxs) :
    block_like(std::move(block_like)), idxs(std::move(idxs)) { }

  template <typename Idx>
  auto operator[](Idx idx);

  void operator=(typename BlockLike::elem x) {
    // TODO if we have a block, we can just do a memset on the whole thing, or if 
    // we statically know that there is a stride of 1
    loop<0>(x);
  }

  // some type of expression
  template <typename Rhs>
  void operator=(Rhs rhs) {
    loop<0>(rhs);
  }

  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    // realize the individual indices and then use those to access block_like
    auto ridxs = realize_idxs<0>(lhs_idxs, iters);
    return block_like(ridxs);
  }

private:
  
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
	auto realized = my_idx.realize(lhs_idxs, iters);
	return std::tuple_cat(std::tuple{realized}, realize_idxs<Depth+1>(lhs_idxs, iters));	
      }
    }
  }

  // a basic loop evaluation that just nests everything at the inner most level
  template <int Depth, typename Rhs, typename...Iters>
  void loop(Rhs &rhs, Iters...iters) {
    if constexpr (Depth == BlockLike::rank) {
      if constexpr (std::is_same<typename BlockLike::elem, Rhs>()) {
	block_like.assign(rhs, iters...);
      } else {
	block_like.assign(rhs.realize(idxs, std::tuple{iters...}), iters...);
      }
    } else {
      auto cur_idx = std::get<Depth>(idxs);
      if constexpr (is_iter<decltype(cur_idx)>()) {
	// make a loop
	builder::dyn_var<loop_type> i = 0;
	if constexpr (is_any_block<BlockLike>()) {
	  for (; i < std::get<Depth>(block_like.bextents); i=i+1) {
	    loop<Depth+1>(rhs, iters..., i);
	  }
	} else {
	  // it's a view
	  for (; i < std::get<Depth>(block_like.vextents); i=i+1) {
	    loop<Depth+1>(rhs, iters..., i);
	  }
	}
      } else {
	// this is a single iteration loop with value
	// cur_idx
	builder::dyn_var<loop_type> i = cur_idx;
	loop<Depth+1>(rhs, iters..., i);
      }
    }
  }
  
};

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
auto SBlock<Elem,Rank,BExtents,BStrides,BOrigin>::view() {
  return SView<Elem,Rank,BExtents,BStrides,BOrigin,BExtents,BStrides,BOrigin>(this->bextents, this->bstrides, this->borigin,
									      this->bextents, this->bstrides, this->borigin,
									      this->data);
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename...Slices>
auto SBlock<Elem,Rank,BExtents,BStrides,BOrigin>::view(Slices...slices) {
  auto vstops = gather_stops<0>(this->bextents, slices...);
  auto vstrides = gather_strides(slices...);  
  auto vorigin = gather_origin(slices...);
  // convert vstops into extents
  auto vextents = convert_stops_to_extents(vorigin, vstops, vstrides);
  auto v = SView<Elem,Rank,BExtents,BStrides,BOrigin,
		 decltype(vextents),decltype(vstrides),decltype(vorigin)>(this->bextents, this->bstrides, this->borigin,
									  vextents, vstrides, vorigin,
									  this->data);
  //  v.foo();
  return v;
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename Idx>
auto SBlock<Elem,Rank,BExtents,BStrides,BOrigin>::operator[](Idx idx) {
  BlockLikeRef<SBlock<Elem, Rank, BExtents, BStrides, BOrigin>, std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename Rhs, typename...Iters>
void SBlock<Elem,Rank,BExtents,BStrides,BOrigin>::assign(Rhs rhs, Iters...iters) {
  auto lidx = this->template linearize<0>(std::tuple{iters...});
  data[lidx] = rhs;
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename...Iters>
builder::dyn_var<Elem> SBlock<Elem,Rank,BExtents,BStrides,BOrigin>::operator()(const std::tuple<Iters...> &iters) {
  auto lidx = this->template linearize<0>(iters);
  return data[lidx];
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename...Iters>
builder::dyn_var<Elem> SBlock<Elem,Rank,BExtents,BStrides,BOrigin>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
template <typename Idx>
auto SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>::operator[](Idx idx) {
  BlockLikeRef<SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>,
	       std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
template <typename Rhs, typename...Iters>
void SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>::assign(Rhs rhs, Iters...iters) {
  // the iters are relative to the View, so first make them relative to the block
  // bi0 = vi0 * vstride0 + vorigin0
  auto biters = this->template compute_block_relative_iters<0>(std::tuple{iters...});
  // then linearize with respect to the block
  auto lidx = this->template linearize<0>(this->bextents, biters);  
  data[lidx] = rhs;
}

template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
template <typename...Iters>
builder::dyn_var<Elem> SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>::operator()(const std::tuple<Iters...> &iters) {
  // the iters are relative to the View, so first make them relative to the block
  // bi0 = vi0 * vstride0 + vorigin0
  auto biters = this->template compute_block_relative_iters<0>(iters);
  // then linearize with respect to the block
  auto lidx = this->template linearize<0>(this->bextents, biters);
  return data[lidx];
}

template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
template <typename...Iters>
builder::dyn_var<Elem> SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>::operator()(Iters...iters) {
  return this->operator()(std::tuple{iters...});
}

template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin
	  >
auto SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>::view() {
  return SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>(this->bextents, this->bstrides, this->borigin,
									      this->vextents, this->vstrides, this->vorigin,
									      this->data);
}

template <typename Elem, int Rank,
	  typename BExtents, typename BStrides, typename BOrigin,
	  typename VExtents, typename VStrides, typename VOrigin>
template <typename...Slices>
auto SView<Elem,Rank,BExtents,BStrides,BOrigin,VExtents,VStrides,VOrigin>::view(Slices...slices) {
  auto vstops = gather_stops<0>(this->vextents, slices...);
  auto vstrides = gather_strides(slices...);  
  auto vorigin = gather_origin(slices...);
  // convert vstops into extents
  auto vextents = convert_stops_to_extents(vorigin, vstops, vstrides);
  return SView<Elem,Rank,BExtents,BStrides,BOrigin,
	       decltype(vextents),decltype(vstrides),decltype(vorigin)>(this->bextents, this->bstrides, this->borigin,
									vextents, vstrides, vorigin,
									this->data);
}

template <typename BlockLike, typename Idxs>
template <typename Idx>
auto BlockLikeRef<BlockLike, Idxs>::operator[](Idx idx) {
  auto merged = std::tuple_cat(idxs, std::tuple{idx});
  BlockLikeRef<BlockLike, decltype(merged)> ref(this->block_like, std::move(merged));
  return ref;
}

}
