// -*-c++-*-

#pragma once

#include <chrono>
#include <iostream>
#include "common/utils.h"
#include "blocks/block_replacer.h"
#include "builder/builder_dynamic.h"
#include "object.h"

namespace shim {
  
///
/// Look for instances of <type> var = build_stackarr_<type>(<sz>) calls and replace
/// with build_stackarr_type(var, <sz>), as this corresponds to a macro call in the 
/// generated code for building an array.
class ReplaceStackBuilder : public block::block_replacer {
  using block_replacer::visit;
  void visit(block::decl_stmt::Ptr a) {
    block::expr::Ptr rhs = a->init_expr;
    if (block::isa<block::function_call_expr>(rhs)) {
      block::function_call_expr::Ptr func = block::to<block::function_call_expr>(rhs);
      block::expr::Ptr name = func->expr1;
      if (block::isa<block::var_expr>(name) && 
	  block::isa<block::var>(block::to<block::var_expr>(name)->var1)) {
	std::string sname = block::to<block::var>(block::to<block::var_expr>(name)->var1)->var_name;
	if (sname == "SHIM_BUILD_STACK_UINT8_T"
	    || sname == "SHIM_BUILD_STACK_UINT16_T"
	    || sname == "SHIM_BUILD_STACK_UINT32_T"
	    || sname == "SHIM_BUILD_STACK_UINT64_T"
	    || sname == "SHIM_BUILD_STACK_CHAR"
	    || sname == "SHIM_BUILD_STACK_INT8_T"
	    || sname == "SHIM_BUILD_STACK_INT16_T"
	    || sname == "SHIM_BUILD_STACK_INT32_T"
	    || sname == "SHIM_BUILD_STACK_INT64_T"
	    || sname == "SHIM_BUILD_STACK_FLOAT"
	    || sname == "SHIM_BUILD_STACK_DOUBLE") {
	  // pass in the var as a pure var object and not a string because 
	  // don't want it passed as a string to the macro
	  block::function_call_expr::Ptr frepl = std::make_shared<block::function_call_expr>();
	  frepl->expr1 = name;
	  block::var_expr::Ptr varrepl = std::make_shared<block::var_expr>();
	  varrepl->var1 = a->decl_var;
	  frepl->args = std::vector<block::expr::Ptr>{varrepl, func->args[0]};
	  block::expr_stmt::Ptr estmt = std::make_shared<block::expr_stmt>();
	  estmt->expr1 = std::move(frepl);
	  estmt->static_offset = a->static_offset;
	  estmt->annotation = a->annotation;
	  node = estmt;
	} else {
	  block::block_replacer::visit(a);
	}
      } else {
	block::block_replacer::visit(a);
      }
    } else {
      block::block_replacer::visit(a);
    }
  }

};

}
