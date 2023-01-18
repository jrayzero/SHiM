// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "builder/static_var.h"
#include "common/loop_type.h"
#include "fwrappers.h"
#include "fwddecls.h"
#include "traits.h"

namespace cola {

///
/// Convert a coordinate to a linear index 
template <int Depth, unsigned long Rank>
builder::dyn_var<loop_type> linearize(const Loc_T<Rank> &extents, builder::dyn_arr<loop_type,Rank> &coord) {
  builder::dyn_var<loop_type> c = coord[Rank-1-Depth];
  if constexpr (Depth == Rank - 1) {
    return c;
  } else {
    return c + extents[Rank-1-Depth] * linearize<Depth+1,Rank>(extents, coord);
  }
}

///
/// Perform a reduction across a region of obj
template <typename Functor, int Begin, int End, int Depth, typename Obj>
auto reduce_region_inner(Obj &obj) {
  if constexpr (Depth >= Begin && Depth < End) {
    builder::dyn_var<loop_type> item = obj[Depth];
    if constexpr (Depth < End - 1) {
      return Functor()(item, reduce_region_inner<Functor,Begin,End,Depth+1>(obj));
    } else {
      return item;
    }
  } else {
    // won't be hit if Depth >= End
    return reduce_region_inner<Functor,Begin,End,Depth+1>(obj);
  }
}

///
/// Perform a reduction across a region of obj
template <typename Functor, int Begin, int End, unsigned long Rank, typename Obj>
auto reduce_region(Obj &obj) {
  static_assert(Begin < Rank && End <= Rank && Begin < End);
  return reduce_region_inner<Functor,Begin,End,0>(obj);
}

///
/// Perform a reduction across a dyn_var<T[]>
template <typename Functor, unsigned long Rank, typename T>
builder::dyn_var<T> reduce(builder::dyn_arr<T,Rank> &arr) {
  builder::dyn_var<T> acc = arr[0];
  for (builder::static_var<T> i = 1; i < Rank; i=i+1) {
    acc = Functor()(acc, arr[i]);
  }
  return acc;
}

///
/// Perform a multiplication reduction across template values
template <loop_type Val, loop_type...Vals>
constexpr loop_type mul_reduce() {
  if constexpr (sizeof...(Vals) == 0) {
    return Val;
  } else {
    return Val * mul_reduce<Vals...>();
  }
}

///
/// Convert a linear index to a coordinate
template <int Depth, unsigned long Rank, typename LIdx>
void delinearize(Loc_T<Rank> &out, LIdx lidx, const Loc_T<Rank> &extents) {
  if constexpr (Depth+1 == Rank) {
    out[Depth] = lidx;
  } else {
    builder::dyn_var<loop_type> m = reduce_region<MulFunctor, Depth+1, Rank, Rank>(extents);
    builder::dyn_var<loop_type> c = lidx / m;
    out[Depth] = c;
    delinearize<Depth+1, Rank>(out, lidx % m, extents);    
  }
}

///
/// Apply Functor to the elements in arr0 and arr1 and store the result in arr
template <typename Functor, unsigned long Rank, int Depth=0>
void apply(Loc_T<Rank> &arr,
	   const Loc_T<Rank> &arr0, 
	   const Loc_T<Rank> &arr1) {
  auto applied = Functor()(arr0[Depth], arr1[Depth]);
  arr[Depth] = applied;
  if constexpr (Depth < Rank-1) {
    apply<Functor,Rank,Depth+1>(arr,arr0, arr1);
  }
}

#define DISPATCH_PRINT_ELEM(dtype, formatter)				\
  template <>							\
  struct DispatchPrintElem<dtype> {				\
    template <typename Val>					\
    void operator()(Val val) { print(formatter, val); }	\
  }

template <typename T>
struct DispatchPrintElem { };
DISPATCH_PRINT_ELEM(uint8_t, "%u ");
DISPATCH_PRINT_ELEM(uint16_t, "%u ");
DISPATCH_PRINT_ELEM(uint32_t, "%u ");
DISPATCH_PRINT_ELEM(uint64_t, "%u ");
DISPATCH_PRINT_ELEM(char, "%c ");
DISPATCH_PRINT_ELEM(int8_t, "%d ");
DISPATCH_PRINT_ELEM(int16_t, "%d ");
DISPATCH_PRINT_ELEM(int32_t, "%d ");
DISPATCH_PRINT_ELEM(int64_t, "%d ");
DISPATCH_PRINT_ELEM(float, "%f ");
DISPATCH_PRINT_ELEM(double, "%f ");

///
/// Call the appropriate print_elem function based on Elem
template <typename Elem, typename Val>
void dispatch_print_elem(Val val) {
  DispatchPrintElem<Elem>()(val);
}

///
/// Create the type that resulting from concatenting Idx to tuple<Idxs...>
template <typename A, typename B>
struct TupleTypeCat { };

template <typename Idx, typename...Idxs>
struct TupleTypeCat<Idx, std::tuple<Idxs...>> {
  using type = std::tuple<Idxs...,Idx>;
};

// Not sure why I even use this...
template <typename Idx>
struct RefIdxType { 
  using type = Idx;
};

template <>
struct RefIdxType<builder::builder> {
  using type = builder::dyn_var<loop_type>;
};

template <>
struct RefIdxType<builder::dyn_var<loop_type>> {
  using type = builder::dyn_var<loop_type>;
};

///
/// Wrap Elem with a pointer
template <typename Elem>
struct PtrWrap {
  using P = Elem*;
};

/// 
/// Wrap Elem as Elem* if !MultiDimPtr. Otherwise wrap it as Elem***... where there
/// are Rank-levels of pointer depth.
template <typename Elem, unsigned long Rank, bool MultiDimPtr, int Depth=0>
auto ptr_wrap() {  
  if constexpr (MultiDimPtr == true) {
    if constexpr (Depth == Rank-1) {
      return PtrWrap<Elem>();
    } else {
      return PtrWrap<typename decltype(ptr_wrap<Elem,Rank,MultiDimPtr,Depth+1>())::P>();
    }
  } else {
    return PtrWrap<Elem>(); // Becomes Elem*
  }
}

///
/// Return the physical layout of the underlying data based on whether MultiDimPtr==true 
/// (return Rank) or false (return 1)
template <unsigned long Rank, bool MultiDimPtr>
constexpr int physical() {
  if constexpr (MultiDimPtr) {
    return Rank;
  } else {
    return 1;
  }
}

/// 
/// Count the number of pointer levels on Elem
template <typename Elem>
struct Peel { constexpr int operator()() { return 0; } };
template <typename Elem>
struct Peel<Elem*> { constexpr int operator()() { return 1 + Peel<Elem>()(); } };

template <typename Elem>
constexpr int peel() {
  return Peel<Elem>()();
}

template <typename Elem>
constexpr bool is_ptr() {
  return peel<Elem>() > 0;
}

///
/// Unspecialized template to determine the core type of a compound expression
template <typename T>
struct ElemToStr { };

///
/// Core type is a builder name
template <const char* Name>
struct ElemToStr<builder::name<Name>> { 
  inline static std::string str = Name; 
  static constexpr bool isArr = false;
};

///
/// Core type is bool
template <>
struct ElemToStr<bool> { 
  inline static std::string str = "bool"; 
  static constexpr bool isArr = false;
};

///
/// Core type is uint8_t
template <>
struct ElemToStr<uint8_t> { 
  inline static std::string str = "uint8_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is uint16_t
template <>
struct ElemToStr<uint16_t> { 
  inline static std::string str = "uint16_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is uint32_t
template <>
struct ElemToStr<uint32_t> { 
  inline static std::string str = "uint32_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is uint64_t
template <>
struct ElemToStr<uint64_t> { 
  inline static std::string str = "uint64_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is char
template <>
struct ElemToStr<char> { 
  inline static std::string str = "char"; 
  static constexpr bool isArr = false;
};

///
/// Core type is int8_t (signed char)
template <>
struct ElemToStr<int8_t> { 
  inline static std::string str = "int8_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is int16_t
template <>
struct ElemToStr<int16_t> { 
  inline static std::string str = "int16_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is int32_t
template <>
struct ElemToStr<int32_t> { 
  inline static std::string str = "int32_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is int64_t
template <>
struct ElemToStr<int64_t> { 
  inline static std::string str = "int64_t"; 
  static constexpr bool isArr = false;
};

///
/// Core type is float
template <>
struct ElemToStr<float> { 
  inline static std::string str = "float"; 
  static constexpr bool isArr = false;
};

///
/// Core type is double
template <>
struct ElemToStr<double> { 
  inline static std::string str = "double"; 
  static constexpr bool isArr = false;
};

///
/// Core type is core type of T
template <typename T>
struct ElemToStr<T*> { 
  inline static std::string str = ElemToStr<T>::str + "*";  
  static constexpr bool isArr = false;
};

///
/// Core type is core type of T
template <typename T>
struct ElemToStr<T[]> { 
  inline static std::string str = ElemToStr<T>::str; 
  inline static std::string sz = "";
  static constexpr bool isArr = true;
};

///
/// Core type is core type of T
template <typename T, int Sz>
struct ElemToStr<T[Sz]> { 
  inline static std::string str = ElemToStr<T>::str;
  inline static std::string sz = std::to_string(Sz);
  static constexpr bool isArr = true;
};

}
