#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "base.h"
#include "functors.h"
#include "staged/staged.h"
#include "unstaged/unstaged_blocklike.h"

using namespace std;
using namespace hmda;

template <typename T>
__attribute__((noinline))
void checkable(T x) {
  static volatile T t = 0;
  t = t + x;
}

void foo() { }

#define CALLABLE(func_name) \
  builder::dyn_var<decltype(func_name)> callable_ ## func_name = builder::as_global(#func_name);

// create dyn var wrappers for any functions you want to call within the generated staged code
CALLABLE(foo);


// These unstage functions happen within the staged function
template<int Depth, typename Elem, int Rank, typename...Rest>
void unstage_bextents(SView<Elem,Rank,Rest...> &sview, builder::dyn_var<loop_type[Rank*6]> &arr) {
  if constexpr (Depth < Rank) {
    arr[Depth] = get<Depth>(sview.bextents);
    unstage_bextents<Depth+1>(sview, arr);
  }
}

template<int Depth, typename Elem, int Rank, typename...Rest>
void unstage_bstrides(SView<Elem,Rank,Rest...> &sview, builder::dyn_var<loop_type[Rank*6]> &arr) {
  if constexpr (Depth < Rank) {
    arr[Depth+Rank] = get<Depth>(sview.bstrides);
    unstage_bstrides<Depth+1>(sview, arr);
  }
}

template<int Depth, typename Elem, int Rank, typename...Rest>
void unstage_borigin(SView<Elem,Rank,Rest...> &sview, builder::dyn_var<loop_type[Rank*6]> &arr) {
  if constexpr (Depth < Rank) {
    arr[Depth+Rank*2] = get<Depth>(sview.borigin);
    unstage_borigin<Depth+1>(sview, arr);
  }
}

template<int Depth, typename Elem, int Rank, typename...Rest>
void unstage_vextents(SView<Elem,Rank,Rest...> &sview, builder::dyn_var<loop_type[Rank*6]> &arr) {
  if constexpr (Depth < Rank) {
    arr[Depth+Rank*3] = get<Depth>(sview.vextents);
    unstage_vextents<Depth+1>(sview, arr);
  }
}

template<int Depth, typename Elem, int Rank, typename...Rest>
void unstage_vstrides(SView<Elem,Rank,Rest...> &sview, builder::dyn_var<loop_type[Rank*6]> &arr) {
  if constexpr (Depth < Rank) {
    arr[Depth+Rank*4] = get<Depth>(sview.vstrides);
    unstage_vstrides<Depth+1>(sview, arr);
  }
}

template<int Depth, typename Elem, int Rank, typename...Rest>
void unstage_vorigin(SView<Elem,Rank,Rest...> &sview, builder::dyn_var<loop_type[Rank*6]> &arr) {
  if constexpr (Depth < Rank) {
    arr[Depth+Rank*5] = get<Depth>(sview.vorigin);
    unstage_vorigin<Depth+1>(sview, arr);
  }
}

template <typename Elem, int Rank, typename UserFunc, typename...Rest>
builder::dyn_var<loop_type[Rank*6]> unstage(SView<Elem,Rank,Rest...> &sview, UserFunc user_func) {
  builder::dyn_var<loop_type[Rank*6]> arr;
  unstage_bextents<0>(sview, arr);
  unstage_bstrides<0>(sview, arr);
  unstage_borigin<0>(sview, arr);
  unstage_vextents<0>(sview, arr);
  unstage_vstrides<0>(sview, arr);
  unstage_vorigin<0>(sview, arr);
  return arr;
}

struct CodeGen: public block::c_code_generator {
  using block::c_code_generator::visit;
  using block::c_code_generator::c_code_generator;
  virtual void visit(block::expr_stmt::Ptr s) {
    string wtf = "wtf";
    if (!s->annotation.compare(0, wtf.size(), wtf)) {
      cout << "GOT WTF" << endl;
      // TODO left off here 
    }
//    block::c_code_generator::visit(s);
  }
public:
  static void generate_code(block::block::Ptr ast, std::ostream &oss, int indent = 0) {
		CodeGen generator(oss);
		generator.curr_indent = indent;
		ast->accept(&generator);
		oss << std::endl;
	}
};



static void simple_stage(builder::dyn_var<int*> data, Block<int,2> ublock) {
  auto block = ublock.stage(data);
  Iter<'I'> I;
  Iter<'J'> J;
  //  auto v2 = block.view(slice(0,9,1),slice(0,5,1));
  //  v2[I][J] = 99;

  for (builder::dyn_var<int> i = 0; i < 16; i=i+8) {
    for (builder::dyn_var<int> j = 0; j < 16; j=j+8) {
      auto v = block.view(slice(i,i+8,1),slice(j,j+8,1));
      //unstage(v);
      // TODO left off here. some combination of adding annotations
      // and an "unstage" call should let me rebuild a view...
      builder::annotate("unstage");
      callable_foo(i,j);
//      v[I][J] = I+J+i+j;
//      callable_my_func(i,j);
    }    
  }
}

int main() {
  // Create an unstaged block that you can do whatever with
  auto my_block = Block<int,3>({3,4,5}, {1,1,1}, {0,0,0});
  cout << my_block.dump() << endl;
  auto my_view = my_block.view();
  cout << my_view.dump() << endl;
  //  stage(my_function_to_stage, std::tuple{}, "foozle", my_block);
  auto block2d = Block<int,2>({16,16}, {1,1}, {0,0});
  // this (eventually) would dynamically compile and run the code on block2d.
  // use "stage" when you just want to run a standalone general kernel (like
  // color conversion)
//  stage(simple_stage, std::tuple{}, "simple_stage", block2d);

  auto ast = builder::builder_context().extract_function_ast(simple_stage, 
							     "simple_stage", 
							     block2d);
  block::eliminate_redundant_vars(ast);	
//  block::c_code_generator::generate_code(ast, std::cout, 0);
  CodeGen::generate_code(ast, std::cout, 0);

  return 0;
}

