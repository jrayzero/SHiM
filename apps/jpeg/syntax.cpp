#include "syntax.h"

constexpr int SOF0 = 0xFFC0;
constexpr int SOF1 = 0xFFC1;
constexpr int SOF2 = 0xFFC2;
constexpr int SOF3 = 0xFFC3;
constexpr int SOF5 = 0xFFC5;
constexpr int SOF6 = 0xFFC6;
constexpr int SOF7 = 0xFFC7;
constexpr int JPG = 0xFFC8;
constexpr int SOF9 = 0xFFC9;
constexpr int SOF10 = 0xFFCA;
constexpr int SOF11 = 0xFFCB;
constexpr int SOF13 = 0xFFCD;
constexpr int SOF14 = 0xFFCE;
constexpr int SOF15 = 0xFFCF;
constexpr int DHT = 0xFFC4;
constexpr int DAC = 0xFFCC;
constexpr int RST0 = 0xFFD0;
constexpr int RST1 = 0xFFD1;
constexpr int RST2 = 0xFFD2;
constexpr int RST3 = 0xFFD3;
constexpr int RST4 = 0xFFD4;
constexpr int RST5 = 0xFFD5;
constexpr int RST6 = 0xFFD6;
constexpr int RST7 = 0xFFD7;
constexpr int SOI = 0xFFD8;
constexpr int EOI = 0xFFD9;
constexpr int SOS = 0xFFDA;
constexpr int DQT = 0xFFDB;
constexpr int DNL = 0xFFDC;
constexpr int DRI = 0xFFDD;
constexpr int DHP = 0xFFDE;
constexpr int EXP = 0xFFDF;
constexpr int APP0 = 0xFFE0;
constexpr int APP1 = 0xFFE1;
constexpr int APP2 = 0xFFE2;
constexpr int APP3 = 0xFFE3;
constexpr int APP4 = 0xFFE4;
constexpr int APP5 = 0xFFE5;
constexpr int APP6 = 0xFFE6;
constexpr int APP7 = 0xFFE7;
constexpr int APP8 = 0xFFE8;
constexpr int APP9 = 0xFFE9;
constexpr int APP10 = 0xFFEA;
constexpr int APP11 = 0xFFEB;
constexpr int APP12 = 0xFFEC;
constexpr int APP13 = 0xFFED;
constexpr int APP14 = 0xFFEE;
constexpr int APP15 = 0xFFEF;
constexpr int JPG0 = 0xFFF0;
constexpr int JPG1 = 0xFFF1;
constexpr int JPG2 = 0xFFF2;
constexpr int JPG3 = 0xFFF3;
constexpr int JPG4 = 0xFFF4;
constexpr int JPG5 = 0xFFF5;
constexpr int JPG6 = 0xFFF6;
constexpr int JPG7 = 0xFFF7;
constexpr int JPG8 = 0xFFF8;
constexpr int JPG9 = 0xFFF9;
constexpr int JPG10 = 0xFFFA;
constexpr int JPG11 = 0xFFFB;
constexpr int JPG12 = 0xFFFC;
constexpr int JPG13 = 0xFFFD;
constexpr int JPG14 = 0xFFFE;
constexpr int JPG15 = 0xFF01;

void syntax_SOI(Bits &bits) {
  bits.pack(SOI, 16);
}

void syntax_EOI(Bits &bits) {
  bits.pack(EOI, 16);
}

void syntax_JFIF(Bits &bits) {
  bits.pack(APP0, 16);
  bits.pack(16, 16);
  bits.pack(0x4A46494600, 40);
  bits.pack(0x0101, 16);
  bits.pack(0x00, 8);
  bits.pack(0x01, 16);
  bits.pack(0x01, 16);
  bits.pack(0x00, 8);
  bits.pack(0x00, 8);
}

void syntax_frame_header(Bits &bits, int H, int W) {
  bits.pack(SOF0, 16);
  int Nf = 3;
  int Lf = 8 + 3 * Nf;
  int Hs = 1;
  int Vs = 1;
  bits.pack(Lf, 16);
  bits.pack(8, 8);
  bits.pack(H, 16);
  bits.pack(W, 16);
  bits.pack(Nf, 8);
  bits.pack(1, 8);
  bits.pack(Hs, 4);
  bits.pack(Vs, 4);
  bits.pack(0, 8);
  bits.pack(2, 8);
  bits.pack(Hs, 4);
  bits.pack(Vs, 4);
  bits.pack(1, 8);
  bits.pack(3, 8);
  bits.pack(Hs, 4);
  bits.pack(Vs, 4);
  bits.pack(1, 8);
}

void syntax_scan_header(Bits &bits) {
  bits.pack(SOS, 16);
  int Ns = 3;
  int Ls = 6 + 2 * Ns;
  bits.pack(Ls, 16);
  bits.pack(Ns, 8);
  bits.pack(1, 8); 
  bits.pack(0, 4);
  bits.pack(0, 4);
  bits.pack(2, 8); 
  bits.pack(1, 4);
  bits.pack(1, 4);
  bits.pack(3, 8); 
  bits.pack(1, 4);
  bits.pack(1, 4);        
  bits.pack(0, 8);
  bits.pack(63, 8);
  bits.pack(0, 4);
  bits.pack(0, 4);
}

void syntax_quant_table(Bits &bits, const Block<int,2> &quant, 
			const Block<int,2> &zigzag, bool is_luma) {
  int Lq = 67;
  int Pq = 0;
  bits.pack(DQT, 16);
  bits.pack(Lq, 16);
  bits.pack(Pq, 4);
  bits.pack(is_luma ? 0 : 1, 4);
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      int q = quant(zigzag(i,j));
      bits.pack(q, 8);
    }
  }
}

void syntax_huffman_table(Bits &bits, int *huffbits, int huffbits_len, int *huffvals, int huffvals_len,
			  bool is_dc, int ident) {
  int Lh = 2 + huffbits_len + huffvals_len;
  bits.pack(DHT, 16);
  bits.pack(Lh, 16);
  bits.pack(is_dc ? 0 : 1, 4);
  bits.pack(ident, 4);
  for (int i = 1; i < huffbits_len; i++) {
    bits.pack(huffbits[i], 8);
  }
  for (int i = 0; i < huffvals_len; i++) {
    bits.pack(huffvals[i], 8);
  }  
}
