// -*-c++-*-

#pragma once

#include <sstream>
#include <vector>
#include "builder/builder.h"
#include "defs.h"
#include "staged_utils.h"

namespace shim {

enum OptType {
  LOOP_PARALLEL
};

// force it to be a header-only
struct Optimization {
  // maintains all the current optimizations requested by the user
  //  inline static std::vector<std::unique_ptr<Optimization>> opts;

  inline static std::string opt_prefix = "opt";
  
  OptType opt_type;

  explicit Optimization(OptType opt_type) : opt_type(opt_type) { }

  //  template <typename T, typename...Args>
  //  static void apply(Args...args) {
  //    Optimization::opts.emplace_back(std::make_unique<T>(args...));    
  //  }
  
};

// This is for parallelizing an actual loop (not an inline one)
// Just plain CPU parallelism
struct Parallel : Optimization {

  inline static std::string repr = "parloop";

  // number of threads. 0 means use omp default
  int nworkers;

  static void apply(int nworkers=0) {
#if ENABLE_OPTS == 1
    std::stringstream ss;
    ss << Optimization::opt_prefix << ":" << repr << ":" << nworkers;
    builder::annotate(ss.str());
#endif
    //    Optimization::apply<CPUParallel>(nworkers);
  }
  
  explicit Parallel(int nworkers) : Optimization(LOOP_PARALLEL), nworkers(nworkers) { }

};

struct Comment {
  inline static std::string repr = "comment";
  static void apply(std::string msg) {
    std::stringstream ss;
    ss << repr << ":" << msg;
    builder::annotate(ss.str());
  }
};

template <typename PermuteObj, typename...Types>
void permute_one(PermuteObj &obj, std::tuple<Types...> value) {
  static_assert(PermuteObj::Rank_T == sizeof...(Types));
  auto arr = tuple_to_arr<loop_type>(value);
  obj.permuted_indices = std::move(arr);
}

template <typename PermuteObj, typename...Types, typename...PermuteItems>
void permute(PermuteObj &&obj, std::tuple<Types...> value, PermuteItems&&...items) {
  permute_one(std::forward<PermuteObj>(obj), value);
  if constexpr (sizeof...(PermuteItems) > 0) {
    permute(std::forward<PermuteItems>(items)...);
  }
}

}
