#pragma once

#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "builder/builder.h"
#include "builder/builder_context.h"

#include "traits.h"
#include "format.h"
#include "utils.h"
#include "fwd.h"

/*
 * The Hierarchical Data Types
 */


// Define necessary specialization for dyn_var<BlockLike>

constexpr char block_t_name[] = "Block";
using block_t = typename builder::name<block_t_name>;

namespace builder {

// This will purely be used to capture input arguments into the builder function pointers
template <>
class dyn_var<block_t>: public dyn_var_impl<block_t> {
public:
  typedef dyn_var_impl<block_t> super;
  using super::super;
  using super::operator=;
  dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}
  dyn_var(): dyn_var_impl<block_t>() {}
};

}

namespace hmda {

struct AddFunctor {
  template <typename Lhs, typename Rhs>
  auto operator()(const Lhs &lhs, const Rhs &rhs) {
    return lhs + rhs;
  }
};

struct MulFunctor {
  template <typename Lhs, typename Rhs>
  auto operator()(const Lhs &lhs, const Rhs &rhs) {
    return lhs * rhs;
  }
};

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
struct Block {
  
  using self = Block<Elem, Rank, BExtents, BStrides, BOrigin>;
  static const int rank = Rank;
  
  Block(const BExtents &bextents, const BStrides &bstrides, const BOrigin &borigin,
	const builder::dyn_var<Elem*> &data) 
    : bextents(bextents), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
      primary_extents(std::move(bextents)), data(data) {
    static_assert(is_tuple<BExtents>(), "Extents must be tuple");
    static_assert(is_tuple<BStrides>(), "Strides must be tuple");
    static_assert(is_tuple<BOrigin>(), "Origin must be tuple");
    static_assert(std::tuple_size<BExtents>() == Rank, "Rank/extent mismatch");
    static_assert(std::tuple_size<BStrides>() == Rank, "Rank/stride mismatch");
    static_assert(std::tuple_size<BOrigin>() == Rank, "Rank/origin mismatch");
  }
    
  const BExtents bextents; 
  const BStrides bstrides;
  const BOrigin borigin;
  const BExtents primary_extents;
  builder::dyn_var<Elem*> data;
  
  // Return a string giving a dump out of all the block information
  std::string dump(bool print_buffer=false) const;  

  // Just returns Block<Elem,Rank>
  std::string inline_dump() const;

  // If you use this indexing operator, it triggers staging when you 
  // evaluate it with Ref::operator=
  template <typename Idx>
  auto operator[](const Idx &idx);

  // If you use this indexing operator, it just does an on-demand access.
  // There is no staging, so you can't use things like Iter.
  template <typename...Idxs>
  Elem &operator()(int idx, Idxs...idxs);

  template <typename...Idxs>
  Elem &operator()(const std::tuple<Idxs...> &idxs);

  template <typename Rhs, typename...Iterators>
  void assign(const Rhs &rhs, const std::tuple<Iterators...> &idxs);
  
};

template <typename Elem, int Rank, typename...BExtents>
auto factory(const builder::dyn_var<Elem*> &buffer, BExtents...bextents) {
  auto bstrides = make_tuple<1, Rank>();
  auto borigin = make_tuple<0, Rank>();
  auto tbextents = std::tuple<BExtents...>(bextents...);
  return Block<Elem, Rank, decltype(tbextents), decltype(bstrides), decltype(borigin)>(tbextents, 
										       bstrides, 
										       borigin,
										       buffer);
}

// For building up a staged expression
// this follows what happens with ETs.
template <typename Derived>
struct Expr { 
  template <typename Rhs>
  Binary<AddFunctor, Expr<Derived>, Rhs> operator+(const Rhs &rhs) {
    return Binary<AddFunctor, Expr<Derived>, Rhs>(*this, rhs);
  }

};

template <typename Functor, typename Operand0, typename Operand1>
struct Binary : public Expr<Binary<Functor, Operand0, Operand1>> {
  Binary(const Operand0 &operand0, const Operand1 &operand1) :
    operand0(operand0), operand1(operand1) { }
  
  template <typename...Iterations, typename...LhsIdxs>
  auto realize(const std::tuple<Iterations...> &iters, const std::tuple<LhsIdxs...> &lhs_idxs) {
    auto op0 = operand0.realize(iters, lhs_idxs);
    auto op1 = operand1.realize(iters, lhs_idxs);
    return Functor()(op0, op1);
  }

  const Operand0 operand0;
  const Operand1 operand1;

};

// covers the entire extent of a dimension
template <char Ident>
struct Iter : public Expr<Iter<Ident>> { 

  template <typename...Iterations, typename...LhsIdxs>
  int realize(const std::tuple<Iterations...> &iters, const std::tuple<LhsIdxs...> &lhs_idxs);

