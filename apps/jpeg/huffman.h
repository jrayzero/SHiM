#pragma once

#include "bits.h"
#include "hmda_unstaged.h"

using namespace hmda;
using namespace std;

extern int luma_DC_huffbits[17];
extern int luma_DC_huffvals[12];
extern int luma_AC_huffbits[17];
extern int luma_AC_huffvals[162];
extern int chroma_DC_huffbits[17];
extern int chroma_DC_huffvals[12];
extern int chroma_AC_huffbits[17];
extern int chroma_AC_huffvals[162];

struct HuffmanCodes {
  int dc_ehufco[256];
  int dc_ehufsz[256];
  int ac_ehufco[256];
  int ac_ehufsz[256];
};

HuffmanCodes generate_codes(int *DC_bits, int *DC_vals, int *AC_bits, int *AC_vals);
void huffman_encode_block(View<int,3> &obj, int last_val, Bits &bits, 
			  const Block<int,2> &zigzag, const HuffmanCodes &codes);
