#pragma once

#include "bits.h"

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
void huffman_encode_block(int *obj, int color_idx, int last_val, Bits &bits, 
			  int *zigzag, const HuffmanCodes &codes);
#if VERSION==2
void pack_DC(Bits &bits, int idx, const HuffmanCodes &codes);
void pack_AC(Bits &bits, int idx, const HuffmanCodes &codes);
void pack_and_stuff(Bits &bits, int val, int nbits);
#endif
