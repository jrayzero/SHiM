// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "staged_utils.h"
#include "fwddecls.h"

using namespace cola;

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
  }

HEAP_DYN_VAR(HEAP_T<uint8_t>);
HEAP_DYN_VAR(HEAP_T<uint16_t>);
HEAP_DYN_VAR(HEAP_T<uint32_t>);
HEAP_DYN_VAR(HEAP_T<uint64_t>);
HEAP_DYN_VAR(HEAP_T<char>);
HEAP_DYN_VAR(HEAP_T<int8_t>);
HEAP_DYN_VAR(HEAP_T<int16_t>);
HEAP_DYN_VAR(HEAP_T<int32_t>);
HEAP_DYN_VAR(HEAP_T<int64_t>);
HEAP_DYN_VAR(HEAP_T<float>);
HEAP_DYN_VAR(HEAP_T<double>);

}

namespace cola {


/// 
/// Base class for defining the type of allocation to use for blocks and views
template <typename Elem, int PhysicalRank>
struct Allocation {

  virtual ~Allocation() = default;

  ///
  /// Whether this is an internally-managed heap allocation
  virtual bool is_heap_strategy() const { return false; }

  ///
  /// Whether this is an internally-managed stack allocation
  virtual bool is_stack_strategy() const { return false; }

  ///
  /// Whether this is a user-managed heap allocation
  virtual bool is_user_heap_strategy() const { return false; }  

  ///
  /// Whether this is a user-managed stack allocation
  virtual bool is_user_stack_strategy() const { return false; }

  ///
  /// Read an element from the underlying data allocation.
  virtual builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) = 0;

  ///
  /// Write an element to the underlying data allocation.
  virtual void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) = 0;

  ///
  /// Memset on the underlying data allocation.
  virtual void memset(builder::dyn_var<loop_type> sz) = 0;

  ///
  /// Get the underlying heap allocation. Only valid for internally-managed heap allocations.
  builder::dyn_var<HEAP_T<Elem>> heap();

  ///
  /// Get the underlying stack allocation. Only valid for internally-managed stack allocations.
  template <int Sz>
  builder::dyn_var<Elem[Sz]> stack();

};

#define DISPATCH_READER(dtype)						\
  template <typename Data, bool IsHeapArr>				\
  struct DispatchRead<dtype, IsHeapArr, Data> {				\
    auto operator()(builder::dyn_var<loop_type> lidx, Data data) {	\
      if constexpr (IsHeapArr) {					\
	return read_heaparr_##dtype(data,lidx);				\
      }	else {								\
	return read_##dtype(data,lidx);					\
      }									\
    }									\
  }

#define DISPATCH_WRITER(dtype)						\
  template <typename Data, bool IsHeapArr>				\
  struct DispatchWrite<dtype, IsHeapArr, Data> {			\
    void operator()(builder::dyn_var<dtype> val, builder::dyn_var<loop_type> lidx, Data data) { \
      if constexpr (IsHeapArr) {					\
	write_heaparr_##dtype(data, lidx, val);				\
      } else {								\
	write_##dtype(data, lidx, val);					\
      }									\
    }									\
}

#define DISPATCH_MULTI_READER(dtype)					\
  template <typename Data>						\
  struct DispatchMultiRead<dtype, Data> {				\
    template <int N>							\
    auto operator()(builder::dyn_var<loop_type> (&lidx)[N], Data data) { \
      return multi_read_##dtype(data,lidx);				\
    }									\
  }

#define DISPATCH_MULTI_WRITER(dtype)					\
  template <typename Data>						\
  struct DispatchMultiWrite<dtype, Data> {				\
    template <int N>							\
    void operator()(builder::dyn_var<dtype> val, builder::dyn_var<loop_type> (&lidx)[N], Data data) { \
      multi_write_##dtype(data, lidx, val);				\
    }									\
  }

#define DISPATCH_MEMSET(dtype)						\
  template <typename Data, bool IsHeapArr>				\
  struct DispatchMemset<dtype, IsHeapArr, Data> {			\
    void operator()(Data data, builder::dyn_var<dtype> val, builder::dyn_var<loop_type> sz) { \
      if constexpr (IsHeapArr) {					\
	memset_heaparr_##dtype(data, val, sz);				\
      } else {								\
	memset_##dtype(data, val, sz);					\
      }									\
    }									\
  }

// calls the appropriate heap builder function
#define DISPATCH_BUILDER(dtype)				\
  template <>						\
  struct DispatchBuildHeap<dtype> {			\
    auto operator()(builder::dyn_var<loop_type> sz) {	\
      return build_heaparr_##dtype(sz);			\
    }							\
  }

template <typename Elem, bool IsHeapArr, typename Data>
struct DispatchRead { };
DISPATCH_READER(uint8_t);
DISPATCH_READER(uint16_t);
DISPATCH_READER(uint32_t);
DISPATCH_READER(uint64_t);
DISPATCH_READER(char);
DISPATCH_READER(int8_t);
DISPATCH_READER(int16_t);
DISPATCH_READER(int32_t);
DISPATCH_READER(int64_t);
DISPATCH_READER(float);
DISPATCH_READER(double);

