// -*-c++-*-

#pragma once

#include <iostream>
#include <map>
#include "builder/dyn_var.h"

namespace cola {

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
  registered(false), name(name), instance_name(instance_name) { 
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
    // as a safe guard, check if register was called
    // this only helps if you allocate on the stack since it's guaranteed to go out of scope
    // since we don't support copy/move initialization and all that jazz, we only could come from the
    // constructor!
/*    if (!registered) {
      std::cerr << "Object " << name << " went out of scope but was never registered." <<  std::endl;
      exit(48);
    }*/
    if (collection.count(name) == 0) {
      assert(!object_fields.empty());
      collection[name] = object_fields.top();
//      objects.pop();
    }
    object_fields.pop();
//    object_instances.pop();
  }

  void do_register() {
/*    registered = true;
    if (collection.count(name) == 0) {
      assert(!object_fields.empty());
      collection[name] = object_fields.top();
      object_fields.pop();
//      objects.pop();
    }*/
  }

  // struct name -> {field name -> field type}
  static inline std::map<std::string, std::map<std::string, std::string>> collection;

  // the set of fields for the current object being created (it's a stack
  // since you can have nested objects)
  static inline std::stack<std::map<std::string, std::string>> object_fields;

  // the current objects (struct names)
//  static inline std::stack<std::string> objects;

  // the current objects (instance names)
//  static inline std::stack<std::string> object_instances;

  static inline const std::string build_staged_object_repr = "build_staged_object";

  static inline int next_instance = 0;

  // name of the struct object
  std::string name;

  // name of the particular instance
  std::string instance_name;

private:

  // whether the user called do_register
  bool registered;

};

// Just a holder for some static things so don't need a template parameter
// when accessing the static things in SField
struct BareSField {
  static inline const std::string repr = "sfield";
};

// If you don't care about the name (like, you're not gonna use this outside of the staged code), 
// then just create a dummy name for each
template <typename Elem>
struct SField {

  explicit SField(std::string name, StagedObject *container) : 
  name(name), object_name(container->name), instance_name(container->instance_name) { 
    assert(!StagedObject::object_fields.empty());
    if (StagedObject::object_fields.top().count(name) != 0) {
      std::cerr << "Duplicate field " << name << " for user-defined StagedObject " << object_name << std::endl;
      exit(48);
    }
    // TODO string type
    StagedObject::object_fields.top()[name] = elem_to_str<Elem>();
  }

  operator builder::dyn_var<Elem>() {
    builder::annotate(BareSField::repr + ":" + name + ":" + instance_name);
    dummy_decl();
    return value;
  }
  
private:

  builder::dyn_var<Elem> value;
  // Name of this field
  std::string name;
  // Name of the struct and particular instance that this belongs to
  std::string object_name;
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

