// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "utils.hpp"

using namespace shim;

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

namespace shim {


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
  /// Whether this is a user-managed allocation
  virtual bool is_user_strategy() const { return false; }  

  ///
  /// Whether this is a user-managed non-stadnard allocation
  virtual bool is_non_standard_strategy() const { return false; }

  ///
  /// Read an element from the underlying data allocation.
  virtual builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) = 0;

  ///
  /// Write an element to the underlying data allocation.
  virtual void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) = 0;

  ///
  /// Get the underlying heap allocation. Only valid for internally-managed heap allocations.
  builder::dyn_var<HEAP_T<Elem>> heap();

  ///
  /// Get the underlying stack allocation. Only valid for internally-managed stack allocations.
//  template <int Sz>
  builder::dyn_var<Elem*> stack();

};

// calls the appropriate heap builder function
#define DISPATCH_BUILDER(dtype)				\
  template <>						\
  struct DispatchBuildHeap<dtype> {			\
    auto operator()(builder::dyn_var<loop_type> sz) {	\
      return build_heaparr_##dtype(sz);			\
    }							\
  }

// calls the appropriate stack builder function
#define DISPATCH_STACK_BUILDER(dtype)				\
  template <>							\
  struct DispatchBuildStack<dtype> {				\
    auto operator()(loop_type sz) {				\
      return build_stackarr_##dtype(sz);			\
    }								\
  }

#define DISPATCH_READ_NON_STANDARD(dtype)			\
  template <typename Data, typename...Expanded>			\
  struct DispatchReadNonStandard<dtype, Data, Expanded...> {		\
    auto operator()(Data data, Expanded...expanded) {		\
      return read_nonstandard_##dtype(data, expanded...);	\
    }								\
  }

#define DISPATCH_WRITE_NON_STANDARD(dtype)				\
  template <typename Data, typename Val, typename...Expanded>		\
  struct DispatchWriteNonStandard<dtype, Data, Val, Expanded...> {		\
    auto operator()(Data data, Val val, Expanded...expanded) {		\
      return write_nonstandard_##dtype(data, val, expanded...);		\
    }									\
  }

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

template <typename Elem>
struct DispatchBuildStack { };
DISPATCH_STACK_BUILDER(uint8_t);
DISPATCH_STACK_BUILDER(uint16_t);
DISPATCH_STACK_BUILDER(uint32_t);
DISPATCH_STACK_BUILDER(uint64_t);
DISPATCH_STACK_BUILDER(char);
DISPATCH_STACK_BUILDER(int8_t);
DISPATCH_STACK_BUILDER(int16_t);
DISPATCH_STACK_BUILDER(int32_t);
DISPATCH_STACK_BUILDER(int64_t);
DISPATCH_STACK_BUILDER(float);
DISPATCH_STACK_BUILDER(double);

template <typename Elem, typename Data, typename...Expanded>
struct DispatchReadNonStandard { };
DISPATCH_READ_NON_STANDARD(uint8_t);
DISPATCH_READ_NON_STANDARD(uint16_t);
DISPATCH_READ_NON_STANDARD(uint32_t);
DISPATCH_READ_NON_STANDARD(uint64_t);
DISPATCH_READ_NON_STANDARD(char);
DISPATCH_READ_NON_STANDARD(int8_t);
DISPATCH_READ_NON_STANDARD(int16_t);
DISPATCH_READ_NON_STANDARD(int32_t);
DISPATCH_READ_NON_STANDARD(int64_t);
DISPATCH_READ_NON_STANDARD(float);
DISPATCH_READ_NON_STANDARD(double);

template <typename Elem, typename Data, typename Val, typename...Expanded>
struct DispatchWriteNonStandard { };
DISPATCH_WRITE_NON_STANDARD(uint8_t);
DISPATCH_WRITE_NON_STANDARD(uint16_t);
DISPATCH_WRITE_NON_STANDARD(uint32_t);
DISPATCH_WRITE_NON_STANDARD(uint64_t);
DISPATCH_WRITE_NON_STANDARD(char);
DISPATCH_WRITE_NON_STANDARD(int8_t);
DISPATCH_WRITE_NON_STANDARD(int16_t);
DISPATCH_WRITE_NON_STANDARD(int32_t);
DISPATCH_WRITE_NON_STANDARD(int64_t);
DISPATCH_WRITE_NON_STANDARD(float);
DISPATCH_WRITE_NON_STANDARD(double);

///
/// Calls the appropriate external HeapArr builder function based on the Elem and allocation type
template <typename Elem>
auto dispatch_build_heap(builder::dyn_var<loop_type> sz) {
  return DispatchBuildHeap<Elem>()(sz);  
}

///
/// Calls the appropriate external StackArr builder function based on the Elem and allocation type
template <typename Elem>
auto dispatch_build_stack(loop_type sz) {
  return DispatchBuildStack<Elem>()(sz);  
}

template <typename Elem, typename Data, typename...Expanded>
auto dispatch_read_non_standard(Data data, Expanded...expanded) {
  return DispatchReadNonStandard<Elem, Data,Expanded...>()(data, expanded...);  
}

