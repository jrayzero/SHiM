#pragma once

#include "expr.h"
#include "staged_blocklike.h"
#include "loops.h"
#include "schedule.h"

namespace hmda {

// Call this with your function pointer, non-blocklike args, and unstaged blocklike args.
// it will generate the code and then run it, acting like a jit (but I feel like JIT has connotations
// with converting bytecode...)
template <typename Func, typename ArgTuple, typename...Unstaged>
void stage(Func func, ArgTuple arg_tuple, std::string name, Unstaged...unstaged) {
  // TODO checked that each Unstaged is really unstaged
  // TODO caching (make a "caching policy" that says how you compare blocklikes--just dims, dims + strides, etc)
  if (name.empty()) name = "__my_staged_func";
  if constexpr (std::tuple_size<ArgTuple>() == 0) {
    auto ast = builder::builder_context().extract_function_ast(func, name, unstaged...);
    block::eliminate_redundant_vars(ast);	
    block::c_code_generator::generate_code(ast, std::cout, 0);
  } else {
    auto ast = builder::builder_context().extract_function_ast(func, name, arg_tuple, unstaged...);
    block::eliminate_redundant_vars(ast);	
    block::c_code_generator::generate_code(ast, std::cout, 0);
  }
  // TODO now we'd dynamically load this little codelet and pass in the unstaged buffer into it
}

template <typename Func, typename...Unstaged>
void stage(Func func, std::string name, Unstaged...unstaged) {
  stage(func, std::tuple{}, name, unstaged...);
}

}
