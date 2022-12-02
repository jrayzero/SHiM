#include "builder/dyn_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "staged/staged_blocklike.h"

using namespace std;
using namespace hmda;
using builder::dyn_var;

/*template <int Rank, typename T>
struct WrappedRecContainer;

// A recursive container for holding dyn_vars. Gets rid of weird issues with built-in data
// structures that screw up the semantics of the dyn_vars. I don't want to just
// use dyn_var<int[]> for holding things like extents b/c it generates code like x={2,4},
// and I don't want to rely on the compiler to extract the array elements.
template <typename T>
struct RecContainer {

  template <int Rank, typename T2>
  friend struct WrappedRecContainer;

  dyn_var<T> val;
  unique_ptr<RecContainer<T>> nested;

  template <typename...Args>
  RecContainer(T arg, Args...args) : val(arg), nested(build_reccon(args...)) { }

  template <typename...Args>
  RecContainer(dyn_var<T> arg, Args...args) : val(arg), nested(build_reccon(args...)) { }

private:

  template <typename...Args>
  static unique_ptr<RecContainer<T>> build_reccon(T arg, Args...args) {
    return make_unique<RecContainer<T>>(arg, args...);
  }

  static unique_ptr<RecContainer<T>> build_reccon() {
    return nullptr;
  }

  template <int Rank, int Depth=Rank>
  auto deepcopy() const {
    dyn_var<T> d = this->val;
    if constexpr (Depth == Rank) {
      auto u = this->nested->template deepcopy<Rank,Depth-1>();
      auto r = RecContainer<T>(d);
      r.nested = move(u);
      return r;
    } else if constexpr (Depth > 1) {
      auto u = this->nested->template deepcopy<Rank,Depth-1>();
      auto r = make_unique<RecContainer<T>>(d);
      r->nested = move(u);
      return r;
    } else {
      return make_unique<RecContainer<T>>(d);
    }
  }
  
};

// A wrapper for RecContainer that contains a Rank, which makes it easier to deep copy.
template <int Rank, typename T>
struct WrappedRecContainer {
 
  RecContainer<T> reccon;

  template <typename...Args>
  WrappedRecContainer(T arg, Args...args) : reccon(arg, args...) { }

  WrappedRecContainer(const WrappedRecContainer &other) : reccon(other.reccon.template deepcopy<Rank>()) { }

};

template <int Rank>
struct Simple {

  WrappedRecContainer<Rank, int> extents;

  dyn_var<int*> data;

  Simple(WrappedRecContainer<Rank,int> extents) : extents(move(extents)) { }

};
*/

void staged() {
//  Block<int,1> block({4},{1},{0});
  Block<int,2> block({4,5},{1,1},{0,0});
/*  auto x = block(1,1);
  auto view = block.view();
  auto y = view(1,1);
  auto z = view.view(2,2)(7,8);
  Block<int,2> block2({2,2}, {1,2,3,4});*/
//  dyn_var<int[2]> extents{4,5};
//  Simple<2> simple({4,5});
//  simple.extents.reccon.val = 18;
//  Simple<2> simple2(simple.extents);
//  simple2.extents.reccon.val = 19;

  Iter<'i'> i;
  Iter<'j'> j;
  dyn_var<loop_type> y = 37;
  block[i][i] = block[0][0]+365+i;

  

}

int main() {
  builder::builder_context context;

  auto ast = context.extract_function_ast(staged, "staged");
  block::eliminate_redundant_vars(ast);	
  block::c_code_generator::generate_code(ast, std::cout, 0);
}
