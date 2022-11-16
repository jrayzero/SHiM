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

template <int Depth, int Rank, typename...TupleTypes>
auto linearize(const std::array<loop_type,Rank> &extents, const std::tuple<TupleTypes...> &coord) {
  auto c = std::get<Rank-1-Depth>(coord);
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + std::get<Rank-1-Depth>(extents) * linearize<Depth+1,Rank>(extents, coord);
  }
}

template <typename T>
struct DataProxy {
  using type = T;
};

  // If Staged = true, "data" is a builder::dyn_var<Elem*>
  // If Staged = false, "data" is Elem * (TODO ref counted array)
template <typename Elem, bool Staged>
constexpr auto determine_data_type() {
  if constexpr (Staged) {
    // it's a pointer for dyn_var because we create the staged block outside of the 
    // buildit context initially
    return DataProxy<builder::dyn_var<Elem*>>();
  } else {
    return DataProxy<Elem*>();
  }
}
template <typename Elem, int Rank>
struct BaseBlock {
  static const int rank = Rank;
  using elem = Elem;

  // The extents in each dimension of this block.
  std::array<loop_type,Rank> bextents;

  BaseBlock(const std::array<loop_type,Rank> &bextents) : bextents(bextents) {
    // TODO you'd create the actual data array in here (if not staged)    
  }

//  template <typename...Iters>
//  auto operator()(const std::tuple<Iters...> &tup);

  // TODO make this private and have block ref be a friend
//  template <typename Rhs, typename...Iters>
//  void assign(Rhs rhs, Iters...iters);

};

// unspecialized
template <typename Elem, int Rank, bool Staged>
struct Block : public BaseBlock<Elem, Rank> { };

// Unstaged version
template <typename Elem, int Rank>
struct Block<Elem,Rank,false> : public BaseBlock<Elem,Rank> {

  static constexpr bool staged = false;

  Block(const std::array<loop_type, Rank> &bextents) :
    BaseBlock<Elem,Rank>(bextents) { }

  Elem *data;


  // Take an unstaged block and create one with Staged = true, as well
  // as a dyn_var buffer. This must only be called within the staging context!
  auto stage(builder::dyn_var<Elem*> data);

};

// Staged version
template <typename Elem, int Rank>
struct Block<Elem,Rank,true> : public BaseBlock<Elem,Rank> {

  static constexpr bool staged = true;

//  using data_type = typename decltype(determine_data_type<Elem,Staged>())::type;  

  // The extents in each dimension of this block.
//  std::array<loop_type,Rank> bextents;
  // When Staged, this is a dyn_var, which essentially acts as a placeholder.
  // When not Staged, it's an actual array
  builder::dyn_var<Elem*> data;

  Block(const std::array<loop_type, Rank> &bextents, const builder::dyn_var<Elem*> &data) :
    BaseBlock<Elem,Rank>(bextents), data(data) { }

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

template <typename Elem, int Rank>
template <typename Idx>
auto Block<Elem,Rank,true>::operator[](Idx idx) {
  BlockRef<Block<Elem, Rank, true>, std::tuple<Idx>> ref(*this, std::tuple{idx});
  return ref;
}

template <typename Elem, int Rank>
auto Block<Elem,Rank,false>::stage(builder::dyn_var<Elem*> data) {
  return Block<Elem, Rank, true>(this->bextents, data);
}

template <typename Elem, int Rank>
template <typename Rhs, typename...Iters>
void Block<Elem,Rank,true>::assign(Rhs rhs, Iters...iters) {
  auto lidx = linearize<0,Rank>(this->bextents, std::tuple{iters...});
  data[lidx] = rhs;
}

// for staging, this returns a dyn_var
template <typename Elem, int Rank>
template <typename...Iters>
auto Block<Elem,Rank,true>::operator()(const std::tuple<Iters...> &iters) {
  auto lidx = linearize<0,Rank>(this->bextents, iters);
  return data[lidx];
}

template <typename BlockLike, typename Idxs>
template <typename Idx>
auto BlockRef<BlockLike, Idxs>::operator[](Idx idx) {
  auto merged = std::tuple_cat(idxs, std::tuple{idx});
  BlockRef<BlockLike, decltype(merged)> ref(this->block_like, std::move(merged));
  return ref;
}

template <typename I>
struct IsBlockOrView {
  constexpr bool operator()() { return false; }
};

template <typename Elem, int Rank, bool Staged>
struct IsBlockOrView<Block<Elem, Rank, Staged>> {
  constexpr bool operator()() { return true; }
};

template <typename T>
constexpr bool is_block_or_view() {
  return IsBlockOrView<T>()();
}

static void foo(builder::dyn_var<int*> B, int a, int b) {
  auto blockA = Block<int, 2, true>({a,b}, B);
  Iter<'i'> i;
  Iter<'j'> j;
  blockA[i][j] = i+j+blockA[i][j];
}

static void generate(block::block::Ptr ast, std::ostream &oss) {
  block::eliminate_redundant_vars(ast);	
  block::c_code_generator::generate_code(ast, oss, 0);
}

static void my_function_to_stage(builder::dyn_var<int*> data, Block<int,3,false> ublockA) {
  // must stage it first
  auto blockA = ublockA.stage(data);
  // then do your stuff
  Iter<'i'> i;
  Iter<'j'> j;
  Iter<'k'> k;
  blockA[i][j][k] = i+j;
}

// stage wrapper for a single unStaged block/view
// theoretically, this would do the staging and essentially jit it, running the code
template <typename Func, typename ArgTuple, typename Unstaged>
void stage(Func func, ArgTuple arg_tuple, Unstaged unstaged, std::string name="") {
  static_assert(is_block_or_view<Unstaged>());
  static_assert(!Unstaged::staged, "Trying to stage a staged object!");
  if (name.empty()) name = "__my_staged_func";
  if constexpr (std::tuple_size<ArgTuple>() == 0) {
    auto ast = builder::builder_context().extract_function_ast(func, name, unstaged);
    block::eliminate_redundant_vars(ast);	
    block::c_code_generator::generate_code(ast, std::cout, 0);
  } else {
    auto ast = builder::builder_context().extract_function_ast(func, name, arg_tuple, unstaged);
    block::eliminate_redundant_vars(ast);	
    block::c_code_generator::generate_code(ast, std::cout, 0);
  }
}


int main() {
  // Create an unstaged block that you can do whatever with
  auto my_block = Block<int, 3, false>({3,4,5});
  stage(my_function_to_stage, std::tuple{}, my_block, "fuzzy");

//  auto ast = builder::builder_context().extract_function_ast(foo, "foo", 4, 5);
//  generate(ast, std::cout);
}

