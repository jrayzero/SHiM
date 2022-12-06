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

// Only support primitive types for this
constexpr char uint8_t_name[] = "HeapArray<uint8_t>";
constexpr char uint16_t_name[] = "HeapArray<uint16_t>";
constexpr char uint32_t_name[] = "HeapArray<uint32_t>";
constexpr char uint64_t_name[] = "HeapArray<uint64_t>";
constexpr char int16_t_name[] = "HeapArray<int16_t>";
constexpr char int32_t_name[] = "HeapArray<int32_t>";
constexpr char int64_t_name[] = "HeapArray<int64_t>";
constexpr char float_name[] = "HeapArray<float>";
constexpr char double_name[] = "HeapArray<double>";

// constructs the appropriate HeapArray object name so it is a constexpr
#define CONSTEXPR_HEAP_NAME(dtype)		\
  template <>					\
  struct BuildHeapName<dtype> {			\
    constexpr auto operator()() {		\
      return dtype##_name;			\
    }						\
  };

template <typename Elem>
struct BuildHeapName { };
CONSTEXPR_HEAP_NAME(uint8_t);
CONSTEXPR_HEAP_NAME(uint16_t);
CONSTEXPR_HEAP_NAME(uint32_t);
CONSTEXPR_HEAP_NAME(uint64_t);
CONSTEXPR_HEAP_NAME(int16_t);
CONSTEXPR_HEAP_NAME(int32_t);
CONSTEXPR_HEAP_NAME(int64_t);
CONSTEXPR_HEAP_NAME(float);
CONSTEXPR_HEAP_NAME(double);

template <typename Elem>
constexpr auto build_heap_name() {
  return BuildHeapName<Elem>()();
}

template <typename Elem>
using HEAP_T = typename builder::name<build_heap_name<Elem>()>;

namespace builder {

// These allow us to insert HeapArray<Elem> datatypes into the generated code
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

HEAP_DYN_VAR(HEAP_T<uint8_t>);
HEAP_DYN_VAR(HEAP_T<uint16_t>);
HEAP_DYN_VAR(HEAP_T<uint32_t>);
HEAP_DYN_VAR(HEAP_T<uint64_t>);
HEAP_DYN_VAR(HEAP_T<int16_t>);
HEAP_DYN_VAR(HEAP_T<int32_t>);
HEAP_DYN_VAR(HEAP_T<int64_t>);
HEAP_DYN_VAR(HEAP_T<float>);
HEAP_DYN_VAR(HEAP_T<double>);

}

// general allocators

// these are all the function wrappers for reading/writing data
// These aren't exactly the correct signatures since dtype* should actually be heap array for some that case, but it works
// (i.e. buildit doesn't complain)
#define READER(dtype)							\
  builder::dyn_var<dtype(loop_type,dtype*)> read_func_##dtype = builder::as_global("read_" # dtype);
#define WRITER(dtype)							\
  builder::dyn_var<void(dtype,loop_type,dtype*)> write_func_##dtype = builder::as_global("write_" # dtype);
#define BUILDER(dtype)							\
  builder::dyn_var<dtype*(loop_type)> build_heap_func_##dtype = builder::as_global("build_heap_" # dtype);

READER(uint8_t);
READER(uint16_t);
READER(uint32_t);
READER(uint64_t);
READER(int16_t);
READER(int32_t);
READER(int64_t);
READER(float);
READER(double);

WRITER(uint8_t);
WRITER(uint16_t);
WRITER(uint32_t);
WRITER(uint64_t);
WRITER(int16_t);
WRITER(int32_t);
WRITER(int64_t);
WRITER(float);
WRITER(double);

BUILDER(uint8_t);
BUILDER(uint16_t);
BUILDER(uint32_t);
BUILDER(uint64_t);
BUILDER(int16_t);
BUILDER(int32_t);
BUILDER(int64_t);
BUILDER(float);
BUILDER(double);

template <typename Elem>
struct Allocation {
  // I don't think I need these is_X functions.
  virtual bool is_heap_strategy() const { return false; }
  virtual bool is_stack_strategy() const { return false; }
  virtual bool is_user_stack_strategy() const { return false; }
  virtual bool is_user_heap_strategy() const { return false; }  
  virtual builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) = 0;
  virtual void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) = 0;
  virtual void memset(builder::dyn_var<int> sz) = 0;
  virtual ~Allocation() = default;
};

