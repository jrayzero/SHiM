// -*-c++-*-

#pragma once

#include <iostream>
#include <map>
#include "builder/dyn_var.h"

namespace cola {

// TODO fields in the struct aren't not necessarily in the order the user specified. Make sure to put them
// in the same order

template <typename Elem>
std::string elem_to_str() {
  if constexpr (std::is_same<uint8_t,Elem>::value) {
    return "uint8_t";
  } else if constexpr (std::is_same<uint16_t,Elem>::value) {
    return "uint16_t";
  } else if constexpr (std::is_same<uint32_t,Elem>::value) {
    return "uint32_t";
  } else if constexpr (std::is_same<uint64_t,Elem>::value) {
    return "uint64_t";
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
}

// dummy decl for finding annotation
template <bool Dummy=false>
void dummy_decl() {
  builder::dyn_var<int> dummy = 0;
}

struct StagedObject {

  // I tried automatically generating the instance name, but the issue was that it would get out
  // of sync with everything since buildit needs to execute the code something in multiple places
  // (this maybe where buildit's novel static tag idea helps in buildit itself). But I don't want to
  // deal with that, so I just make the user specify the name
  explicit StagedObject(std::string name, std::string instance_name) : 
    name(name), instance_name(instance_name) { 
    object_fields.push({});
    builder::annotate(build_staged_object_repr + ":" + name + ":" + instance_name);
    dummy_decl();
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

  // struct name -> {field name -> field type}
  static inline std::map<std::string, std::map<std::string, std::string>> collection;

  // the set of fields for the current object being created (it's a stack
  // since you can have nested objects)
  static inline std::stack<std::map<std::string, std::string>> object_fields;

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
// when accessing the static things in SField
struct BareSField {
  static inline const std::string repr_read = "sfield_read";
  static inline const std::string repr_write = "sfield_write";
};

// If you don't care about the name (like, you're not gonna use this outside of the staged code), 
// then just create a dummy name for each
template <typename Elem>
struct SField {

  // TODO only allow default values for fundamental types
  explicit SField(StagedObject *container, std::string name="", builder::dyn_var<Elem> def_val=0) : 
    name(name.empty() ? "__field" + std::to_string(container->next_field++) : name),
    object_name(container->name), instance_name(container->instance_name) {
    assert(!StagedObject::object_fields.empty());
    if (StagedObject::object_fields.top().count(this->name) != 0) {
      std::cerr << "Duplicate field " << this->name << " for user-defined StagedObject " << object_name << std::endl;
      exit(48);
    }
    StagedObject::object_fields.top()[this->name] = elem_to_str<Elem>();
    this->operator=(def_val);
  }

  void operator=(builder::dyn_var<Elem> rhs) {
    builder::annotate(BareSField::repr_write + ":" + name + ":" + instance_name);
    dummy_decl();
    this->value = rhs;
  }

  operator builder::dyn_var<Elem>() {
    builder::annotate(BareSField::repr_read + ":" + name + ":" + instance_name);
    dummy_decl();
    return value;
  }
  
private:

  builder::dyn_var<Elem> value;
  // Name of this field
  std::string name;
  // Name of the struct this belongs to
  std::string object_name;
  // Name of the particular struct instance this belongs to
  std::string instance_name;
};

/*template <typename Elem, int Sz>
struct AField {

  explicit AField(std::string name) : name(name) {
    assert(!StagedObject::object_fields.empty());
    if (StagedObject::object_fields.top().count(name) != 0) {
//      std::cerr << "Duplicate field " << name << " for user-defined StagedObject " << StagedObject::objects.top() << std::endl;
      exit(48);
    }
    // TODO string type
    StagedObject::object_fields.top()[name] = "afield<TODO>";
  } 

private:

  std::string name;
};

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

