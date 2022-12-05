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

// Things for heap allocation

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

#define HEAP_DYN_VAR(type)					\
  template <>							\
  struct dyn_var<type> : public dyn_var_impl<type> {		\
								\
    typedef dyn_var_impl<type> super;				\
    using super::super;						\
    using super::operator=;					\
    builder operator= (const dyn_var<type> &t) {		\
      return (*this) = (builder)t;				\
    }								\
								\
    dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}	\
    dyn_var(): dyn_var_impl<type>() {}				\
								\
    dyn_var<type> read(dyn_var<loop_type> lidx) {		\
      return (cast)this->operator[](lidx);			\
    }								\
								\
  };

HEAP_DYN_VAR(HEAP_T<uint8_t>)
HEAP_DYN_VAR(HEAP_T<int32_t>)

}

// general allocators

// these are all the function wrappers for reading/writing data b/c it's difficult to operate
// directly on the dyn_vars

// These aren't exactly the correct signatures since dtype* should actually be heap array for some that case, but it works
// (i.e. buildit doesn't complain)
#define READER(dtype)							\
  builder::dyn_var<dtype(loop_type,dtype*)> read_func_##dtype = builder::as_global("read_" # dtype);
#define WRITER(dtype)							\
  builder::dyn_var<void(dtype,loop_type,dtype*)> write_func_##dtype = builder::as_global("write_" # dtype);

READER(int);
WRITER(int);

template <typename Elem>
struct Allocator {
  virtual bool is_heap_strategy() const { return false; }
  virtual bool is_stack_strategy() const { return false; }
  virtual bool is_user_strategy() const { return false; }
  // this really returns a reference 
  virtual builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) = 0;
  virtual void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) = 0;
  virtual void memset(builder::dyn_var<int> sz) = 0;
  virtual ~Allocator() = default;
};

template <typename Elem, typename Data>
struct DispatchRead {  };

template <typename Data>
struct DispatchRead<int, Data> {
  auto operator()(builder::dyn_var<loop_type> lidx, Data data) {
    return read_func_int(lidx,data); 
  }
};

template <typename Elem, typename Data>
auto dispatch_read(builder::dyn_var<loop_type> lidx, Data data) {
  return DispatchRead<Elem,Data>()(lidx, data);
}

template <typename Elem, typename Data>
struct DispatchWrite {  };

template <typename Data>
struct DispatchWrite<int, Data> {
  void operator()(builder::dyn_var<int> val, builder::dyn_var<loop_type> lidx, Data data) { 
    write_func_int(val, lidx, data);
  }
};

template <typename Elem, typename Data>
void dispatch_write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx, Data data) {
  DispatchWrite<Elem,Data>()(val, lidx, data);
}


// This is a reference-counted array
template <typename Elem>
struct HeapAllocator : public Allocator<Elem> {
  builder::dyn_var<HEAP_T<Elem>> data;
  // If I use dyn_var instead of builder, it doesn't generate heapallocator
  explicit HeapAllocator(builder::builder sz) : data(sz) { }
  bool is_heap_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem>(val, lidx, data);
  }
  void memset(builder::dyn_var<int> sz) override {
    // TODO need to actually call a special memset for heap array
    memset_heaparr_func(this->data, 0, sz);
  }
  virtual ~HeapAllocator() = default;
};

template <typename Elem>
struct StackAllocator : public Allocator<Elem> {
  builder::dyn_var<Elem[]> data;
  // If I use dyn_var instead of builder, it doesn't generate heapallocator
  explicit StackAllocator(builder::builder sz) : data(sz) { }
  bool is_stack_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem>(val, lidx, data);
  }
  void memset(builder::dyn_var<int> sz) override {
    memset_func(this->data, 0, sz);
  }  
  virtual ~StackAllocator() = default;
};

template <typename Elem>
struct UserAllocator : public Allocator<Elem> {
  builder::dyn_var<Elem*> data;
  explicit UserAllocator(builder::dyn_var<Elem*> data) : data(data) { }
  bool is_user_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem>(val, lidx, data);
  }
  void memset(builder::dyn_var<int> sz) override {
    memset_func(this->data, 0, sz);
  }  
  virtual ~UserAllocator() = default;
};

