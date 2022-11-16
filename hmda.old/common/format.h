#pragma once

namespace hmda {

  template<typename T>
  struct TypeToStr {
  };
  
  template<>
  struct TypeToStr<bool> {
    std::string operator()() { return "bool"; }
  };

  template<>
    struct TypeToStr<int32_t> {
    std::string operator()() { return "int32"; }
  };


  template<>
    struct TypeToStr<int64_t> {
    std::string operator()() { return "int64"; }
  };

  template<>
    struct TypeToStr<float> {
    std::string operator()() { return "float"; }
  };

  template<typename T>
    std::string type_to_str() {
    return TypeToStr<T>()();
  }

}
