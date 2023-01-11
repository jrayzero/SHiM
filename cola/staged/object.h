// -*-c++-*-

#pragma once

#include <map>

namespace cola {

///
/// Unspecialized template to determine the core type of a compound expression
template <typename T>
struct ElemToStr { };

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

namespace builder {

template <typename Name>
struct GetName { };

template <const char *N>
struct GetName<name<N>> {
  std::string operator()() {
    return N;
  }
};

struct StructInfo {
  // struct name -> {field name -> field type repr}
  inline static std::map<std::string, std::map<std::string, std::string>> structs;  
};

template <typename T>
class cola_dyn_var_wrapper : public dyn_var_impl<T> {
public:
  typedef dyn_var_impl<T> super;
  using super::super;
  using super::operator=;
  builder operator= (const dyn_var<T> &t) {
    return (*this) = (builder)t;
  }	
//  cola_dyn_var_wrapper(const cola_dyn_var_wrapper& t): dyn_var_impl((builder)t){}
  cola_dyn_var_wrapper(): dyn_var_impl<T>() {
    std::string n = GetName<T>()();
    if (StructInfo::structs.count(n) == 0) {
      StructInfo::structs[n] = {};
    }
  }
  
protected:
  
  template <typename FieldType, typename This>
  dyn_var<FieldType> add_field(This *this_ptr, std::string field_name) {
    std::string n = GetName<T>()();
    assert(StructInfo::structs.count(n) > 0);
    dyn_var<FieldType> field = as_member_of(this_ptr, field_name);
    std::string type = cola::ElemToStr<FieldType>::str;
    std::string repr = type + " " + field_name;
    if constexpr (cola::ElemToStr<FieldType>::isArr) {
      repr += "[" + cola::ElemToStr<FieldType>::sz + "]";
    }
    if (StructInfo::structs[n].count(field_name) == 0) {
      StructInfo::structs[n][field_name] = repr;
    }
    return field;
  }

};

}
