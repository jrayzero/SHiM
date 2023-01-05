// -*-c++-*-

#pragma once

#include <iostream>
#include <map>
#include "builder/dyn_var.h"

namespace cola {

// TODO I don't know if this will interact nicely with the inline indexing because that creates a different type of Expr (and
// utilizes different free operator functions). However, these in here shoudl operate just like plain old dyn_vars, so 
// maybe it will just work?

/*template <typename Elem>
std::string elem_to_str() {
  if constexpr (std::is_same<bool,Elem>::value) {
    return "bool";
  } else if constexpr (std::is_same<uint8_t,Elem>::value) {
    return "uint8_t";
  } else if constexpr (std::is_same<uint16_t,Elem>::value) {
    return "uint16_t";
  } else if constexpr (std::is_same<uint32_t,Elem>::value) {
    return "uint32_t";
  } else if constexpr (std::is_same<uint64_t,Elem>::value) {
    return "uint64_t";
  } else if constexpr (std::is_same<char,Elem>::value) {
    return "char";
  } else if constexpr (std::is_same<int8_t,Elem>::value) {
    return "int8_t";
  } else if constexpr (std::is_same<int16_t,Elem>::value) {
    return "int16_t";
  } else if constexpr (std::is_same<int32_t,Elem>::value) {
    return "int32_t";
  } else if constexpr (std::is_same<int64_t,Elem>::value) {
    return "int64_t";
  } else if constexpr (std::is_same<float,Elem>::value) {
    return "float";
  } else if constexpr (std::is_same<double,Elem>::value) {
    return "double";
  } else {
    // this better be a user type
    return Elem::type_to_str();
  }
}*/

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


// dummy decl for finding annotation
template <bool Dummy=false>
void dummy_decl(std::string annotation) {
  builder::annotate(annotation);
  builder::dyn_var<int> dummy = 57;
}

struct StagedObject {

  // I tried automatically generating the instance name, but the issue was that it would get out
  // of sync with everything since buildit needs to execute the code something in multiple places
  // (this maybe where buildit's novel static tag idea helps in buildit itself). But I don't want to
  // deal with that, so I just make the user specify the name
  explicit StagedObject(std::string name, std::string instance_name) : 
    name(name), instance_name(instance_name) { 
    object_fields.push({});
    dummy_decl(build_staged_object_repr + ":" + name + ":" + instance_name);
  }

  // delete all of these to make my life easier when tracking these things
  StagedObject(const StagedObject&) = delete;
  StagedObject(StagedObject&&) = delete;
  StagedObject& operator=(const StagedObject&) = delete;
  StagedObject& operator=(StagedObject&&) = delete;
  
  virtual ~StagedObject() {
    if (collection.count(name) == 0) {
      assert(!object_fields.empty());
      collection[name] = object_fields.top();
    }
    object_fields.pop();
  }

  // struct name -> {field name,field type}
  static inline std::map<std::string, std::vector<std::pair<std::string,std::string>>> collection;

  // the set of fields for the current object being created (it's a stack
  // since you can have nested objects)
  static inline std::stack<std::vector<std::pair<std::string,std::string>>> object_fields;

  static inline const std::string build_staged_object_repr = "build_staged_object";

  static inline int next_instance = 0;

  // name of the struct object
  std::string name;

  // name of the particular instance
  std::string instance_name;
  
  int next_field = 0;

private:

