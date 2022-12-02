#pragma once

#include "blocks/c_code_generator.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"

namespace hmda {

using loop_type = int32_t;

template <int Rank>
using Loc_T = builder::dyn_var<loop_type[Rank]>;

builder::dyn_var<int(int)> floor_func = builder::as_global("floor");
builder::dyn_var<void*(int)> malloc_func = builder::as_global("malloc");
builder::dyn_var<void(void*,int,int)> memset_func = builder::as_global("memset");
builder::dyn_var<void(void*,void*,int)> memcpy_func = builder::as_global("memcpy");

/*template <int Rank, typename T>
struct WrappedRecContainer;
 
// A recursive container for holding dyn_vars. Gets rid of weird issues with built-in data
// structures that screw up the semantics of the dyn_vars. I don't want to just
// use dyn_var<int[]> for holding things like extents b/c it generates code like x={2,4},
// and I don't want to rely on the compiler to extract the array elements.
template <typename T>
struct RecContainer {

  template <int Rank, typename T2>
  friend struct WrappedRecContainer;

  builder::dyn_var<T> val;
  std::unique_ptr<RecContainer<T>> nested;

  template <typename...Args>
  RecContainer(T arg, Args...args) : val(arg), nested(build_reccon(args...)) { }

  template <typename...Args>
  RecContainer(builder::dyn_var<T> arg, Args...args) : val(arg), nested(build_reccon(args...)) { }

  template <T Val, int Rank, int Idx=0>
  static auto build_from() {
    if constexpr (Idx == 0) {
      auto reccon = RecContainer<T>(Val);
      if constexpr (Rank > 1) {
	reccon.nested = build_from<Val,Rank,Idx+1>();
      }
      return reccon;
    } else if constexpr (Idx == Rank - 1) {
      auto reccon = std::make_unique<RecContainer<T>>(Val);
      return reccon;
    } else {
      auto reccon = std::make_unique<RecContainer<T>>(Val);
      reccon->nested = build_from<Val,Rank,Idx+1>();
      return reccon;
    }
  }

  template <typename Functor>
  builder::dyn_var<loop_type> reduce() {
    if (nested) {
      return Functor()(val, nested->template reduce<Functor>());
    } else {
      return val;
    }
  }

private:

  template <typename...Args>
  static std::unique_ptr<RecContainer<T>> build_reccon(T arg, Args...args) {
    return std::make_unique<RecContainer<T>>(arg, args...);
  }

  static std::unique_ptr<RecContainer<T>> build_reccon() {
    return nullptr;
  }

  template <int Rank, int Depth=Rank>
  auto deepcopy(bool deepcopy_dyn_var=false) const {
    if constexpr (Rank == 1) {
      if (deepcopy_dyn_var) {
	  builder::dyn_var<T> d;
	  d = this->val;
	  auto r = RecContainer<T>(d);
	  r.nested = nullptr;
	  return r; 
      } else {
	  builder::dyn_var<T> d = this->val;
	  auto r = RecContainer<T>(d);
	  r.nested = nullptr;
	  return r;
      }
    } else {
      if constexpr (Depth == Rank) {
	auto u = this->nested->template deepcopy<Rank,Depth-1>(deepcopy_dyn_var);
	if (deepcopy_dyn_var) {
	  builder::dyn_var<T> d;
	  d = this->val;
	  auto r = RecContainer<T>(d);
	  r.nested = move(u);
	  return r;
	} else {
	  builder::dyn_var<T> d = this->val;
	  auto r = RecContainer<T>(d);
	  r.nested = move(u);
	  return r;
	}
      } else if constexpr (Depth > 1) {
	auto u = this->nested->template deepcopy<Rank,Depth-1>(deepcopy_dyn_var);
	if (deepcopy_dyn_var) {
	  builder::dyn_var<T> d;
	  d = this->val;
	  auto r = std::make_unique<RecContainer<T>>(d);
	  r->nested = move(u);
	  return r;
	} else {
	  builder::dyn_var<T> d = this->val;
	  auto r = std::make_unique<RecContainer<T>>(d);
	  r->nested = move(u);
	  return r;
	}
      } else {
	if (deepcopy_dyn_var) {
	  builder::dyn_var<T> d;
	  d = this->val;
	  return std::make_unique<RecContainer<T>>(d);
	} else {
	  builder::dyn_var<T> d = this->val;
	  return std::make_unique<RecContainer<T>>(d);
	}
      }
    }
  }
	
  template <int Idx, int Depth=0>
  builder::dyn_var<T> get() const {
    if constexpr (Depth==Idx) {
      return this->val;
    } else {
      return this->nested->template get<Idx,Depth+1>();
    }
  }

};

// A wrapper for RecContainer that contains a Rank, which makes it easier to deep copy.
template <int Rank, typename T>
struct WrappedRecContainer {
 
  RecContainer<T> reccon;

  template <typename...Args>
  WrappedRecContainer(T arg, Args...args) : reccon(arg, args...) { 
    static_assert(1+sizeof...(Args) == Rank);
  }

  WrappedRecContainer(const WrappedRecContainer<Rank,T> &other) : reccon(other.reccon.template deepcopy<Rank>()) { }

  WrappedRecContainer(RecContainer<T> reccon) : reccon(std::move(reccon)) { }

  template <typename Functor>
  builder::dyn_var<loop_type> reduce() {
    return reccon.template reduce<Functor>();
  }

  auto deepcopy() { return WrappedRecContainer<Rank,T>(*this); }

  template <int Idx>
  builder::dyn_var<T> get() const {
    static_assert(Idx < Rank);
    return reccon.template get<Idx>();
  }

};

template <int Rank, typename T>
using Wrapped = WrappedRecContainer<Rank,T>;
*/
template <typename T, T Val, int N>
auto make_tup() {
  if constexpr (N == 0) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{Val}, make_tup<T, Val, N-1>());
  }
}

