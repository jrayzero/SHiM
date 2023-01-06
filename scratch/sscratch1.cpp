#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "builder/dyn_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "staged/staged.h"

using namespace std;
using namespace cola;
using builder::dyn_var;
using builder::static_var;

using dint = dyn_var<int>;

void foo() {
  Block<int,2> myBlock = Block<int,2>::heap({10,10});
  Iter<'i'> i;
  Iter<'j'> j;
  myBlock[i][j] = i+j*2;
  myBlock.view().dump_loc();
  myBlock.dump_data();
}

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage: ./sscratch <output_fn>" << endl;
    exit(-1);
  }
  std::stringstream ss;
  stage(foo, false, "scratch", argv[1], ss.str(), ss.str());
}
