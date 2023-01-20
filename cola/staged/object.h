// -*-c++-*-
//
#pragma once

#define DYN_VAR_PTR(type)				\
  template <>						\
  class dyn_var<type*> : public dyn_var_impl<type*> {	\
  public:						\
  typedef dyn_var_impl<type*> super;			\
  using super::super;					\
  using super::operator=;				\
  builder operator= (const dyn_var<type*> &t) {		\
    return (*this) = (builder)t;			\
  }							\
  dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}	\
  dyn_var(): dyn_var_impl<type*>() {}			\
  dyn_var<type> operator *() {				\
    return (cast)this->operator[](0);			\
  }							\
  dyn_var<type> _p = as_member_of(this, "_p");		\
  dyn_var<type>* operator->() {				\
    _p = (cast)this->operator[](0);			\
    return _p.addr();					\
  }							\
  };

#define DYN_VAR_BP(type)				\
  typedef dyn_var_impl<type> super;			\
  using super::super;					\
  using super::operator=;				\
  builder operator= (const dyn_var<type> &t) {		\
    return (*this) = (builder)t;			\
  }							\
  dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}	\
  dyn_var(): dyn_var_impl<type>() {}					


//
//#include <map>
//#include "staged_utils.h"
//
//namespace builder {
//
//template <typename Name>
//struct GetName { };
//
//template <const char *N>
//struct GetName<name<N>> {
//  std::string operator()() {
//    return N;
//  }
//};
//
//template <typename Name>
//struct GetName<Name*> {
//  std::string operator()() {
//    return GetName<Name>()();
//  }
//};
//
//struct StructInfo {
//  // struct name -> {field name -> field type repr}
//  inline static std::map<std::string, std::map<std::string, std::string>> structs;  
//};
//
//template <typename T>
//class cola_dyn_var_wrapper : public dyn_var_impl<T> {
//public:
//  typedef dyn_var_impl<T> super;
//  using super::super;
//  using super::operator=;
//  builder operator= (const dyn_var<T> &t) {
//    return (*this) = (builder)t;
//  }	
//  cola_dyn_var_wrapper(const dyn_var<T>& t): dyn_var_impl<T>((builder)t){}
//  cola_dyn_var_wrapper(): dyn_var_impl<T>() {
//    std::string n = GetName<T>()();
//    if (StructInfo::structs.count(n) == 0) {
//      StructInfo::structs[n] = {};
//    }
//  }
//
//  // support -> for ptr fields
//  template <typename std::enable_if<is_ptr<T>(),int>::type=0>
//  dyn_var<T> *operator->() {
//
//  }
//  
//protected:
//  
//  template <typename FieldType, typename This>
//  dyn_var<FieldType> add_field(This *this_ptr, std::string field_name) {
//    std::string n = GetName<T>()();
//    assert(StructInfo::structs.count(n) > 0);
//    dyn_var<FieldType> field = as_member_of(this_ptr, field_name);
//    std::string type = cola::ElemToStr<FieldType>::str;
//    std::string repr = type + " " + field_name;
//    if constexpr (cola::ElemToStr<FieldType>::isArr) {
//      repr += "[" + cola::ElemToStr<FieldType>::sz + "]";
//    }
//    if (StructInfo::structs[n].count(field_name) == 0) {
//      StructInfo::structs[n][field_name] = repr;
//    }
//    return field;
//  }
//
//  ///
//  /// Add a field that inherits from a dyn var (like a cola_dyn_var_wrapper)
//  /// Cannot currently print out this field to a struct definition.
//  template <typename FieldType, typename This>
//  FieldType add_rfield(This *this_ptr, std::string field_name) {
////    using T2 = decltype(typename GetCoreT<T>::Core_T);
////    std::string n = T2::str;
////    assert(StructInfo::structs.count(field_name) > 0);
//    as_member_of field(this_ptr, field_name);// = as_member_of(this_ptr, field_name);
//    FieldType f(field);
//    return f;
//    //std::string type = cola::ElemToStr<FieldType>::str;
//    //std::string repr = type + " " + field_name;
//    //if constexpr (cola::ElemToStr<FieldType>::isArr) {
//    //  repr += "[" + cola::ElemToStr<FieldType>::sz + "]";
//    //}
//    // just here to make sure that you don't re-add the same field twice
////    if (StructInfo::structs[n].count(field_name) == 0) {
////      StructInfo::structs[n][field_name] = "";
////    }
////    return field;
//  }
//
//};
//}
//
///*namespace builder {
//
//template <typename T>
//class cola_dyn_var;
//
//template <typename T, bool dummy>
//class dyn_var<T> : public dyn_var_impl<T> {
//  friend class cola_dyn_var<T>;
//private:
//  typedef dyn_var_impl<T> super;
//  using super::super;
//  using super::operator=;
//  builder operator= (const dyn_var<T> &t) {
//    return (*this) = (builder)t;
//  }	
//  dyn_var(const dyn_var& t): dyn_var_impl<T>((builder)t){}
//  dyn_var(): dyn_var_impl<T>() {
//    std::string n = GetName<T>()();
//    if (StructInfo::structs.count(n) == 0) {
//      StructInfo::structs[n] = {};
//    }
//  }
//};
//
//}*/
////namespace builder {
//
//template <typename T>
//class cola_dyn_var {
//
//  template <>
//  class builder::dyn_var<T> : public builder::dyn_var_impl<T> {
//  public:
//
//    typedef builder::dyn_var_impl<T> super;
//    using super::super;
//    using super::operator=;
//    builder::builder operator= (const builder::dyn_var<T> &t) {
//      return (*this) = (builder)t;
//    }
//    builder::dyn_var(const builder::dyn_var& t): builder::dyn_var_impl((builder::builder)t){}
//    builder::dyn_var(): builder::dyn_var_impl<T>() {}
//  
//    //  cola_dyn_var<int> member = as_member_of(this, "member");
//
//  };
//};
//
////}
//
