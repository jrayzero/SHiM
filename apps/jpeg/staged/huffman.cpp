#include <stdio.h>
#include <iostream>
#include "huffman.h"

int luma_DC_huffbits[] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
int luma_DC_huffvals[] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB};
int luma_AC_huffbits[] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7D};
int luma_AC_huffvals[] =
  {0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
   0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
   0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
   0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
   0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
   0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
   0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
   0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
   0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
   0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
   0xF9,0xFA};

int chroma_DC_huffbits[] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
int chroma_DC_huffvals[] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB};
int chroma_AC_huffbits[] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
int chroma_AC_huffvals[] =
  {0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
   0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,
   0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,
   0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,
   0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,
   0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
   0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,
   0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,
   0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
   0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
   0xF9,0xFA};

int generate_huffsize(int *huffbits, int huffsize[257]) {
  int p = 0;
  for (int l = 1; l < 17; l++) {
    int i = huffbits[l];
    i--;
    while (i >= 0) {
      huffsize[p] = l;
      p++;
      i--;
    }
  }
  huffsize[p] = 0;
  return p;
}

void generate_huffcode(int huffsize[257], int huffcode[257]) {
  int code = 0;
  int si = huffsize[0];
  int p = 0;
  while (huffsize[p] != 0) {
    while (huffsize[p] == si) {
      huffcode[p] = code;
      p++;
      code++;     
    }
    code <<= 1;
    si++;
  }
}


void generate_ehufco(int *huffvals, int huffcode[257], int huffsize[257], int lastp,
		    int ehufco[256], int ehufsz[256]) {
  for (int p = 0; p < lastp; p++) {
    int i = huffvals[p];
    ehufco[i] = huffcode[p];
    ehufsz[i] = huffsize[p];
  }
}

HuffmanCodes generate_codes(int *DC_bits, int *DC_vals, int *AC_bits, int *AC_vals) {
  int DC_huffsize[257];
  int AC_huffsize[257];  
  int DC_huffcode[257];
  int AC_huffcode[257];
  HuffmanCodes codes;
  int DC_lastp = generate_huffsize(DC_bits, DC_huffsize);
  int AC_lastp = generate_huffsize(AC_bits, AC_huffsize);
  generate_huffcode(DC_huffsize, DC_huffcode);
  generate_huffcode(AC_huffsize, AC_huffcode);
  generate_ehufco(DC_vals, DC_huffcode, DC_huffsize, DC_lastp, codes.dc_ehufco, codes.dc_ehufsz);
  generate_ehufco(AC_vals, AC_huffcode, AC_huffsize, AC_lastp, codes.ac_ehufco, codes.ac_ehufsz);
  return codes;
}

void huffman_encode_block_proxy(HeapArray<int> obj, int color_idx, int last_val,
				Bits &bits, int *zigzag, const HuffmanCodes &codes) {
  huffman_encode_block(obj.base->data, color_idx, last_val, bits, zigzag, codes);
}

void huffman_encode_block(int *obj, int color_idx, int last_val,
			  Bits &bits, int *zigzag, const HuffmanCodes &codes) {
  // DC
  int base_lidx = color_idx * 8 * 8;
  int dc = obj[base_lidx];
  int temp = dc - last_val;
  int temp2 = temp;
  if (temp < 0) {
    temp = -temp;
    temp2--;
  }
  int nbits = 0;
  while (temp > 0) {
    nbits++;
    temp >>= 1;
  }
  bits.pack_and_stuff(codes.dc_ehufco[nbits], codes.dc_ehufsz[nbits], 0xFF, 0);
  if (nbits != 0) {
    bits.pack_and_stuff(temp2, nbits, 0xFF, 0);
  }
  // AC
  int run = 0;  
  for (int e = 1; e < 64; e++) {
    temp = obj[base_lidx+zigzag[e]];
    if (temp == 0)
      run++;
    else {
      while (run > 15) {
	bits.pack_and_stuff(codes.ac_ehufco[0xF0], codes.ac_ehufsz[0xF0], 0xFF, 0);
	run -= 16;
      }
      temp2 = temp;
      if (temp < 0) {
	temp = -temp;
	temp2--;
      }
      nbits = 1;
      temp >>= 1;
      while (temp > 0) {
	nbits++;
	temp >>= 1;
      }
      int i = (run << 4) + nbits;
      bits.pack_and_stuff(codes.ac_ehufco[i], codes.ac_ehufsz[i], 0xFF, 0);
      bits.pack_and_stuff(temp2, nbits, 0xFF, 0);
      run = 0;
    }
  }
  if (run > 0)
    bits.pack_and_stuff(codes.ac_ehufco[0], codes.ac_ehufsz[0], 0xFF, 0);
}

void pack_DC(Bits &bits, int idx, const HuffmanCodes &codes) {
  bits.pack_and_stuff(codes.dc_ehufco[idx], codes.dc_ehufsz[idx], 0xFF, 0);  
}

void pack_AC(Bits &bits, int idx, const HuffmanCodes &codes) {
  bits.pack_and_stuff(codes.ac_ehufco[idx], codes.ac_ehufsz[idx], 0xFF, 0);
}

void pack_and_stuff(Bits &bits, int val, int nbits) {
  bits.pack_and_stuff(val, nbits, 0xFF, 0);
}
