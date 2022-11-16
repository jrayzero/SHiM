#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/extract_cuda.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include <array>

using loop_type = int32_t;

// Notes
// - realize functions are not const for staged since they may lead to a call to Block::operator(), which
// would update the dyn_var for data when you return it (b/c it needs to record that dyn_var does something)
//   - this is also why the Rhs arguments are not const

struct AddFunctor {
  template <typename L, typename R>
  auto operator()(const L &l, const R &r) {
    return l + r;
  }
};

template <typename Derived>
struct Expr;
template <typename Functor, typename Operand0, typename Operand1>
struct Binary;
template <typename BlockLike, typename Idxs>
struct BlockRef;

template <typename Derived>
struct Expr {

  template <typename Rhs>
  Binary<AddFunctor, Derived, Rhs> operator+(const Rhs &rhs) {
    return Binary<AddFunctor, Derived, Rhs>(*static_cast<Derived*>(this), rhs);
  }
  
};

template <typename Functor, typename Operand0, typename Operand1>
struct Binary : public Expr<Binary<Functor, Operand0, Operand1>> {

  Binary(const Operand0 &operand0, const Operand1 &operand1) :
    operand0(operand0), operand1(operand1) { }

  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    auto o0 = operand0.realize(lhs_idxs, iters);
    auto o1 = operand1.realize(lhs_idxs, iters);
    return Functor()(o0, o1);
  }
  
private:
  
  Operand0 operand0;
  Operand1 operand1;
  
};

template <char Ident>
struct Iter : public Expr<Iter<Ident>> {

  template <typename LhsIdxs, typename Iters>
  auto realize(const LhsIdxs &lhs_idxs, const Iters &iters) {
    // figure out the index of Ident within lhs_idxs so I can get the correct
    // iter
    constexpr int idx = find_iter_idx<LhsIdxs>();
    return std::get<idx>(iters);
  }

private:

  template <typename LhsIdxs>
  struct FindIterIdxProxy { };

  template <typename...LhsIdxs>
  struct FindIterIdxProxy<std::tuple<LhsIdxs...>> {
    constexpr int operator()() const { return FindIterIdx<0, LhsIdxs...>()(); }
  };
  
  template <int Depth, typename LhsIdx, typename...LhsIdxs>
  struct FindIterIdx {
    constexpr int operator()() const {
      if constexpr (sizeof...(LhsIdxs) == 0) {
	return -1; // not found
      } else {
	return FindIterIdx<Depth+1, LhsIdxs...>()();
      }
    }
  };

  template <int Depth, typename...LhsIdxs>
  struct FindIterIdx<Depth, Iter<Ident>, LhsIdxs...> {
    constexpr int operator()() const { return Depth; }
  };

  template <typename LhsIdxs>
  static constexpr int find_iter_idx() {
    constexpr int idx = FindIterIdxProxy<LhsIdxs>()();
    static_assert(idx != -1);
    return idx;
  }
  
};

template <char Ident, typename Functor>
struct Redux {

};

template <typename I>
struct IsIterType {
  constexpr bool operator()() { return false; }
};

template <char Ident>
struct IsIterType<Iter<Ident>> {
  constexpr bool operator()() { return true; }
};

template <typename T>
constexpr bool is_iter() {
  return IsIterType<T>()();
}

template <typename I>
struct IsReduxType {
  constexpr bool operator()() { return false; }
};

template <char Ident, typename Functor>
struct IsReduxType<Redux<Ident, Functor>> {
  constexpr bool operator()() { return true; }
};

template <typename T>
constexpr bool is_redux() {
  return IsReduxType<T>()();
}

// apply a reduction across the elements within a specific range
template <typename Functor, int Start, int Stop, int Depth, typename...TupleTypes>
auto reduce_region(const std::tuple<TupleTypes...> &tup) {
  static_assert(Start < Stop);
  static_assert(Stop <= sizeof...(TupleTypes));
  if constexpr (Depth >= Start && Depth < Stop) {
    auto elem = std::get<Depth>(tup);    
    if constexpr (Depth < Stop - 1) {
      return Functor()(elem, reduce_region<Functor, Start, Stop, Depth+1>(tup));      
    } else {
      return elem;
    }
  } else {
    return reduce_region<Functor, Start, Stop, Depth + 1>(tup);
  }
}

template <int Depth, typename...ExtentTypes, typename...CoordTypes>
auto linearize(const std::tuple<ExtentTypes...> &extents, const std::tuple<CoordTypes...> &coord) {
  constexpr int sz = sizeof...(ExtentTypes);
  auto c = std::get<sz-1-Depth>(coord);
  if constexpr (Depth == sz - 1) {
    return c;
  } else {
    return c + std::get<sz-1-Depth>(extents) * linearize<Depth+1>(extents, coord);
  }
}

template <typename Elem, int Rank, typename BExtents>
struct Block {

  static const int rank = Rank;
  using elem = Elem;

  // The extents in each dimension of this block.
  BExtents bextents;
  // A placeholder for data. Primary component of the staged block
  builder::dyn_var<Elem *> data;

  Block(const BExtents &bextents, const builder::dyn_var<Elem*> &data) :
    bextents(bextents), data(data) { }

  template <typename Idx>
  auto operator[](Idx idx);

  template <typename...Iters>
  auto operator()(const std::tuple<Iters...> &tup);

  // TODO make this private and have block ref be a friend
  template <typename Rhs, typename...Iters>
  void assign(Rhs rhs, Iters...iters);

};

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

template <typename Elem, int Rank, typename BExtents>
template <typename Idx>
auto Block<Elem,Rank,BExtents>::operator[](Idx idx) {
  BlockRef<Block<Elem, Rank, BExtents>, std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank, typename BExtents>
template <typename Rhs, typename...Iters>
void Block<Elem,Rank,BExtents>::assign(Rhs rhs, Iters...iters) {
  auto lidx = linearize<0>(this->bextents, std::tuple{iters...});
  this->data[lidx] = rhs;
}

// for staging, this returns a dyn_var
template <typename Elem, int Rank, typename BExtents>
template <typename...Iters>
auto Block<Elem,Rank,BExtents>::operator()(const std::tuple<Iters...> &iters) {
  auto lidx = linearize<0>(this->bextents, iters);
  return this->data[lidx];
}

template <typename BlockLike, typename Idxs>
template <typename Idx>
auto BlockRef<BlockLike, Idxs>::operator[](Idx idx) {
  auto merged = std::tuple_cat(idxs, std::tuple{idx});
  BlockRef<BlockLike, decltype(merged)> ref(this->block_like, std::move(merged));
  return ref;
}

static void foo(builder::dyn_var<int*> A, builder::dyn_var<int*> B, int a, int b) {
  auto blockA = Block<int, 2, std::tuple<int,int>>(std::tuple{a,b}, A);
  Iter<'i'> i;
  Iter<'j'> j;
  blockA[i][j] = i+j+blockA[i][j];
}

static void generate(block::block::Ptr ast, std::ostream &oss) {
  block::eliminate_redundant_vars(ast);	
  block::c_code_generator::generate_code(ast, oss, 0);
}

int main() {
  auto ast = builder::builder_context().extract_function_ast(foo, "foo", 4, 5);
  generate(ast, std::cout);
}

