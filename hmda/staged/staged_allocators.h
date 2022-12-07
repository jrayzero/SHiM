// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "staged_utils.h"

using namespace hmda;

// Only support primitive types for this
constexpr char uint8_t_name[] = "hmda::HeapArray<uint8_t>";
constexpr char uint16_t_name[] = "hmda::HeapArray<uint16_t>";
constexpr char uint32_t_name[] = "hmda::HeapArray<uint32_t>";
constexpr char uint64_t_name[] = "hmda::HeapArray<uint64_t>";
constexpr char int16_t_name[] = "hmda::HeapArray<int16_t>";
constexpr char int32_t_name[] = "hmda::HeapArray<int32_t>";
constexpr char int64_t_name[] = "hmda::HeapArray<int64_t>";
constexpr char float_name[] = "hmda::HeapArray<float>";
constexpr char double_name[] = "hmda::HeapArray<double>";

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

// TODO for these, I can just have the string name be hmda::read<dtype> and then only need one HMDA function
// these are all the function wrappers for reading/writing data
// These aren't exactly the correct signatures since dtype* should actually be heap array for some that case, but it works
// (i.e. buildit doesn't complain). I have multiple versions so I can specify the element type for the signature
#define READER(dtype)							\
  builder::dyn_var<dtype(dtype*,loop_type)> read_func_##dtype = builder::as_global("hmda::read_" # dtype);
#define WRITER(dtype)							\
  builder::dyn_var<void(dtype*,loop_type,dtype)> write_func_##dtype = builder::as_global("hmda::write_" # dtype);
#define HREADER(dtype)							\
  builder::dyn_var<dtype(HEAP_T<dtype>,loop_type)> read_heaparr_func_##dtype = builder::as_global("hmda::read_heaparr_" # dtype);
#define HWRITER(dtype)							\
  builder::dyn_var<void(HEAP_T<dtype>,loop_type,dtype)> write_heaparr_func_##dtype = builder::as_global("hmda::write_heaparr_" # dtype);
#define BUILDER(dtype)							\
  builder::dyn_var<HEAP_T<dtype>(loop_type)> build_heaparr_func_##dtype = builder::as_global("hmda::build_heaparr_" # dtype);
#define MEMSET(dtype) \
  builder::dyn_var<void(dtype*,dtype,loop_type)> memset_func_##dtype = builder::as_global("hmda::memset_" # dtype);
#define HMEMSET(dtype) \
  builder::dyn_var<void(HEAP_T<dtype>,dtype,loop_type)> memset_heaparr_func_##dtype = builder::as_global("hmda::memset_heaparr_" # dtype);

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

HREADER(uint8_t);
HREADER(uint16_t);
HREADER(uint32_t);
HREADER(uint64_t);
HREADER(int16_t);
HREADER(int32_t);
HREADER(int64_t);
HREADER(float);
HREADER(double);

HWRITER(uint8_t);
HWRITER(uint16_t);
HWRITER(uint32_t);
HWRITER(uint64_t);
HWRITER(int16_t);
HWRITER(int32_t);
HWRITER(int64_t);
HWRITER(float);
HWRITER(double);

BUILDER(uint8_t);
BUILDER(uint16_t);
BUILDER(uint32_t);
BUILDER(uint64_t);
BUILDER(int16_t);
BUILDER(int32_t);
BUILDER(int64_t);
BUILDER(float);
BUILDER(double);

MEMSET(uint8_t);
MEMSET(uint16_t);
MEMSET(uint32_t);
MEMSET(uint64_t);
MEMSET(int16_t);
MEMSET(int32_t);
MEMSET(int64_t);
MEMSET(float);
MEMSET(double);

HMEMSET(uint8_t);
HMEMSET(uint16_t);
HMEMSET(uint32_t);
HMEMSET(uint64_t);
HMEMSET(int16_t);
HMEMSET(int32_t);
HMEMSET(int64_t);
HMEMSET(float);
HMEMSET(double);

template <typename Elem>
struct Allocation {
  // I don't think I need these is_X functions.
  virtual bool is_heap_strategy() const { return false; }
  virtual bool is_stack_strategy() const { return false; }
  virtual bool is_user_stack_strategy() const { return false; }
  virtual bool is_user_heap_strategy() const { return false; }  
  virtual builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) = 0;
  virtual void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) = 0;
  virtual void memset(builder::dyn_var<loop_type> sz) = 0;
  // for automatic allocations, we can access the underlying data. For 
  // user allocations, just use the data you passed in 
  builder::dyn_var<HEAP_T<Elem>> heap();
  template <int Sz>
  builder::dyn_var<Elem[Sz]> stack();
  virtual ~Allocation() = default;
};

// calls the appropriate read function
#define DISPATCH_READER(dtype)						\
  template <typename Data, bool IsHeapArr>				\
  struct DispatchRead<dtype, IsHeapArr, Data> {				\
    auto operator()(builder::dyn_var<loop_type> lidx, Data data) {	\
      if constexpr (IsHeapArr) {					\
	return read_heaparr_func_##dtype(data,lidx);			\
      }	else {								\
	return read_func_##dtype(data,lidx);				\
      }									\
    }									\
  };

