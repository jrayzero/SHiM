#pragma once

#include "builder/builder_dynamic.h"

namespace hmda {

template <typename Func, typename...Args>
void stage(Func func, std::string name, std::ostream &output, Args...args) {
  if (name.empty()) name = "__my_staged_func";
  auto ast = builder::builder_context().extract_function_ast(func, name, args...);
  block::eliminate_redundant_vars(ast);
  output << "#include \"hmda_runtime.h\"" << std::endl;
  block::c_code_generator::generate_code(ast, output, 0);
}

}
