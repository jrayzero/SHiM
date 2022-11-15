#include "blocks/c_code_generator.h"
#include "staged/staged.h"

using namespace std;

using namespace hmda;

// The dyn_var here acts as the placeholder in the staged block for the array we write to
static void test(builder::dyn_var<int*> A, int foo) {
  staged::Iter<'I'> i;
  staged::Iter<'J'> j;
  auto block = staged::factory<int, 2>(A,4,5);
  block[i][j] = i + j;
}

int main() {

  builder::builder_context context;
  auto ast = context.extract_function_ast(test, "test", 0);
  block::c_code_generator::generate_code(ast, std::cout, 0);
  return 0;  

}