template <int Depth, int Rank, typename...TupleTypes>
static builder::dyn_var<loop_type> linearize(Loc_T<Rank> extents, const std::tuple<TupleTypes...> &coord) {
  builder::dyn_var<loop_type> c = std::get<Rank-1-Depth>(coord);
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + extents[Rank-1-Depth] * linearize<Depth+1,Rank>(extents, coord);
  }
}

// perform a reduction across a range of something that works with std::get 
template <typename Functor, int Begin, int End, int Depth, typename Obj>
auto reduce_region(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  if constexpr (Depth >= Begin && Depth < End) {
    builder::dyn_var<loop_type> item = obj[Depth];
    if constexpr (Depth < End - 1) {
      return Functor()(item, reduce_region<Functor,Begin,End,Depth+1>(obj));
    } else {
      return item;
    }
  } else {
    // won't be hit if Depth >= End
    return reduce_region<Functor,Begin,End,Depth+1>(obj);
  }
}

template <typename Functor, int Begin, int End, typename Obj>
auto reduce_region(const Obj &obj) {
  constexpr int tuple_sz = std::tuple_size<Obj>();
  static_assert(Begin < tuple_sz && End <= tuple_sz && Begin < End);
  static_assert(tuple_sz > 0);
  return reduce_region<Functor,Begin,End,0>(obj);
}

template <typename Functor, int Rank, typename T>
builder::dyn_var<T> reduce(builder::dyn_var<T[Rank]> arr) {
  builder::dyn_var<T> acc = arr[0];
  for (builder::static_var<T> i = 1; i < Rank; i=i+1) {
    acc = Functor()(acc, arr[i]);
  }
  return acc;
}

template <int Depth, typename LIdx, typename Extents>
auto delinearize(LIdx lidx, const Extents &extents) {
  constexpr int tuple_size = std::tuple_size<Extents>();
  if constexpr (Depth+1 == tuple_size) {
    return std::tuple{lidx};
  } else {
    builder::dyn_var<loop_type> m = reduce_region<MulFunctor, Depth+1, tuple_size>(extents);
    builder::dyn_var<loop_type> c = lidx / m;
    return std::tuple_cat(std::tuple{c}, delinearize<Depth+1>(lidx % m, extents));
  }
}

/*template <int Rank, int Depth, typename...Iters>
auto compute_block_relative_iters(const Wrapped<Rank,loop_type> &vstrides, 
				  const Wrapped<Rank,loop_type> &vorigin,
				  const std::tuple<Iters...> &viters) {
  if constexpr (Depth == sizeof...(Iters)) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{std::get<Depth>(viters) * vstrides.template get<Depth>() +
	  vorigin.template get<Depth>()}, 
      compute_block_relative_iters<Rank,Depth+1>(vstrides, vorigin, viters));
  }
}*/

template <int Rank, int Depth, typename...Iters>
auto compute_block_relative_iters(Loc_T<Rank> vstrides, 
				  Loc_T<Rank> vorigin,
				  const std::tuple<Iters...> &viters) {
  if constexpr (Depth == sizeof...(Iters)) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{std::get<Depth>(viters) * vstrides[Depth] +
	  vorigin[Depth]}, 
      compute_block_relative_iters<Rank,Depth+1>(vstrides, vorigin, viters));
  }
}

struct End { }; 

struct Slice {
  builder::dyn_var<loop_type> start;
  builder::dyn_var<loop_type> stop;
  builder::dyn_var<loop_type> stride;
  Slice(builder::dyn_var<loop_type> start, builder::dyn_var<loop_type> stop, builder::dyn_var<loop_type> stride) : start(start),
														   stop(stop),
														   stride(stride) { }
};

auto slice(builder::dyn_var<loop_type> start, builder::dyn_var<loop_type> stop, builder::dyn_var<loop_type> stride) {
  return Slice(start, stop, stride);
}

template <typename MaybeSlice>
struct IsSlice { constexpr bool operator()() { return false; } };

