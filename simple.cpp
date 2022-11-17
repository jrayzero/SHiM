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

void foo(int,int) { }

#define CALLABLE(func_name) \
  builder::dyn_var<decltype(func_name)> callable_ ## func_name = builder::as_global(#func_name);

// create dyn var wrappers for any functions you want to call within the generated staged code
CALLABLE(foo);

/*static void my_function_to_stage(builder::dyn_var<int*> data, Block<int,3> ublockA) {
  // must stage it first
  auto blockA = ublockA.stage(data);
  // then do your stuff  
  auto viewA = blockA.view();
  Iter<'i'> i;
  Iter<'j'> j;
  Iter<'k'> k;
  blockA[i][j][k] = i+j+blockA[i][j][0];
  viewA[i][j][k] = 1+i;//blockA[i+j][j][0];

  viewA.view(slice(0,3,1), slice(0,4,1), slice(4,5,2))[i][j][k] = 888;

  for (builder::dyn_var<int> i = 0; i < 3; i=i+1) {
    viewA.view(slice(i,3,1), slice(0,4,1), slice(4,5,2))[i][j][k] = 888;    
  }

  // call an external function
  callable_foo(0,1);
  
  }*/

static void simple_stage(builder::dyn_var<int*> data, Block<int,2> ublock) {
  auto block = ublock.stage(data);
  Iter<'I'> I;
  Iter<'J'> J;
  //  auto v2 = block.view(slice(0,9,1),slice(0,5,1));
  //  v2[I][J] = 99;

  for (builder::dyn_var<int> i = 0; i < 16; i=i+8) {
    for (builder::dyn_var<int> j = 0; j < 16; j=j+8) {
      auto v = block.view(slice(i,i+8,1),slice(j,j+8,1));
      v[I][J] = I+J+9;
      callable_foo(i,j);
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
  stage(simple_stage, std::tuple{}, "simple_stage", block2d);

  return 0;
}

