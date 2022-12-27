#include <sstream>
#include <iostream>
#include "staged/staged.h"
#include "staged/bitstream.h"

using namespace std;
using namespace bits;

void parse_h264(builder::dyn_var<uint8_t*> buffer, builder::dyn_var<uint64_t> length) {
  auto bitstream = Bitstream(buffer, length);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage: ./sh264 <output_fn>" << endl;
    exit(-1);
  }
  chrono::steady_clock::time_point start = chrono::steady_clock::now();
  ofstream src;
  ofstream hdr;
  string source = string(argv[1]) + ".cpp";
  string header = string(argv[1]) + ".h";
  src.open(source);
  hdr.open(header);
  hdr << "#pragma once" << endl;
  hdr << "#include <cmath>" << endl;
  hdr << "void parse_h264(uint8_t *data, uint64_t length);" << endl;
  hdr.flush();
  hdr.close();
  src << "#include \"" << header << "\"" << endl;
  src << "#include \"hmda_runtime.h\"" << endl;
  stringstream ss;
  hmda::stage(parse_h264, "parse_h264", ss);
  src << ss.str() << endl;
  src.flush();
  src.close();
  chrono::steady_clock::time_point stop = chrono::steady_clock::now();
  cout << "Staging took " << chrono::duration_cast<chrono::nanoseconds> (stop - start).count()/1e9 << "s" << endl;    
}