template <>
struct IsSlice<Slice> { constexpr bool operator()() { return true; } };

template <typename MaybeSlice>
constexpr bool is_slice() {
  return IsSlice<MaybeSlice>()();
}

/*template <typename Functor, int Rank, int Depth>
auto apply(const Wrapped<Rank,loop_type> &vec0, const Wrapped<Rank,loop_type> &vec1) {
  if constexpr (Depth < Rank) {
    auto applied = Functor()(vec0.template get<Depth>(), vec1.template get<Depth>());
    if constexpr (Depth == 0) {
      auto u = apply<Functor,Rank,Depth+1>(vec0, vec1);
      auto r = RecContainer<loop_type>(applied);
      r.nested = std::move(u);
      return r;
    } else if constexpr (Depth == Rank - 1) {
      // last one
      return std::make_unique<RecContainer<loop_type>>(applied);
    } else {
      auto u = apply<Functor,Rank,Depth+1>(vec0, vec1);
      auto r = std::make_unique<RecContainer<loop_type>>(applied);
      r->nested = std::move(u);
      return r;
    }
  }
}*/

template <typename Functor, int Rank, int Depth>
void apply(Loc_T<Rank> vec,
	   Loc_T<Rank> vec0, 
	   Loc_T<Rank> vec1) {
  auto applied = Functor()(vec0[Depth], vec1[Depth]);
  vec[Depth] = applied;
  if constexpr (Depth < Rank-1) {
    apply<Functor,Rank,Depth+1>(vec,vec0, vec1);
  }
}
 
//template <typename Functor, int Rank>
//Wrapped<Rank,loop_type> apply(const Wrapped<Rank,loop_type> &vec0, 
//			      const Wrapped<Rank,loop_type> &vec1) {
//  return Wrapped<Rank,loop_type>(apply<Functor,Rank,0>(vec0,vec1));
//}

template <typename Functor, int Rank>
Loc_T<Rank> apply(Loc_T<Rank> vec0, 
		  Loc_T<Rank> vec1) {
  Loc_T<Rank> vec;
  apply<Functor, Rank, 0>(vec, vec0, vec1);
  return vec;
//  return Wrapped<Rank,loop_type>(apply<Functor,Rank,0>(vec0,vec1));
}

template <int Idx, int Rank, typename Arg, typename...Args>
void gather_origin(Loc_T<Rank> vec, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    vec[Idx] = arg.start;
    if constexpr (Idx < Rank - 1) {
      gather_origin<Idx+1,Rank>(vec,args...);
    }
  } else {
    // the start is just the value
    vec[Idx] = arg;
    if constexpr (Idx < Rank - 1) {
      gather_origin<Idx+1,Rank>(vec,args...);
    }
  }
}


template <int Idx, int Rank, typename Arg, typename...Args>
void gather_strides(Loc_T<Rank> vec, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    vec[Idx] = arg.stride;
    if constexpr (Idx < Rank - 1) {
      gather_strides<Idx+1,Rank>(vec, args...);
    }
  } else {
    vec[Idx] = 1;
    if constexpr (Idx < Rank - 1) {
      gather_strides<Idx+1,Rank>(vec, args...);
    }
  }
}

template <int Idx, int Rank, typename Arg, typename...Args>
void gather_stops(Loc_T<Rank> vec, Loc_T<Rank> extents, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    builder::dyn_var<loop_type> stop = arg.stop;
    if constexpr (std::is_same<End, decltype(stop)>::value) {
      vec[Idx] = extents[Idx];
      if constexpr (Idx < Rank - 1) {
	gather_stops<Idx+1,Rank>(vec, extents, args...);
      }
    } else {
      vec[Idx] = stop;
      if constexpr (Idx < Rank - 1) {
	gather_stops<Idx+1,Rank>(vec, extents, args...);
      }
    }
  } else {
    vec[Idx] = arg;
    if constexpr (Idx < Rank - 1) {
      gather_stops<Idx+1,Rank>(vec, extents, args...);
    }    
  }
}

template <int Depth, int Rank>
void convert_stops_to_extents(Loc_T<Rank> arr, Loc_T<Rank> starts, Loc_T<Rank> stops, Loc_T<Rank> strides) {
  builder::dyn_var<loop_type> extent = floor_func((stops[Depth] - starts[Depth] - (loop_type)1) / 
						  strides[Depth]) + (loop_type)1;
  arr[Depth] = extent;
  if constexpr (Depth < Rank - 1) {
    convert_stops_to_extents<Depth+1,Rank>(arr, starts, stops, strides);
  }
}

template <int Rank>
Loc_T<Rank> convert_stops_to_extents(Loc_T<Rank> starts, Loc_T<Rank> stops, Loc_T<Rank> strides) {
  Loc_T<Rank> arr;
  convert_stops_to_extents<0, Rank>(arr, starts, stops, strides);
  return arr;
}

}
