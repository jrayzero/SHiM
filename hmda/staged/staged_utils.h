#pragma once

#include "blocks/c_code_generator.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"

namespace hmda {

using loop_type = int32_t;

builder::dyn_var<int(int)> floor_func = builder::as_global("floor");

template <int Rank, typename T>
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

  auto deepcopy() { return WrappedRecContainer<Rank,T>(*this); }

  template <int Idx>
  builder::dyn_var<T> get() const {
    static_assert(Idx < Rank);
    return reccon.template get<Idx>();
  }

};

template <int Rank, typename T>
using Wrapped = WrappedRecContainer<Rank,T>;

template <typename T, T Val, int N>
auto make_tup() {
  if constexpr (N == 0) {
    return std::tuple{};
  } else {
    return std::tuple_cat(std::tuple{Val}, make_tup<T, Val, N-1>());
  }
}

template <int Depth, int Rank, typename Extents, typename...TupleTypes>
static builder::dyn_var<loop_type> linearize(const Extents &extents, const std::tuple<TupleTypes...> &coord) {
  builder::dyn_var<loop_type> c = std::get<Rank-1-Depth>(coord);
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + extents.template get<Rank-1-Depth>() * linearize<Depth+1,Rank>(extents, coord);
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

template <int Rank, int Depth, typename...Iters>
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

template <typename Functor, int Rank, int Depth>
auto apply(//Wrapped<Rank,loop_type> &vec,
	   const Wrapped<Rank,loop_type> &vec0, const Wrapped<Rank,loop_type> &vec1) {
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
}
 
template <typename Functor, int Rank>
Wrapped<Rank,loop_type> apply(const Wrapped<Rank,loop_type> &vec0, 
			      const Wrapped<Rank,loop_type> &vec1) {
  return Wrapped<Rank,loop_type>(apply<Functor,Rank,0>(vec0,vec1));
}

template <int Idx, int Rank, typename Arg, typename...Args>
auto gather_origin(Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the Astart
    builder::dyn_var<loop_type> start = arg.start;
    static_assert(!std::is_same<End, decltype(start)>::value, "End only valid for the Stop parameter of Slice");
    if constexpr (Idx < Rank) {
      if constexpr (Idx == 0) {
	auto u = gather_origin<Idx+1,Rank>(args...);
	auto r = RecContainer<loop_type>(start);
	r.nested = std::move(u);
	return r;
      } else if constexpr (Idx == Rank-1) {
	// last one
	return std::make_unique<RecContainer<loop_type>>(start);
      } else {
	auto u = gather_origin<Idx+1,Rank>(args...);	
	auto r = std::make_unique<RecContainer<loop_type>>(start);
	r->nested = std::move(u);
	return r;
      }
    }
  } else {
    // the start is just the value
    builder::dyn_var<loop_type> start = arg;
    static_assert(!std::is_same<End, decltype(start)>::value, "End only valid for the Stop parameter of Slice");
    if constexpr (Idx < Rank) {
      if constexpr (Idx == 0) {
	auto u = gather_origin<Idx+1,Rank>(args...);	
	auto r = RecContainer<loop_type>(start);
	r.nested = std::move(u);
	return r;
      } else if constexpr (Idx == Rank-1) {
	// last one
	return std::make_unique<RecContainer<loop_type>>(start);
      } else {
	auto u = gather_origin<Idx+1,Rank>(args...);
	auto r = std::make_unique<RecContainer<loop_type>>(start);
	r->nested = std::move(u);
	return r;
      }
    }
  }
}


template <int Idx, int Rank, typename Arg, typename...Args>
auto gather_strides(Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the stride
    builder::dyn_var<loop_type> stride = arg.stride;
    static_assert(!std::is_same<End, decltype(stride)>::value, "End only valid for the Stop parameter of Slice");
    if constexpr (Idx < Rank) {
      if constexpr (Idx == 0) {
	auto u = gather_strides<Idx+1,Rank>(args...);
	auto r = RecContainer<loop_type>(stride);
	r.nested = std::move(u);
	return r;
      } else if constexpr (Idx == Rank-1) {
	// last one
	return std::make_unique<RecContainer<loop_type>>(stride);
      } else {
	auto u = gather_strides<Idx+1,Rank>(args...);
	auto r = std::make_unique<RecContainer<loop_type>>(stride);
	r->nested = std::move(u);
	return r;
      }
    }
  } else {
    // the stride is just 1
    builder::dyn_var<loop_type> stride = 1;
    static_assert(!std::is_same<End, decltype(stride)>::value, "End only valid for the Stop parameter of Slice");
    if constexpr (Idx < Rank) {
      if constexpr (Idx == 0) {
	auto u = gather_strides<Idx+1,Rank>(args...);
	auto r = RecContainer<loop_type>(stride);
	r.nested = std::move(u);
	return r;
      } else if constexpr (Idx == Rank-1) {
	// last one
	return std::make_unique<RecContainer<loop_type>>(stride);
      } else {
	auto u = gather_strides<Idx+1,Rank>(args...);
	auto r = std::make_unique<RecContainer<loop_type>>(stride);
	r->nested = std::move(u);
	return r;
      }
    }
  }
}