  std::string inline_dump() const { return std::string(1, Ident); }
  
};

template <typename Rhs, typename...Iterators>
auto realize(const Rhs &rhs, const std::tuple<Iterators...> &iters) {
  if constexpr (std::is_arithmetic<Rhs>()) {
    return rhs; // just a scalar, no unwinding needed
  }
}

template <typename BlockLike, typename...Idxs>
struct Ref : public Expr<Ref<BlockLike, Idxs...>> {

  Ref(BlockLike &block_like) : block_like(block_like) { }
  
  template <typename Idx>
  auto operator[](const Idx &idx);

  // trigger stage generation 
  template <typename Rhs>
  void operator=(Rhs rhs) {
    basic_loop<0>(rhs, std::tuple{} /* no iterators at the top level yet!*/);
  }

  BlockLike block_like;
  std::tuple<Idxs...> idxs;

private:

  template <int Depth, typename Rhs, typename...Iterators>
  void basic_loop(Rhs rhs, const std::tuple<Iterators...> &iters) {
    if constexpr (Depth == BlockLike::rank) {
      // do the assignment
      block_like.assign(realize(rhs, iters), iters);
    } else {
      auto this_idx = std::get<Depth>(idxs);
      if constexpr (std::is_integral<decltype(this_idx)>()) {
	// scalar value at this dimension, only a single iteration
	auto new_iters = std::tuple_cat(iters, std::tuple{builder::static_var<int>(this_idx)});
	basic_loop<Depth+1>(rhs, new_iters);
      } else {
	// this is an Iter. The extent is the extent of the block_like
	auto extent = std::get<Depth>(block_like.primary_extents);
	for (builder::dyn_var<int> i = 0; i < extent; i = i + 1) {
	  auto new_iters = std::tuple_cat(iters, std::tuple{builder::dyn_var<int>(i)});
	  basic_loop<Depth+1>(rhs, new_iters);
	}
      }
    }
  }
  
};
  
/*
 * Block impls
 */

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
std::string Block<Elem, Rank, BExtents, BStrides, BOrigin>::dump(bool print_buffer) const {
  std::stringstream ss;
  ss << "Block<" << type_to_str<Elem>() << "," << Rank << ">" << std::endl;
  ss << "  Extents: {" << tuple_to_str(bextents) << "}" << std::endl;
  ss << "  Strides: {" << tuple_to_str(bstrides) << "}" << std::endl;
  ss << "  Origin: {" << tuple_to_str(borigin) << "}" << std::endl;
  if (print_buffer) {
    ss << "  Buffer:" << std::endl;
  }
  return ss.str();
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
std::string Block<Elem, Rank, BExtents, BStrides, BOrigin>::inline_dump() const {
  std::stringstream ss;
  ss << "Block<" << type_to_str<Elem>() << "," << Rank << ">" << std::endl;
  return ss.str();
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename Idx>
auto Block<Elem, Rank, BExtents, BStrides, BOrigin>::operator[](const Idx &idx) {
  Ref<self, std::tuple<Idx>> ref(*this);
  ref.idxs = std::tuple<Idx>{idx};
  static_assert(std::tuple_size<decltype(ref.idxs)>() == 1);
  return ref;
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename...Idxs>
Elem &Block<Elem, Rank, BExtents, BStrides, BOrigin>::operator()(const std::tuple<Idxs...> &idxs) {
  auto lidx = linearize<0>(this->bextents, idxs);
  return data[lidx];
}

template <typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
template <typename Rhs, typename...Iterators>
void Block<Elem, Rank, BExtents, BStrides, BOrigin>::assign(const Rhs &rhs, 
							    const std::tuple<Iterators...> &idxs) {
  auto lidx = linearize<0>(this->bextents, idxs);
  data[lidx] = rhs;
}

/*
 * Ref impls
 */

template <typename BlockLike, typename...Idxs>
auto ref_factory(BlockLike &block_like, const std::tuple<Idxs...> &idxs) {
  auto ref = Ref<BlockLike, Idxs...>(block_like);
  ref.idxs = move(idxs);
  return ref;
}

template <typename BlockLike, typename...Idxs>
template <typename Idx>
auto Ref<BlockLike, Idxs...>::operator[](const Idx &idx) {
  auto merged_indices = std::tuple_cat(this->idxs, std::tuple<Idx>{idx});
  auto ref = ref_factory(this->block_like, merged_indices);
  return ref;
}

/*
 * Expr impls
 */ 

template <char Ident>
template <typename...Iterations, typename...LhsIdxs>
int Iter<Ident>::realize(const std::tuple<Iterations...> &iters, const std::tuple<LhsIdxs...> &lhs_idxs) {    
  return find_iter<Ident, LhsIdxs...>(iters);
}

}
