#pragma once

#include "builder/block_type_extractor.h"

namespace builder {

template <>
class type_extractor<bool> {
public:
  static block::type::Ptr extract_type(void) {
    block::scalar_type::Ptr type = std::make_shared<block::scalar_type>();
    type->scalar_type_id = block::scalar_type::BOOL_TYPE;
    return type;
  }
};

template <>
class type_extractor<signed char /*int8_t*/> {
public:
  static block::type::Ptr extract_type(void) {
    block::scalar_type::Ptr type = std::make_shared<block::scalar_type>();
    type->scalar_type_id = block::scalar_type::SIGNED_CHAR_TYPE;
    return type;
  }
};

  
}
