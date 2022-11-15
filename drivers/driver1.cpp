#include "blocks/c_code_generator.h"
#include "hmda.h"
#include "arrays.h"

using namespace std;

static void test(builder::dyn_var<int*> A, int foo) {
  hmda::Iter<'I'> i;
  hmda::Iter<'J'> j;
  auto block = hmda::factory<int, 2>(A,4,5);
  block[i][j] = 19;
}

int main() {

  builder::builder_context context;
  auto ast = context.extract_function_ast(test, "test", 0);
  block::c_code_generator::generate_code(ast, std::cout, 0);
  return 0;  

}