  // whether the user called do_register
  bool registered;

};

// Just a holder for some static things so don't need a template parameter
// when accessing the static things in a field
struct BareSField {
  static inline const std::string repr_read = "sfield_read";
  static inline const std::string repr_write = "sfield_write";
};

// for an operation between an SField and a primitive (or dyn_var), return a dummy expression
// that can be used for resolving the type based on cpp automatictype conversions
template <typename Elem, typename Rhs>
auto determine_ret_type() {
  if constexpr (std::is_arithmetic<Rhs>()) {
    if constexpr (std::is_arithmetic<Elem>()) {
      return (Elem)0 + (Rhs)0;
    } else {
      return (typename GetCoreT<Elem>::Core_T)0 + (Rhs)0;
    }
  } else { 
    if constexpr (std::is_arithmetic<Elem>()) {
      return (Elem)0 + (typename GetCoreT<Elem>::Core_T)0;
    } else {
      return (typename GetCoreT<Elem>::Core_T)0 + (typename GetCoreT<Elem>::Core_T)0;
    }
  }
}

// If you don't care about the name (like, you're not gonna use this outside of the staged code), 
// then just create a dummy name for each
/// This is the primary template for supporting fields that are c++ arithmetic types
template <typename Elem, typename IsPrimitive=void>
struct SField {

  explicit SField(StagedObject *container, std::string name="", 
		  // in the event that elem is Elem* or Elem[], this will create a dummy dyn_var<Elem>=0 entry
		  builder::dyn_var<typename GetCoreT<Elem>::Core_T> def_val=0) : 
    name(name.empty() ? "__field" + std::to_string(container->next_field++) : name),
    object_name(container->name), instance_name(container->instance_name) {
    static_assert(std::is_arithmetic<typename GetCoreT<Elem>::Core_T>());
    assert(!StagedObject::object_fields.empty());
    auto top = StagedObject::object_fields.top();
    for (auto info : top) {
      if (info.first == this->name) {
	std::cerr << "Duplicate field " << this->name << " for user-defined StagedObject " << object_name << std::endl;
	exit(48);
      }
    }
//    if constexpr (std::is_arithmetic<typename GetCoreT<Elem>::Core_T>()) {
      if constexpr (ElemToStr<Elem>::isArr) {
	StagedObject::object_fields.top().emplace_back(std::pair<std::string,std::string>{this->name + "[" + ElemToStr<Elem>::sz + "]", 
											  ElemToStr<Elem>::str});
      } else {
	StagedObject::object_fields.top().emplace_back(std::pair<std::string,std::string>{this->name, ElemToStr<Elem>::str});
      }
      if constexpr (std::is_arithmetic<Elem>()) {
	// scalar, non user
	this->operator=(def_val);
      } // else it's an array or pointer thing. don't initialize
/*    } else {
      static_assert(std::is_same<void,typename GetCoreT<Elem>::Core_T>()>());
      // okay, this must be a user type because the core type was void
      
    }*/

  }

  ///
  /// Perform a write to this field.
  // TODO this fails when write to field from other field!
  void operator=(builder::dyn_var<Elem> rhs) {
    dummy_decl(BareSField::repr_write + ":" + name + ":" + instance_name);
    this->value = rhs;
  }

  ///
  /// Perform an access to this field
  // TODO need to check underlying field is an array or a pointer
  // TODO move impl out of class
//  void operator[](builder::dyn_var<loop_type> idx) {
//    dummy_decl(BareSFIeld);
//  }

  /// 
  /// Conversion to the underlying dyn_var
  operator builder::dyn_var<Elem>() {
    assert(false);
    dummy_decl(BareSField::repr_read + ":" + name + ":" + instance_name);
    return value;
  }

  builder::dyn_var<Elem> force() {
    dummy_decl(BareSField::repr_read + ":" + name + ":" + instance_name);
    return value;
  }
  
  ///
  /// Perform addition compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator+(Rhs rhs);

  ///
  /// Perform subtraction compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator-(Rhs rhs);

  ///
  /// Perform multiplication compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator*(Rhs rhs);

  ///
  /// Perform division compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator/(Rhs rhs);

  ///
  /// Perform left shift compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator<<(Rhs rhs);

  ///
  /// Perform right shift compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator>>(Rhs rhs);

  ///
  /// Perform less than compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator<(Rhs rhs);

  ///
  /// Perform less than or equals compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator<=(Rhs rhs);

  ///
  /// Perform greater than compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator>(Rhs rhs);

  ///
  /// Perform greater than or equals compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator>=(Rhs rhs);

