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
struct BlockRef;

// Staged version
template <typename Elem, int Rank>
struct Block<Elem,Rank,true> : public BaseBlockLike<Elem,Rank> {

  static constexpr bool staged = true;

  builder::dyn_var<Elem*> data;

  Block(const std::array<loop_type, Rank> &bextents, const builder::dyn_var<Elem*> &data) :
    BaseBlockLike<Elem,Rank>(bextents, make_array<loop_type,1,Rank>(), 
			     make_array<loop_type,0,Rank>(), bextents), data(data) { }

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
    ss << "Block<" << Rank << ">" << std::endl;
    ss << "  Extents: " << join(this->bextents) << std::endl;
    ss << "  Strides: " << join(this->bstrides) << std::endl;
    ss << "  Origin:  " << join(this->borigin) << std::endl;
    return ss.str();
  }

};

// Expression template for lazy eval of inline functions
template <typename BlockLike, typename Idxs>
struct BlockRef {

  // Underlying block/view used to create this BlockRef
  BlockLike block_like;
  // Indices used to define this BlockRef
  Idxs idxs;

  BlockRef(BlockLike block_like, Idxs idxs) :
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
	for (; i < std::get<Depth>(block_like.bextents); i=i+1) {
	  loop<Depth+1>(rhs, iters..., i);
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

template <typename Elem, int Rank>
template <typename Idx>
auto Block<Elem,Rank,true>::operator[](Idx idx) {
  BlockRef<Block<Elem, Rank, true>, std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank>
template <typename Rhs, typename...Iters>
void Block<Elem,Rank,true>::assign(Rhs rhs, Iters...iters) {
  auto lidx = this->template linearize<0>(std::tuple{iters...});
  data[lidx] = rhs;
}

// for staging, this returns a dyn_var
template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<Elem> Block<Elem,Rank,true>::operator()(const std::tuple<Iters...> &iters) {
  // TODO verify that no Iters in here
  auto lidx = this->template linearize<0>(iters);
  return data[lidx];
}

template <typename Elem, int Rank>
template <typename...Iters>
builder::dyn_var<Elem> Block<Elem,Rank,true>::operator()(Iters...iters) {
  // TODO verify that no Iters in here
  return this->operator()(std::tuple{iters...});
}

template <typename BlockLike, typename Idxs>
template <typename Idx>
auto BlockRef<BlockLike, Idxs>::operator[](Idx idx) {
  auto merged = std::tuple_cat(idxs, std::tuple{idx});
  BlockRef<BlockLike, decltype(merged)> ref(this->block_like, std::move(merged));
  return ref;
}

namespace staged {

template <typename Elem, int Rank>
using Block = Block<Elem, Rank, true>;

}

}