template <int Idx, int Rank, typename Arg, typename...Args>
auto gather_stops(const Wrapped<Rank,loop_type> &extents, Arg arg, Args...args) {
  if constexpr (is_slice<Arg>()) {
    // need to extract the stop
    builder::dyn_var<loop_type> stop = arg.stop;
    if constexpr (std::is_same<End, decltype(stop)>::value) {
      auto stop2 = extents.template get<Idx>();
      if constexpr (Idx < Rank) {
	if constexpr (Idx == 0) {
	  auto u = gather_stops<Idx+1,Rank>(extents, args...);
	  auto r = RecContainer<loop_type>(stop2);
	  r.nested = std::move(u);
	  return r;
	} else if constexpr (Idx == Rank-1) {
	  // last one
	  return std::make_unique<RecContainer<loop_type>>(stop2);
	} else {
	  auto u = gather_stops<Idx+1,Rank>(extents, args...);
	  auto r = std::make_unique<RecContainer<loop_type>>(stop2);
	  r->nested = std::move(u);
	  return r;
	}
      }
    } else {
      if constexpr (Idx < Rank) {
	if constexpr (Idx == 0) {
	  auto u = gather_stops<Idx+1,Rank>(extents, args...);
	  auto r = RecContainer<loop_type>(stop);
	  r.nested = std::move(u);
	  return r;
	} else if constexpr (Idx == Rank-1) {
	  // last one
	  return std::make_unique<RecContainer<loop_type>>(stop);
	} else {
	  auto u = gather_stops<Idx+1,Rank>(extents, args...);
	  auto r = std::make_unique<RecContainer<loop_type>>(stop);
	  r->nested = std::move(u);
	  return r;
	}
      }
    }
  } else {
    // the stop is just the value    
    if constexpr (Idx < Rank) {
      if constexpr (Idx == 0) {
	auto u = gather_stops<Idx+1,Rank>(extents, args...);
	auto r = RecContainer<loop_type>(arg);
	r.nested = std::move(u);
	return r;
      } else if constexpr (Idx == Rank-1) {
	// last one
	return std::make_unique<RecContainer<loop_type>>(arg);
      } else {
	auto u = gather_stops<Idx+1,Rank>(extents, args...);
	auto r = std::make_unique<RecContainer<loop_type>>(arg);
	r->nested = std::move(u);
	return r;
      }
    }
  }
}

template <int Depth, int Rank, typename Starts, typename Stops, typename Strides>
auto convert_stops_to_extents(Starts starts, Stops stops, Strides strides) {
  if constexpr (Depth < Rank) {
    builder::dyn_var<loop_type> extent = floor_func((stops.template get<Depth>() - 
						     starts.template get<Depth>() - (loop_type)1) / strides.template get<Depth>()) + (loop_type)1;
    if constexpr (Depth == 0) {
      auto u = convert_stops_to_extents<Depth+1,Rank>(starts, stops, strides);
      auto r = RecContainer<loop_type>(extent);
      r.nested = std::move(u);
      return r;
    } else if constexpr (Depth == Rank-1) {
      // last one
      return std::make_unique<RecContainer<loop_type>>(extent);
    } else {
      auto u = convert_stops_to_extents<Depth+1,Rank>(starts, stops, strides);
      auto r = std::make_unique<RecContainer<loop_type>>(extent);
      r->nested = std::move(u);
      return r;
    }
    
  }
}

template <int Rank, typename Starts, typename Stops, typename Strides>
auto convert_stops_to_extents(Starts starts, Stops stops, Strides strides) {
  return Wrapped<Rank,loop_type>(convert_stops_to_extents<0,Rank>(starts, stops, strides));
}

}
