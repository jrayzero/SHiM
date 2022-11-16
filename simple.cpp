#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "base.h"
#include "functors.h"
#include "expr.h"
#include "staged/staged.h"
#include "unstaged/unstaged_blocklike.h"

static void my_function_to_stage(builder::dyn_var<int*> data, Block<int,3,false> ublockA) {
  // must stage it first
  auto blockA = ublockA.stage(data);
  // then do your stuff
  Iter<'i'> i;
  Iter<'j'> j;
  Iter<'k'> k;
  blockA[i][j][k] = i+j+blockA[i][j][0];
  blockA(0,1,2);
}


int main() {
  // Create an unstaged block that you can do whatever with
  auto my_block = Block<int, 3, false>({3,4,5});
  stage(my_function_to_stage, std::tuple{}, "foozle", my_block);

  return 0;
}

