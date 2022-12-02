// -*-c++-*-

#pragma once

#include "blocks/c_code_generator.h"
#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"
#include "arrays.h"
#include "staged_utils.h"

using namespace hmda;

// TODO other primitive types

// Only support primitive types for this
constexpr char uint8_name[] = "HeapArray<uint8_t>";
constexpr char int32_name[] = "HeapArray<int32_t>";

template <typename Elem>
struct BuildHeapName { };

template <>
struct BuildHeapName<uint8_t> {
  constexpr auto operator()() {
    return uint8_name;
  }
};

template <>
struct BuildHeapName<int32_t> {
  constexpr auto operator()() {
    return int32_name;
  }
};

template <typename Elem>
constexpr auto build_heap_name() {
  return BuildHeapName<Elem>()();
}

template <typename Elem>
using HEAP_T = typename builder::name<build_heap_name<Elem>()>;

// this wraps the heaparray from unstaged in staging so we can use it in the generated staged code
namespace builder {

#define HEAP_DYN_VAR(type)				\
  template <>						\
    struct dyn_var<type> : public dyn_var_impl<type> {	\
							\
    typedef dyn_var_impl<type> super;			\
    using super::super;					\
    using super::operator=;				\
    builder operator= (const dyn_var<type> &t) {	\
      return (*this) = (builder)t;			\
    }							\
							\
  dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}	\
  dyn_var(): dyn_var_impl<type>() {}			\
							\
    template <typename Elem>				\
      void write(dyn_var<loop_type> lidx, Elem val) {	\
      (cast)this->operator[](lidx) = val;		\
    }							\
							\
    template <typename Elem>				\
      dyn_var<Elem> read(dyn_var<loop_type> lidx) {	\
      return (cast)this->operator[](lidx);		\
    }							\
							\
  };

HEAP_DYN_VAR(HEAP_T<uint8_t>)
HEAP_DYN_VAR(HEAP_T<int32_t>)

}
