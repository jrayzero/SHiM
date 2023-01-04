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
  hmda_cpp_code_generator(std::ostream &_oss) : block::c_code_generator(_oss) { }
  static void generate_code(block::block::Ptr ast, std::ostream &oss, int indent = 0) {
    hmda_cpp_code_generator generator(oss);
    generator.curr_indent = indent;
    // first generate all the struct definitions
    std::stringstream structs;
    for (auto kv : StagedObject::collection) {
      structs << "struct " << kv.first << " {" << std::endl;
      for (auto p : kv.second) {
	structs << "  " << p.second << " " << p.first << ";" << std::endl;
      }
      structs << "};" << std::endl;
    }
    oss << structs.str();
    // then back to the usual
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

  // apply string-matching based optimizations
  void visit(block::for_stmt::Ptr s) {
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
	  oss << prg.str();
	  printer::indent(oss, curr_indent);
	}
	block::c_code_generator::visit(s);
      } else {
	std::cerr << "Unknown AST annotation: " << annot << std::endl;
	exit(48);
      }
    } else {
      block::c_code_generator::visit(s);
    }
  }

  // Mostly deals with finding my annotations for staged objects
  void visit(block::decl_stmt::Ptr s) {
    std::string annot = s->annotation;
    std::vector<std::string> annots = split_on(annot, ";");
    if (annots.size() > 1) {
      std::cerr << "Only supporting single opt applications currently." << std::endl;
      exit(-1);
    }
    if (!annots.empty()) {
      std::vector<std::string> comps = split_on(annots[0], ":");
      // TODO don't hardcode the annot name--make a static variable in StagedObject for it
      if (is_same(comps[0], StagedObject::build_staged_object_repr)) {
	std::stringstream prg;
	prg << comps[1] << " " << comps[2] << ";";
	oss << prg.str();
	printer::indent(oss, curr_indent);
	// don't generate the actual loop--it's just a dummy
      } else if (is_same(comps[0], BareSField::repr_read)) {
	// Delay until var_expr
	std::stringstream prg;
	prg << comps[2] << "." << comps[1];
	is_delayed_var = true;
	delayed_var_repl = prg.str();
	// don't generate the actual loop--it's just a dummy
      } else if (is_same(comps[0], BareSField::repr_write)) {
	// Delay until var_expr
	std::stringstream prg;
	prg << comps[2] << "." << comps[1];
	is_delayed_var = true;
	delayed_var_repl = prg.str();
	// don't generate the actual loop--it's just a dummy
      } else {
	std::cerr << "Unknown AST annotation: " << annot << std::endl;
	exit(48);
      }
    } else {
      block::c_code_generator::visit(s);
    }
  }

  void visit(block::var_expr::Ptr s) {
    if (is_delayed_var) {
      assert(!delayed_var_repl.empty());
      oss << delayed_var_repl;
      is_delayed_var = false;
      delayed_var_repl = "";
    } else {
      block::c_code_generator::visit(s);
    }
  }

/*  void visit(block::assign_expr::Ptr s) {
    if (is_delayed_var) {
      assert(!delayed_var_repl.empty());
      oss << delayed_var_repl << " = ";
      s->expr1->accept(this);
      is_delayed_var = false;
      delayed_var_repl = "";
    } else {
      block::c_code_generator::visit(s);
    }
  }*/

private:

  bool is_delayed_var = false;
  std::string delayed_var_repl;

  bool is_same(std::string is, std::string should_be) {
    return !is.compare(0, should_be.size(), should_be);
  }
  
};

template <typename Func, typename...Args>
void stage(Func func, std::string name, std::ostream &output, Args...args) {
  if (name.empty()) name = "__my_staged_func";
  auto ast = builder::builder_context().extract_function_ast(func, name, args...);
//  block::eliminate_redundant_vars(ast);
  hmda_cpp_code_generator::generate_code(ast, output, 0);
}

}