// calls the appropriate read function
#define DISPATCH_READER(dtype)						\
  template <typename Data>						\
  struct DispatchRead<dtype, Data> {					\
    auto operator()(builder::dyn_var<loop_type> lidx, Data data) {	\
      return read_func_##dtype(lidx,data);				\
    }									\
  };

template <typename Elem, typename Data>
struct DispatchRead { };
DISPATCH_READER(uint8_t);
DISPATCH_READER(uint16_t);
DISPATCH_READER(uint32_t);
DISPATCH_READER(uint64_t);
DISPATCH_READER(int16_t);
DISPATCH_READER(int32_t);
DISPATCH_READER(int64_t);
DISPATCH_READER(float);
DISPATCH_READER(double);

template <typename Elem, typename Data>
auto dispatch_read(builder::dyn_var<loop_type> lidx, Data data) {
  return DispatchRead<Elem,Data>()(lidx, data);
}

// calls the appropriate write function
#define DISPATCH_WRITER(dtype)						\
  template <typename Data>						\
  struct DispatchWrite<dtype, Data> {					\
    void operator()(builder::dyn_var<dtype> val, builder::dyn_var<loop_type> lidx, Data data) { \
      write_func_##dtype(val, lidx, data);				\
    }									\
  };

template <typename Elem, typename Data>
struct DispatchWrite { };
DISPATCH_WRITER(uint8_t);
DISPATCH_WRITER(uint16_t);
DISPATCH_WRITER(uint32_t);
DISPATCH_WRITER(uint64_t);
DISPATCH_WRITER(int16_t);
DISPATCH_WRITER(int32_t);
DISPATCH_WRITER(int64_t);
DISPATCH_WRITER(float);
DISPATCH_WRITER(double);

template <typename Elem, typename Data>
void dispatch_write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx, Data data) {
  DispatchWrite<Elem,Data>()(val, lidx, data);
}

// calls the appropriate heap builder function
#define DISPATCH_BUILDER(dtype)				\
  template <>						\
  struct DispatchBuildHeap<dtype> {			\
    auto operator()(builder::dyn_var<loop_type> sz) {	\
      return build_heap_func_##dtype(sz);		\
    }							\
  };

template <typename Elem>
struct DispatchBuildHeap { };
DISPATCH_BUILDER(uint8_t);
DISPATCH_BUILDER(uint16_t);
DISPATCH_BUILDER(uint32_t);
DISPATCH_BUILDER(uint64_t);
DISPATCH_BUILDER(int16_t);
DISPATCH_BUILDER(int32_t);
DISPATCH_BUILDER(int64_t);
DISPATCH_BUILDER(float);
DISPATCH_BUILDER(double);

template <typename Elem>
auto dispatch_build_heap(builder::dyn_var<loop_type> sz) {
  return DispatchBuildHeap<Elem>()(sz);  
}

// This is a reference-counted array
template <typename Elem>
struct HeapAllocation : public Allocation<Elem> {
  builder::dyn_var<HEAP_T<Elem>> data;
  explicit HeapAllocation(builder::dyn_var<loop_type> sz) : data(dispatch_build_heap<Elem>(sz)) { }
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
  virtual ~HeapAllocation() = default;
};

template <typename Elem, int Size>
struct StackAllocation : public Allocation<Elem> {
  builder::dyn_var<Elem[Size]> data;
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
  virtual ~StackAllocation() = default;
};

template <typename Elem>
struct UserStackAllocation : public Allocation<Elem> {
  builder::dyn_var<Elem[]> data;
  explicit UserStackAllocation(builder::dyn_var<Elem[]> data) : data(data) { }
  bool is_user_stack_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem>(val, lidx, data);
  }
  void memset(builder::dyn_var<int> sz) override {
    memset_func(this->data, 0, sz);
  }  
  virtual ~UserStackAllocation() = default;
};

template <typename Elem>
struct UserHeapAllocation : public Allocation<Elem> {
  builder::dyn_var<Elem*> data;
  explicit UserHeapAllocation(builder::dyn_var<Elem*> data) : data(data) { }
  bool is_user_heap_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem>(val, lidx, data);
  }
  void memset(builder::dyn_var<int> sz) override {
    memset_func(this->data, 0, sz);
  }  
  virtual ~UserHeapAllocation() = default;
};

