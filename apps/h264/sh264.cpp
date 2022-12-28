#include <sstream>
#include <iostream>
#include "staged/staged.h"
#include "staged/bitstream.h"

using namespace std;
using namespace cola;

using cursor = builder::dyn_var<uint64_t>;
using dint = builder::dyn_var<int>;

// TODO wrapper for stringstream with dyn_var so can make better error messages
template <typename C, typename T, typename U>
void check(C cursor, T got, U should_be, string elem) {
  if (got != should_be) {
    printn("Invalid value for syntax element <", elem, "> starting at cursor <", cursor, ">. Got ", got, ", expected ", should_be);
    hexit(-1);
  }
}

// NEED THE REFERENCE HERE SINCE THE CURSOR INSIDE THE BITSTREAM CHANGES! This is unlike HMDAs
void nal_unit(Bitstream &bitstream) {
  if (bitstream.exists(8)==0) {
    print_string("Expected at least 8 bits for nal_unit header");
    hexit(-1);
  }
  check(bitstream.cursor, bitstream.pop(1), 0, "forbidden_zero_bit");
  dint nal_ref_idc = bitstream.pop<int>(2); 
  dint nal_unit_type = bitstream.pop<int>(5);
  printn("<nal_ref_idc> = ", nal_ref_idc);
  printn("<nal_unit_type> = ", nal_unit_type);
  // remove all the padding bytes and overwrite the payload  
  cursor payload_idx = bitstream.cursor / 8;
  cursor starting_cursor = bitstream.cursor;
  dint bytes_in_payload = 0;
  dint cond = 1;
  while (cond) {
    if (bitstream.peek_aligned<int>(24) == 0x000001) {
      cond = 0;
    } else if (bitstream.peek_aligned<int>(32) == 0x00000001) {
      cond = 0;
    } else {
      if (bitstream.peek_aligned<int>(24) == 0x000003) {
	// emulation (stuffing) bits
	bitstream.bitstream[payload_idx] = bitstream.pop_aligned(8);
	bitstream.bitstream[payload_idx+1] = bitstream.pop_aligned(8);
	payload_idx = payload_idx + 2;
	bytes_in_payload = bytes_in_payload + 2;
	check(bitstream.cursor, bitstream.pop_aligned(8), 0x03, "emulation_prevention_three_byte");
      } else {
	bitstream.bitstream[payload_idx] = bitstream.pop_aligned(8);
	payload_idx = payload_idx + 1;
	bytes_in_payload = bytes_in_payload + 1;
      }
    }
  }
  printn("Read <", bytes_in_payload, "> bytes in nal unit payload.");
  // save the cursor before we process the payload according to the nal unit type
  cursor upper_cursor = bitstream.cursor;
  bitstream.cursor = starting_cursor;
  // TODO PROCESS PAYLOAD
  // now reset the cursor to parse the next nal unit
  bitstream.cursor = upper_cursor;
  printn("Cursor ended at ", bitstream.cursor);
}

void annexB(Bitstream &bitstream) {
  // buildit seg faults if I try to make this use a compound condition
  dint cond = 1;
  while (cond) {
    if (bitstream.peek_aligned<int>(24) != 0x000001) {
      if (bitstream.peek_aligned<int>(32) != 0x00000001) {
	check(bitstream.cursor, bitstream.pop_aligned(8), 0x00, "leading_zero_8bits");
      } else {
	cond = 0;
      }
    } else {
      cond = 0;
    }
  }
  if (bitstream.peek_aligned(24) != 0x000001) {
    check(bitstream.cursor, bitstream.pop_aligned(8), 0x00, "zero_byte");
  }
  check(bitstream.cursor, bitstream.pop_aligned(24), 0x000001, "start_code_prefix_one_3bytes");
  nal_unit(bitstream);
  cond = 1;
  while (cond) {
    if (bitstream.peek_aligned<int>(24) != 0x000001) {
      if (bitstream.peek_aligned<int>(32) != 0x00000001) {
	check(bitstream.cursor, bitstream.pop_aligned(8), 0x00, "trailing_zero_8bits");
      } else {
	cond = 0;
      }
    } else {
      cond = 0;
    }
  }
}

void parse_h264(builder::dyn_var<uint8_t*> buffer, builder::dyn_var<uint64_t> length) {
  auto bitstream = Bitstream(buffer, length);
  while (bitstream.exists(8)==1) {
    annexB(bitstream);
  }
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
  src << "#include \"runtime/runtime.h\"" << endl;
  stringstream ss;
  stage(parse_h264, "parse_h264", ss);
  src << ss.str() << endl;
  src.flush();
  src.close();
  chrono::steady_clock::time_point stop = chrono::steady_clock::now();
  cout << "Staging took " << chrono::duration_cast<chrono::nanoseconds> (stop - start).count()/1e9 << "s" << endl;    
}
