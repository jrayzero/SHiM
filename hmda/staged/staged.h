// -*-c++-*-

#pragma once

#include "blocks/c_code_generator.h"
#include "builder/builder_dynamic.h"

namespace hmda {
// fixes the syntax for staged function args of the form
// dyn_var<T[]>
// buildit prints "T[] arg", but correct syntax is "T arg[]"
class hmda_cpp_code_generator : public block::c_code_generator {
  using c_code_generator::visit;
public:
  hmda_cpp_code_generator(std::ostream &_oss) : block::c_code_generator(_oss) { }
  static void generate_code(block::block::Ptr ast, std::ostream &oss, int indent = 0) {
    hmda_cpp_code_generator generator(oss);
    generator.curr_indent = indent;
    ast->accept(&generator);
    oss << std::endl;
  }

  void visit(block::func_decl::Ptr a) {
    a->return_type->accept(this);
    if (a->hasMetadata<std::vector<std::string>>("attributes")) {
      const auto &attributes = a->getMetadata<std::vector<std::string>>("attributes");
      for (auto attr: attributes) {
	oss << " " << attr;
      }
    }
    oss << " " << a->func_name;
    oss << " (";
    bool printDelim = false;
    for (auto arg : a->args) {
      if (printDelim)
	oss << ", ";
      printDelim = true;
      if (block::isa<block::function_type>(arg->var_type)) {
	handle_func_arg(arg);
      } else if (block::isa<block::array_type>(arg->var_type)) { // change here 
	// Elem arg[]
	auto atype = block::to<block::array_type>(arg->var_type);
	atype->element_type->accept(this);
	oss << " " << arg->var_name;
	oss << "[";
	if (atype->size != -1) {
	  oss << atype->size;
	}
	oss << "]";
      } else { 
	arg->var_type->accept(this);
	oss << " " << arg->var_name;
      }
    }
    if (!printDelim)
      oss << "void";
    oss << ")";
    if (block::isa<block::stmt_block>(a->body)) {
      oss << " ";
      a->body->accept(this);
      oss << std::endl;
    } else {
      oss << std::endl;
      curr_indent++;
      printer::indent(oss, curr_indent);
      a->body->accept(this);
      oss << std::endl;
      curr_indent--;
    }    
  }
};

template <typename Func, typename...Args>
void stage(Func func, std::string name, std::ostream &output, Args...args) {
  if (name.empty()) name = "__my_staged_func";
  auto ast = builder::builder_context().extract_function_ast(func, name, args...);
  block::eliminate_redundant_vars(ast);
  output << "#include \"hmda_runtime.h\"" << std::endl;
  hmda_cpp_code_generator::generate_code(ast, output, 0);
}

}