template <typename Elem, bool IsHeapArr, typename Data>
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

template <typename Elem, bool IsHeapArr, typename Data>
auto dispatch_read(builder::dyn_var<loop_type> lidx, Data data) {
  return DispatchRead<Elem,IsHeapArr,Data>()(lidx, data);
}

// calls the appropriate write function
#define DISPATCH_WRITER(dtype)						\
  template <typename Data, bool IsHeapArr>				\
  struct DispatchWrite<dtype, IsHeapArr, Data> {			\
    void operator()(builder::dyn_var<dtype> val, builder::dyn_var<loop_type> lidx, Data data) { \
      if constexpr (IsHeapArr) {					\
	write_heaparr_func_##dtype(data, lidx, val);			\
      } else {								\
	write_func_##dtype(data, lidx, val);				\
      }									\
    }									\
  };

template <typename Elem, bool IsHeapArr, typename Data>
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

template <typename Elem, bool IsHeapArr, typename Data>
void dispatch_write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx, Data data) {
  DispatchWrite<Elem,IsHeapArr,Data>()(val, lidx, data);
}

// calls the appropriate memset function
#define DISPATCH_MEMSET(dtype)						\
  template <typename Data, bool IsHeapArr>				\
  struct DispatchMemset<dtype, IsHeapArr, Data> {			\
    void operator()(Data data, builder::dyn_var<dtype> val, builder::dyn_var<loop_type> sz) { \
      if constexpr (IsHeapArr) {					\
	memset_heaparr_func_##dtype(data, val, sz);			\
      } else {								\
	memset_func_##dtype(data, val, sz);				\
      }									\
    }									\
  };

template <typename Elem, bool IsHeapArry, typename Data>
struct DispatchMemset { };
DISPATCH_MEMSET(uint8_t);
DISPATCH_MEMSET(uint16_t);
DISPATCH_MEMSET(uint32_t);
DISPATCH_MEMSET(uint64_t);
DISPATCH_MEMSET(int16_t);
DISPATCH_MEMSET(int32_t);
DISPATCH_MEMSET(int64_t);
DISPATCH_MEMSET(float);
DISPATCH_MEMSET(double);

template <typename Elem, bool IsHeapArr, typename Data>
void dispatch_memset(Data data, builder::dyn_var<Elem> val, builder::dyn_var<loop_type> sz) {
  DispatchMemset<Elem,IsHeapArr,Data>()(data, val, sz);
}

// calls the appropriate heap builder function
#define DISPATCH_BUILDER(dtype)				\
  template <>						\
  struct DispatchBuildHeap<dtype> {			\
    auto operator()(builder::dyn_var<loop_type> sz) {	\
      return build_heaparr_func_##dtype(sz);		\
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
    return dispatch_read<Elem,true>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem,true>(val, lidx, data);
  }
  void memset(builder::dyn_var<loop_type> sz) override {
    dispatch_memset<Elem,true>(data, Elem(0), sz);
  }
  virtual ~HeapAllocation() = default;
};

template <typename Elem, int Size>
struct StackAllocation : public Allocation<Elem> {
  builder::dyn_var<Elem[Size]> data;
  bool is_stack_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem,false>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem,false>(val, lidx, data);
  }
  void memset(builder::dyn_var<loop_type> sz) override {
    dispatch_memset<Elem,false>(data, Elem(0), sz);
  }  
  virtual ~StackAllocation() = default;
};

template <typename Elem>
struct UserStackAllocation : public Allocation<Elem> {
  builder::dyn_var<Elem[]> data;
  explicit UserStackAllocation(builder::dyn_var<Elem[]> data) : data(data) { }
  bool is_user_stack_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem,false>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem,false>(val, lidx, data);
  }
  void memset(builder::dyn_var<loop_type> sz) override {
    dispatch_memset<Elem,false>(data, Elem(0), sz);
  }  
  virtual ~UserStackAllocation() = default;
};

template <typename Elem>
struct UserHeapAllocation : public Allocation<Elem> {
  builder::dyn_var<Elem*> data;
  explicit UserHeapAllocation(builder::dyn_var<Elem*> data) : data(data) { }
  bool is_user_heap_strategy() const override { return true; }
  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> lidx) override { 
    return dispatch_read<Elem,false>(lidx, data);
  }
  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx) override { 
    dispatch_write<Elem,false>(val, lidx, data);
  }
  void memset(builder::dyn_var<loop_type> sz) override {
    dispatch_memset<Elem,false>(data, Elem(0), sz);
  }  
  virtual ~UserHeapAllocation() = default;
};

template <typename Elem>
builder::dyn_var<HEAP_T<Elem>> Allocation<Elem>::heap() {
    assert(is_heap_strategy());
    return static_cast<HeapAllocation<Elem>*>(this)->data;
}
template <typename Elem>
template <int Sz>
builder::dyn_var<Elem[Sz]> Allocation<Elem>::stack() {
  assert(is_stack_strategy());
  return static_cast<StackAllocation<Elem,Sz>*>(this)->data;
}
