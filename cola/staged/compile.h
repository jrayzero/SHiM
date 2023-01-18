// -*-c++-*-

#pragma once

#include <chrono>
#include <iostream>
#include "common/utils.h"
#include "blocks/c_code_generator.h"
#include "builder/builder_dynamic.h"
#include "annotations.h"
#include "object.h"

namespace cola {
// fixes the syntax for staged function args of the form
// dyn_var<T[]>
// buildit prints "T[] arg", but correct syntax is "T arg[]"
class hmda_cpp_code_generator : public block::c_code_generator {
  using c_code_generator::visit;
  std::vector<std::string> sigs;
public:
  static std::vector<std::string> generate_code(block::block::Ptr ast, std::ostream &oss, int indent = 0) {
    hmda_cpp_code_generator generator;
    generator.curr_indent = indent;
/*    // first generate all the struct definitions
    std::stringstream structs;
    for (auto kv : builder::StructInfo::structs) {
      structs << "struct " << kv.first << " {" << std::endl;
      for (auto p : kv.second) {
	structs << "  " << p.second << ";" << std::endl;
      }
      structs << "};" << std::endl;
    }
    oss << structs.str();*/
    // then back to the usual
    ast->accept(&generator);
    oss << generator.last;
    oss << std::endl;
    return generator.sigs;
  }
  
  void visit(block::func_decl::Ptr a) {
    std::stringstream ss;
    a->return_type->accept(this);
    ss << last;
    if (a->hasMetadata<std::vector<std::string>>("attributes")) {
      const auto &attributes = a->getMetadata<std::vector<std::string>>("attributes");
      for (auto attr: attributes) {
	ss << " " << attr;
      }
    }
    ss << " " << a->func_name;
    ss << " (";
    bool printDelim = false;
    for (auto arg : a->args) {
      if (printDelim)
	ss << ", ";
      printDelim = true;
      if (block::isa<block::function_type>(arg->var_type)) {
	handle_func_arg(arg);
      } else {
	arg->var_type->accept(this);
	ss << last;
	ss << " " << arg->var_name;
      }
    }
    if (!printDelim)
      ss << "void";
    ss << ")";
    sigs.push_back(ss.str());
    if (block::isa<block::stmt_block>(a->body)) {
      ss << " ";
      a->body->accept(this);
      ss << last;
      ss << std::endl;
    } else {
      ss << std::endl;
      curr_indent++;
      printer::indent(ss, curr_indent);
      a->body->accept(this);
      ss << last;
      ss << std::endl;
      curr_indent--;
    }
    last = ss.str();
  }

  // apply string-matching based optimizations
  void visit(block::for_stmt::Ptr s) {
    std::stringstream ss;
    std::string annot = s->annotation;
    // first split on different annotations
    std::vector<std::string> annots = split_on(annot, ";");
    if (annots.size() > 1) {
      std::cerr << "Only supporting single opt applications currently." << std::endl;
      exit(-1);
    }
    if (!annots.empty()) {
      std::vector<std::string> comps = split_on(annots[0], ":");
      if (!comps[0].compare(0, Optimization::opt_prefix.size(), Optimization::opt_prefix)) {
	// okay, it's an optimization!
	if (comps.size() == 1) {
	  std::cerr << "Not enough components in optimization: " << annots[0] << std::endl;
	  exit(-1);
	}
	if (!comps[1].compare(0, Parallel::repr.size(), Parallel::repr)) {
	  std::stringstream prg;
	  prg << "#pragma omp parallel for";
	  int nworkers = stoi(comps[2]);
	  if (nworkers > 0) {
	    prg << " num_threads(" << nworkers << ")" << std::endl;
	  } else {
	    prg << std::endl;
	  }

	  ss << prg.str();
	  printer::indent(ss, curr_indent);
	}
	block::c_code_generator::visit(s);
	ss << last;
      } else {
	std::cerr << "Unknown AST annotation: " << annot << std::endl;
	exit(48);
      }
    } else {
      block::c_code_generator::visit(s);
      ss << last;
    }
    last = ss.str();
  }

private:

  bool is_same(std::string is, std::string should_be) {
    return !is.compare(0, should_be.size(), should_be);
  }
  
};

struct CompileOptions {
  static inline bool isCPP = true;
};

// isCPP just dictates whether or not the functions are wrapped with extern in the c code
template <typename Func, typename...Args>
void stage(Func func, std::string name, std::string fn_prefix, std::string pre_hdr, std::string pre_src, Args...args) {
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::ofstream src;
  std::ofstream hdr;
  std::string header = fn_prefix + ".h";  
  std::string source = CompileOptions::isCPP ? fn_prefix + ".cpp" : fn_prefix + ".c";
  hdr.open(header);
  src.open(source);
  if (name.empty()) name = "__my_staged_func";
  auto ast = builder::builder_context().extract_function_ast(func, name, args...);
  block::eliminate_redundant_vars(ast);
  hdr << "#pragma once" << std::endl;
  hdr << pre_hdr;
  src << pre_src;
  if (!CompileOptions::isCPP) {
    src << "#include <stdio.h>" << std::endl;
    src << "#include <math.h>" << std::endl;
    src << "#include <stdbool.h>" << std::endl;
    src << "#include <stdlib.h>" << std::endl;
    hdr << "#include <stdbool.h>" << std::endl;
  } else {
    src << "#include <cstdio>" << std::endl;
  } 
  // Plain C does not support the HeapArray and things like that!
  if (CompileOptions::isCPP)
    src << "#include \"runtime/runtime.h\"" << std::endl;
  if (!CompileOptions::isCPP) {
    src << "#define lshift(a,b) ((a) << (b))" << std::endl;
    src << "#define rshift(a,b) ((a) >> (b))" << std::endl;
  }
  src << "#define COLA_CAST_UINT8_T(x) (uint8_t)(x)" << std::endl;
  src << "#define COLA_CAST_UINT16_T(x) (uint16_t)(x)" << std::endl;
  src << "#define COLA_CAST_UINT32_T(x) (uint32_t)(x)" << std::endl;
  src << "#define COLA_CAST_UINT64_T(x) (uint64_t)(x)" << std::endl;
  src << "#define COLA_CAST_INT8_T(x) (int8_t)(x)" << std::endl;
  src << "#define COLA_CAST_INT16_T(x) (int16_t)(x)" << std::endl;
  src << "#define COLA_CAST_INT32_T(x) (int32_t)(x)" << std::endl;
  src << "#define COLA_CAST_INT64_T(x) (int64_t)(x)" << std::endl;
  src << "#define COLA_CAST_CHAR(x) (char)(x)" << std::endl;
  src << "#define COLA_CAST_FLOAT(x) (float)(x)" << std::endl;
  src << "#define COLA_CAST_DOUBLE(x) (double)(x)" << std::endl;
  src << "void print_newline() { printf(\"\\n\"); }" << std::endl;
  std::stringstream src2;
  std::vector<std::string> sigs = hmda_cpp_code_generator::generate_code(ast, src2, 0);
  for (auto sig : sigs) {
    hdr << sig << ";" << std::endl;
  }
  src << src2.str();
  hdr.flush();
  hdr.close();
  src.flush();
  src.close();
  std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
  std::cout << "Staging took " << std::chrono::duration_cast<std::chrono::nanoseconds> (stop - start).count()/1e9 << "s" << std::endl;  
}

std::string basename(std::string path, std::string suffix="") {
  std::stringstream f;
  std::string f2 = std::filesystem::path(path).stem();
  f << f2 << suffix;
  return f.str();
}

}