template <typename Elem, typename Data, typename Val, typename...Expanded>
auto dispatch_write_non_standard(Data data, Val val, Expanded...expanded) {
  return DispatchWriteNonStandard<Elem, Data,Val,Expanded...>()(data, val, expanded...);  
}

///
/// Defines an internally-allocated reference-counted heap
template <typename Elem>
struct HeapAllocation : public Allocation<Elem,1> {

  virtual ~HeapAllocation() = default;

  explicit HeapAllocation(builder::dyn_var<loop_type> sz) : data(dispatch_build_heap<Elem>(sz)) { }

  bool is_heap_strategy() const override { return true; }

  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,1> &idxs) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,1> &idxs) override;

  builder::dyn_var<HEAP_T<Elem>> data;
};

///
/// Defines an internally-allocated stack
template <typename Elem>
struct StackAllocation : public Allocation<Elem,1> {

  virtual ~StackAllocation() = default;

  // For a stack allocation, this better be a constant sz. otherwise your generated code may fail
  explicit StackAllocation(builder::static_var<loop_type> sz) : data(dispatch_build_stack<Elem>(sz)) { }

  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,1> &idxs) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,1> &idxs) override;

  builder::dyn_var<Elem*> data;

};
                
/// Defines a user-allocated region of data
template <typename Elem, typename Storage, int PhysicalRank>
struct UserAllocation : public Allocation<Elem, PhysicalRank> {

  virtual ~UserAllocation() = default;

  explicit UserAllocation(builder::dyn_var<Storage> data) : data(data) { }

  bool is_user_strategy() const override { return true; }

  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) override;

  builder::dyn_var<Storage> data;

};

/// Defines a non-standard user-allocated region of data
template <typename Elem, typename Storage, int PhysicalRank>
struct NonStandardAllocation : public Allocation<Elem, PhysicalRank> {

  virtual ~NonStandardAllocation() = default;

  explicit NonStandardAllocation(builder::dyn_var<Storage> data) : data(data) { }

  bool is_non_standard_strategy() const override { return true; }

  builder::dyn_var<Elem> read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) override;

  void write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) override;

  builder::dyn_var<Storage> data;

};

template <typename Elem, int PhysicalRank>
builder::dyn_var<HEAP_T<Elem>> Allocation<Elem, PhysicalRank>::heap() {
  assert(is_heap_strategy());
  return static_cast<HeapAllocation<Elem>*>(this)->data;
}
template <typename Elem, int PhysicalRank>
builder::dyn_var<Elem*> Allocation<Elem, PhysicalRank>::stack() {
  assert(is_stack_strategy());
  return static_cast<StackAllocation<Elem>*>(this)->data;
}

template <typename Elem>
builder::dyn_var<Elem> HeapAllocation<Elem>::read(builder::dyn_arr<loop_type,1> &idxs) { 
  return data[idxs[0]];
}

template <typename Elem>
void HeapAllocation<Elem>::write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,1> &idxs) { 
  data[idxs[0]] = val;
}

template <typename Elem>
builder::dyn_var<Elem> StackAllocation<Elem>::read(builder::dyn_arr<loop_type,1> &idxs) { 
  return data[idxs[0]];
}

template <typename Elem>
void StackAllocation<Elem>::write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,1> &idxs) { 
  data[idxs[0]] = val;
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
  return multi_read<PhysicalRank,0>(data, idxs);
}

template <typename Elem, typename Storage, int PhysicalRank>
void UserAllocation<Elem,Storage,PhysicalRank>::write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) {
  multi_write<PhysicalRank,0>(data, val, idxs);
}

template <int Rank, int Depth, typename Elem, typename Data, typename Idxs, typename...Expanded>
auto non_standard_read(Data &data, Idxs &idxs, Expanded...expanded) {
  auto x = idxs[Depth];
  if constexpr (Depth == Rank - 1) {
    return dispatch_read_non_standard<Elem,Data,Expanded...>(data, expanded..., x);
  } else {
    return non_standard_read<Rank,Depth+1,Elem>(data, idxs, expanded..., x);
  }
}

template <int Rank, int Depth, typename Elem, typename Data, typename Val, typename Idxs, typename...Expanded>
void non_standard_write(Data &data, Val val, Idxs &idxs, Expanded...expanded) {
  if constexpr (Depth == Rank - 1) {
    dispatch_write_non_standard<Elem, Data, Val, Expanded...>(data, val, expanded..., idxs[Depth]);
  } else {
    auto x = idxs[Depth];
    non_standard_write<Rank,Depth+1,Elem>(data, val, idxs, expanded..., x);
  }
}

template <typename Elem, typename Storage, int PhysicalRank>  
builder::dyn_var<Elem> NonStandardAllocation<Elem,Storage,PhysicalRank>::read(builder::dyn_arr<loop_type,PhysicalRank> &idxs) {
  return non_standard_read<PhysicalRank,0,Elem>(data, idxs);
}

template <typename Elem, typename Storage, int PhysicalRank>
void NonStandardAllocation<Elem,Storage,PhysicalRank>::write(builder::dyn_var<Elem> val, builder::dyn_arr<loop_type,PhysicalRank> &idxs) {
  non_standard_write<PhysicalRank,0,Elem>(data, val, idxs);
}

}