  ///
  /// Perform equals compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator==(Rhs rhs);

  ///
  /// Perform not equals compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator!=(Rhs rhs);

  ///
  /// Perform boolean and compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator&&(Rhs rhs);

  ///
  /// Perform boolean or compound expression
  template <typename Rhs>
  builder::dyn_var<bool> operator||(Rhs rhs);

  ///
  /// Perform bitwise and compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator&(Rhs rhs);

  ///
  /// Perform bitwise or compound expression
  template <typename Rhs>
  builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> operator|(Rhs rhs);

private:

  builder::dyn_var<Elem> value;
  // Name of this field
  std::string name;
  // Name of the struct this belongs to
  std::string object_name;
  // Name of the particular struct instance this belongs to
  std::string instance_name;
};

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator+(Rhs rhs) {
  return force() + rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator-(Rhs rhs) {
  return force() - rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator*(Rhs rhs) {
  return force() * rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator/(Rhs rhs) {
  return force() / rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator<<(Rhs rhs) {
  return force() << rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator>>(Rhs rhs) {
  return force() >> rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator<(Rhs rhs) {
  return force() < rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator<=(Rhs rhs) {
  return force() <= rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator>(Rhs rhs) {
  return force() > rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator>=(Rhs rhs) {
  return force() >= rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator==(Rhs rhs) {
  return force() == rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator!=(Rhs rhs) {
  return force() != rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator&&(Rhs rhs) {
  return force() && rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<bool> SField<Elem,IsPrimitive>::operator||(Rhs rhs) {
  return force() || rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator&(Rhs rhs) {
  return force() & rhs;
}

template <typename Elem, typename IsPrimitive>
template <typename Rhs>
builder::dyn_var<decltype(determine_ret_type<Elem,Rhs>())> SField<Elem,IsPrimitive>::operator|(Rhs rhs) {
  return force() | rhs;
}

///
/// Free version of SField::operator+ between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator+(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs + rhs.force();
}

///
/// Free version of SField::operator- between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator-(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs - rhs.force();
}

///
/// Free version of SField::operator* between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator*(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs * rhs.force();
}

///
/// Free version of SField::operator/ between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator/(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs / rhs.force();
}

///
/// Free version of SField::operator<< between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator<<(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs << rhs.force();
}

///
/// Free version of SField::operator>> between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator>>(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs >> rhs.force();
}

///
/// Free version of SField::operator< between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator<(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs < rhs.force();
}

///
/// Free version of SField::operator<= between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator<=(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs <= rhs.force();
}

///
/// Free version of SField::operator> between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator>(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs > rhs.force();
}

///
/// Free version of SField::operator>= between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator>=(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs >= rhs.force();
}

///
/// Free version of SField::operator== between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator==(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs == rhs.force();
}

///
/// Free version of SField::operator!= between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator!=(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs != rhs.force();
}

///
/// Free version of SField::operator&& between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator&&(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs && rhs.force();
}

///
/// Free version of SField::operator|| between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<bool> operator||(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs || rhs.force();
}

///
/// Free version of SField::operator& between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator&(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs & rhs.force();
}

///
/// Free version of SField::operator| between non-SField and SField
template <typename Lhs, typename Elem, typename IsPrimitive>
builder::dyn_var<decltype(determine_ret_type<Lhs,Elem>())> operator|(Lhs lhs, SField<Elem,IsPrimitive> rhs) {
  return lhs | rhs.force();
}
/*
template <typename Elem>
struct PField {

  explicit PField(std::string name) : name(name) { 
    assert(!StagedObject::object_fields.empty());
    if (StagedObject::object_fields.top().count(name) != 0) {
//      std::cerr << "Duplicate field " << name << " for user-defined StagedObject " << StagedObject::objects.top() << std::endl;
      exit(48);
    }
    // TODO string type
    StagedObject::object_fields.top()[name] = "pfield<TODO>";
  }

private:

  std::string name;
};
*/

}