template <typename Elem, bool IsHeapArr, typename Data>
struct DispatchWrite { };
DISPATCH_WRITER(uint8_t);
DISPATCH_WRITER(uint16_t);
DISPATCH_WRITER(uint32_t);
DISPATCH_WRITER(uint64_t);
DISPATCH_WRITER(char);
DISPATCH_WRITER(int8_t);
DISPATCH_WRITER(int16_t);
DISPATCH_WRITER(int32_t);
DISPATCH_WRITER(int64_t);
DISPATCH_WRITER(float);
DISPATCH_WRITER(double);

template <typename Elem, typename Data>
struct DispatchMultiRead { };
DISPATCH_MULTI_READER(uint8_t);
DISPATCH_MULTI_READER(uint16_t);
DISPATCH_MULTI_READER(uint32_t);
DISPATCH_MULTI_READER(uint64_t);
DISPATCH_MULTI_READER(char);
DISPATCH_MULTI_READER(int8_t);
DISPATCH_MULTI_READER(int16_t);
DISPATCH_MULTI_READER(int32_t);
DISPATCH_MULTI_READER(int64_t);
DISPATCH_MULTI_READER(float);
DISPATCH_MULTI_READER(double);

template <typename Elem, typename Data>
struct DispatchMultiWrite { };
DISPATCH_MULTI_WRITER(uint8_t);
DISPATCH_MULTI_WRITER(uint16_t);
DISPATCH_MULTI_WRITER(uint32_t);
DISPATCH_MULTI_WRITER(uint64_t);
DISPATCH_MULTI_WRITER(char);
DISPATCH_MULTI_WRITER(int8_t);
DISPATCH_MULTI_WRITER(int16_t);
DISPATCH_MULTI_WRITER(int32_t);
DISPATCH_MULTI_WRITER(int64_t);
DISPATCH_MULTI_WRITER(float);
DISPATCH_MULTI_WRITER(double);

template <typename Elem, bool IsHeapArry, typename Data>
struct DispatchMemset { };
DISPATCH_MEMSET(uint8_t);
DISPATCH_MEMSET(uint16_t);
DISPATCH_MEMSET(uint32_t);
DISPATCH_MEMSET(uint64_t);
DISPATCH_MEMSET(char);
DISPATCH_MEMSET(int8_t);
DISPATCH_MEMSET(int16_t);
DISPATCH_MEMSET(int32_t);
DISPATCH_MEMSET(int64_t);
DISPATCH_MEMSET(float);
DISPATCH_MEMSET(double);

template <typename Elem>
struct DispatchBuildHeap { };
DISPATCH_BUILDER(uint8_t);
DISPATCH_BUILDER(uint16_t);
DISPATCH_BUILDER(uint32_t);
DISPATCH_BUILDER(uint64_t);
DISPATCH_BUILDER(char);
DISPATCH_BUILDER(int8_t);
DISPATCH_BUILDER(int16_t);
DISPATCH_BUILDER(int32_t);
DISPATCH_BUILDER(int64_t);
DISPATCH_BUILDER(float);
DISPATCH_BUILDER(double);

///
/// Calls the appropriate external read function based on the Elem and allocation type
template <typename Elem, bool IsHeapArr, typename Data>
auto dispatch_read(builder::dyn_var<loop_type> lidx, Data data) {
  return DispatchRead<Elem,IsHeapArr,Data>()(lidx, data);
}

///
/// Calls the appropriate external write function based on the Elem and allocation type
template <typename Elem, bool IsHeapArr, typename Data>
void dispatch_write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> lidx, Data data) {
  DispatchWrite<Elem,IsHeapArr,Data>()(val, lidx, data);
}

///
/// Calls the appropriate external read function based on the Elem and allocation type
template <typename Elem, typename Data, int N>
auto dispatch_multi_read(builder::dyn_var<loop_type> (&lidx)[N], Data data) {
  return DispatchMultiRead<Elem,Data>()(lidx, data);
}

///
/// Calls the appropriate external write function based on the Elem and allocation type
template <typename Elem, typename Data, int N>
void dispatch_multi_write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> (&lidx)[N], Data data) {
  DispatchMultiWrite<Elem,Data>()(val, lidx, data);
}

///
/// Calls the appropriate external write function based on the Elem and allocation type
template <typename Elem, bool IsHeapArr, typename Data>
void dispatch_memset(Data data, builder::dyn_var<Elem> val, builder::dyn_var<loop_type> sz) {
  DispatchMemset<Elem,IsHeapArr,Data>()(data, val, sz);
}

///
/// Calls the appropriate external HeapArr builder function based on the Elem and allocation type
template <typename Elem>
auto dispatch_build_heap(builder::dyn_var<loop_type> sz) {
  return DispatchBuildHeap<Elem>()(sz);  
}

///
/// Defines an internally-allocated reference-counted heap
template <typename Elem>
struct HeapAllocation : public Allocation<Elem,1> {

