// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "staged_utils.h"
#include "fwddecls.h"

using namespace hmda;

namespace builder {

// These allow us to insert HeapArray<Elem> datatypes into the generated code
// I can't do template <typename Elem> dyn_var<HEAP_T<Elem>> b/c that would be
// an invalid partial specialization
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

namespace hmda {

// general allocators

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
	return read_heaparr_##dtype(data,lidx);			\
      }	else {								\
	return read_##dtype(data,lidx);				\
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
	write_heaparr_##dtype(data, lidx, val);			\
      } else {								\
	write_##dtype(data, lidx, val);				\
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
	memset_heaparr_##dtype(data, val, sz);			\
      } else {								\
	memset_##dtype(data, val, sz);				\
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
      return build_heaparr_##dtype(sz);		\
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

}
