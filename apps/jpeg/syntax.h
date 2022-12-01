#pragma once

#include "bits.h"
#include "hmda_unstaged.h"

using namespace hmda;
using namespace std;

void syntax_SOI(Bits &bits);
void syntax_EOI(Bits &bits);
void syntax_JFIF(Bits &bits);
void syntax_frame_header(Bits &bits, int H, int W);
void syntax_scan_header(Bits &bits);
void syntax_quant_table(Bits &bits, const Block<int,2> &quant, 
			const Block<int,2> &zigzag, bool is_luma);
void syntax_huffman_table(Bits &bits, int *huffbits, int huffbits_len, 
			  int *huffvals, int huffvals_len,
			  bool is_dc, int ident);

