// -*-c++-*-

#pragma once

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
public:
//  hmda_cpp_code_generator(std::ostream &oss) : c_code_generator(oss) { }
  static void generate_code(block::block::Ptr ast, std::ostream &oss, int indent = 0) {
    hmda_cpp_code_generator generator;//(oss);
    generator.curr_indent = indent;
    // first generate all the struct definitions
    std::stringstream structs;
    for (auto kv : builder::StructInfo::structs) {
      structs << "struct " << kv.first << " {" << std::endl;
      for (auto p : kv.second) {
	structs << "  " << p.second << ";" << std::endl;
      }
      structs << "};" << std::endl;
    }
    oss << structs.str();
    // then back to the usual
    ast->accept(&generator);
    oss << generator.last;
    oss << std::endl;
  }

  // apply string-matching based optimizations
/*  void visit(block::for_stmt::Ptr s) {
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
  }*/

private:

  bool is_same(std::string is, std::string should_be) {
    return !is.compare(0, should_be.size(), should_be);
  }
  
};

template <typename Func, typename...Args>
void stage(Func func, std::string name, std::ostream &output, Args...args) {
  if (name.empty()) name = "__my_staged_func";
  auto ast = builder::builder_context().extract_function_ast(func, name, args...);
  block::eliminate_redundant_vars(ast);
  hmda_cpp_code_generator::generate_code(ast, output, 0);
}

}
