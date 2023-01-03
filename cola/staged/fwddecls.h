// -*-c++-*-

#pragma once

// forward declarations for a bunch of things I need
// also includes some general type definitions

namespace cola {

///// Forward declarations

// Block-like things
template <typename Elem, int Rank>
struct Block;
template <typename Elem, int Rank>
struct View;
template <typename BlockLike, typename Idxs> 
struct Ref;

// object fields
template <typename Elem>
struct SField;

// Expressions
template <typename Derived>
struct Expr;
template <typename Functor, typename Operand0>
struct Unary;
template <typename Functor, typename Operand0, typename Operand1>
struct Binary;
template <char Ident>
struct Iter;
template <typename BlockLIke, typename Idxs>
struct Ref;
template <typename To, typename Operand>
struct TemplateCast;

// Other
template <bool B>
struct Slice;

///// Types

// Definitions for the HeapArray type
constexpr char uint8_t_name[] = "cola::HeapArray<uint8_t>";
constexpr char uint16_t_name[] = "cola::HeapArray<uint16_t>";
constexpr char uint32_t_name[] = "cola::HeapArray<uint32_t>";
constexpr char uint64_t_name[] = "cola::HeapArray<uint64_t>";
constexpr char int16_t_name[] = "cola::HeapArray<int16_t>";
constexpr char int32_t_name[] = "cola::HeapArray<int32_t>";
constexpr char int64_t_name[] = "cola::HeapArray<int64_t>";
constexpr char float_name[] = "cola::HeapArray<float>";
constexpr char double_name[] = "cola::HeapArray<double>";

#define CONSTEXPR_HEAP_NAME(dtype)		\
  template <>					\
  struct BuildHeapName<dtype> {			\
    constexpr auto operator()() {		\
      return dtype##_name;			\
    }						\
  }

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

// Based on Elem, return the appropriate constexpr char name for HeapArray
template <typename Elem>
constexpr auto build_heap_name() {
  return BuildHeapName<Elem>()();
}

// Register the name of HeapArray<Elem> in buildit
template <typename Elem>
using HEAP_T = typename builder::name<build_heap_name<Elem>()>;

/// The type for location information in blocks and views
template <int Rank>
using Loc_T = builder::dyn_var<loop_type[Rank]>;

}