  virtual ~HeapAllocation() = default;

  explicit HeapAllocation(builder::dyn_var<loop_type> sz) : data(dispatch_build_heap<Elem>(sz)) { }

  bool is_heap_strategy() const override { return true; }

  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> (&idxs)[1]) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> (&idxs)[1]) override;

  void memset(builder::dyn_var<loop_type> sz) override;

  builder::dyn_var<HEAP_T<Elem>> data;
};

///
/// Defines an internally-allocated stack
template <typename Elem, int Size>
struct StackAllocation : public Allocation<Elem,1> {

  virtual ~StackAllocation() = default;

  bool is_stack_strategy() const override { return true; }

  builder::dyn_var<Elem> read(builder::dyn_var<loop_type> (&idxs)[1]) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> (&idxs)[1]) override;

  void memset(builder::dyn_var<loop_type> sz) override;

  builder::dyn_var<Elem[Size]> data;

};
                
/// Defines a user-allocated region of data
template <typename Elem, typename Storage, int PhysicalRank>
struct UserAllocation : public Allocation<Elem, PhysicalRank> {

  virtual ~UserAllocation() = default;

  explicit UserAllocation(builder::dyn_var<Storage> data) : data(data) { }

  bool is_user_heap_strategy() const override { return true; }

  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) override;

  void memset(builder::dyn_var<loop_type> sz) override;
  
  builder::dyn_var<Storage> data;

};

template <typename Elem, int PhysicalRank>
builder::dyn_var<HEAP_T<Elem>> Allocation<Elem, PhysicalRank>::heap() {
  assert(is_heap_strategy());
  return static_cast<HeapAllocation<Elem>*>(this)->data;
}
template <typename Elem, int PhysicalRank>
template <int Sz>
builder::dyn_var<Elem[Sz]> Allocation<Elem, PhysicalRank>::stack() {
  assert(is_stack_strategy());
  return static_cast<StackAllocation<Elem,Sz>*>(this)->data;
}

template <typename Elem>
builder::dyn_var<Elem> HeapAllocation<Elem>::read(builder::dyn_var<loop_type> (&idxs)[1]) { 
  return dispatch_read<Elem,true>(idxs[0], data);
}

template <typename Elem>
void HeapAllocation<Elem>::write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> (&idxs)[1]) { 
  dispatch_write<Elem,true>(val, idxs[0], data);
}

template <typename Elem>
void HeapAllocation<Elem>::memset(builder::dyn_var<loop_type> sz) {
  dispatch_memset<Elem,true>(data, Elem(0), sz);
}

template <typename Elem, int Sz>
builder::dyn_var<Elem> StackAllocation<Elem,Sz>::read(builder::dyn_var<loop_type> (&idxs)[1]) { 
  return dispatch_read<Elem,false>(idxs[0], data);
}

template <typename Elem, int Sz>
void StackAllocation<Elem,Sz>::write(builder::dyn_var<Elem> val, builder::dyn_var<loop_type> (&idxs)[1]) { 
  dispatch_write<Elem,false>(val, idxs[0], data);
}

template <typename Elem, int Sz>
void StackAllocation<Elem,Sz>::memset(builder::dyn_var<loop_type> sz) {
  dispatch_memset<Elem,false>(data, Elem(0), sz);
}  

template <int Rank, int Depth, typename Data, typename Idxs>
auto multi_read(Data &data, Idxs &idxs) {
  auto x = data[idxs[Depth]];
  if constexpr (Depth == Rank - 1) {
    return x;
  } else {
    return multi_read<Rank,Depth+1>(x, idxs);
  }
}

template <int Rank, int Depth, typename Data, typename Val, typename Idxs>
void multi_write(Data &data, Val val, Idxs &idxs) {
  if constexpr (Depth == Rank - 1) {
    data[idxs[Depth]] = val;
  } else {
    auto x = data[idxs[Depth]];
    multi_write<Rank,Depth+1>(x, val, idxs);
  }
}

template <typename Elem, typename Storage, int PhysicalRank>  
builder::dyn_var<Elem> UserAllocation<Elem,Storage,PhysicalRank>::read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) {
//  if constexpr (PhysicalRank == 1) {
//    return dispatch_read<Elem,false>(idxs[0], data);
//  } else {
//    return dispatch_multi_read<Elem>(idxs, data);
  return multi_read<PhysicalRank,0>(data, idxs);
//  return data[0][0];
//  return 10;
//  }
}

template <typename Elem, typename Storage, int PhysicalRank>
void UserAllocation<Elem,Storage,PhysicalRank>::write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) {
//  if constexpr (PhysicalRank == 1) {
//    dispatch_write<Elem,false>(val, idxs[0], data);
//  } else {
//    dispatch_multi_write<Elem>(val, idxs, data);    
  multi_write<PhysicalRank,0>(data, val, idxs);
  //  }
}

template <typename Elem, typename Storage, int PhysicalRank>
void UserAllocation<Elem,Storage,PhysicalRank>::memset(builder::dyn_var<loop_type> sz) {
  assert(false);
}

}
