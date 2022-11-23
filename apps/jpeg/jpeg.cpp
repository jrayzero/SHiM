#include "hmda.h"
#include "bits.h"
#include "syntax.h"
#include "huffman.h"

// This uses just the raw BLock/View unstaged API. It does not use
// any expression template features.

using namespace std;
using namespace hmda;

int _luma_quant[] = {
  16,11,10,16,24,40,51,61,
  12,12,14,19,26,58,60,55,
  14,13,16,24,40,57,69,56,
  14,17,22,29,51,87,80,62,
  18,22,37,56,68,109,103,77,
  24,35,55,64,81,104,113,92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103,99
};

int _chroma_quant[] = {
  17,18,24,47,99,99,99,99,
  18,21,26,66,99,99,99,99,
  24,26,56,99,99,99,99,99,
  47,66,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99};

int _zigzag[] = {
  0,  1,  8, 16, 9, 2, 3, 10, 
  17, 24, 32, 25, 18, 11, 4,
  5,  12, 19, 26, 33, 40, 48, 
  41, 34, 27, 20, 13, 6,  7, 
  14, 21, 28, 35, 42, 49, 56, 
  57, 50, 43, 36, 29, 22, 15, 
  23, 30, 37, 44, 51, 58, 59, 
  52, 45, 38, 31, 39, 46, 53, 
  60, 61, 54, 47, 55, 62, 63
};

void scale_quant(Block<int,2> &quant, int quality) {
  int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      quant(i,j) = (quant(i,j)*scale+50)/100;
    }
  }
}

template <typename RGB_T>
void color(const RGB_T &RGB, Block<int,3> &YCbCr) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      YCbCr(0,i,j) = int(RGB(i,j,0)*0.299     + RGB(i,j,1)*0.587     + RGB(i,j,2)*0.114);
      YCbCr(1,i,j) = int(RGB(i,j,0)*-0.168736 + RGB(i,j,1)*-0.33126  + RGB(i,j,2)*0.500002) + 128;
      YCbCr(2,i,j) = int(RGB(i,j,0)*0.5       + RGB(i,j,1)*-0.418688 + RGB(i,j,2)*-0.081312) + 128;
    }
  }
}

#define descale(x,n) (x + (1 << (n-1))) >> n
#define FIX_0_298631336 2446
#define FIX_0_390180644 3196
#define FIX_0_541196100 4433
#define FIX_0_765366865 6270
#define FIX_0_899976223 7373
#define FIX_1_175875602 9633
#define FIX_1_501321110 12299
#define FIX_1_847759065 15137
#define FIX_1_961570560 16069
#define FIX_2_053119869 16819
#define FIX_2_562915447 20995
#define FIX_3_072711026 25172

// potential for parallelization here across rows/columns (basically breaking the 
// whole thing into a grid)
void dct(View<int,3> &obj) {
  for (int64_t r = 0; r < 8; r++) {
    auto row = obj.view(slice(0,1,1),slice(r,r+1,1),slice(0,8,1));
    int64_t tmp0 = row(0) + row(7);
    int64_t tmp7 = row(0) - row(7);
    int64_t tmp1 = row(1) + row(6);
    int64_t tmp6 = row(1) - row(6);
    int64_t tmp2 = row(2) + row(5);
    int64_t tmp5 = row(2) - row(5);
    int64_t tmp3 = row(3) + row(4);
    int64_t tmp4 = row(3) - row(4);
    int64_t tmp10 = tmp0 + tmp3;
    int64_t tmp13 = tmp0 - tmp3;
    int64_t tmp11 = tmp1 + tmp2;
    int64_t tmp12 = tmp1 - tmp2;
    row(0) = (tmp10 + tmp11) << 2;
    row(4) = (tmp10 - tmp11) << 2;
    int64_t z1 = (tmp12 + tmp13) * FIX_0_541196100;
    row(2) = descale(z1 + tmp13 * FIX_0_765366865, 11);
    row(6) = descale(z1 + tmp12 * -FIX_1_847759065, 11);
    z1 = tmp4 + tmp7;
    int64_t z2 = tmp5 + tmp6;
    int64_t z3 = tmp4 + tmp6;
    int64_t z4 = tmp5 + tmp7;
    int64_t z5 = (z3 + z4) * FIX_1_175875602;
    tmp4 = tmp4 * FIX_0_298631336;
    tmp5 = tmp5 * FIX_2_053119869;
    tmp6 = tmp6 * FIX_3_072711026;
    tmp7 = tmp7 * FIX_1_501321110;
    z1 = z1 * -FIX_0_899976223;
    z2 = z2 * -FIX_2_562915447;
    z3 = z3 * -FIX_1_961570560;
    z4 = z4 * -FIX_0_390180644;
    z3 += z5;
    z4 += z5;
    row(7) = descale(tmp4 + z1 + z3, 11);
    row(5) = descale(tmp5 + z2 + z4, 11);
    row(3) = descale(tmp6 + z2 + z3, 11);
    row(1) = descale(tmp7 + z1 + z4, 11);
  }
  for (int64_t c = 0; c < 8; c++) {
    auto col = obj.view(slice(0,1,1), slice(0,8,1), slice(c,c+1,1));    
    int64_t tmp0 = col(0,0) + col(7,0);
    int64_t tmp7 = col(0,0) - col(7,0);
    int64_t tmp1 = col(1,0) + col(6,0);
    int64_t tmp6 = col(1,0) - col(6,0);
    int64_t tmp2 = col(2,0) + col(5,0);
    int64_t tmp5 = col(2,0) - col(5,0);
    int64_t tmp3 = col(3,0) + col(4,0);
    int64_t tmp4 = col(3,0) - col(4,0);
    int64_t tmp10 = tmp0 + tmp3;
    int64_t tmp13 = tmp0 - tmp3;
    int64_t tmp11 = tmp1 + tmp2;
    int64_t tmp12 = tmp1 - tmp2;
    col(0,0) = descale(tmp10 + tmp11, 2);
    col(4,0) = descale(tmp10 - tmp11, 2);
    int64_t z1 = (tmp12 + tmp13) * FIX_0_541196100;
    col(2,0) = descale(z1 + tmp13 * FIX_0_765366865, 15);
    col(6,0) = descale(z1 + tmp12 * -FIX_1_847759065, 15);
    z1 = tmp4 + tmp7;
    int64_t z2 = tmp5 + tmp6;
    int64_t z3 = tmp4 + tmp6;
    int64_t z4 = tmp5 + tmp7;
    int64_t z5 = (z3 + z4) * FIX_1_175875602;
    tmp4 = tmp4 * FIX_0_298631336;
    tmp5 = tmp5 * FIX_2_053119869;
    tmp6 = tmp6 * FIX_3_072711026;
    tmp7 = tmp7 * FIX_1_501321110;
    z1 = z1 * -FIX_0_899976223;
    z2 = z2 * -FIX_2_562915447;
    z3 = z3 * -FIX_1_961570560;
    z4 = z4 * -FIX_0_390180644;
    z3 += z5;
    z4 += z5;
    col(7,0) = descale(tmp4 + z1 + z3, 15);
    col(5,0) = descale(tmp5 + z2 + z4, 15);
    col(3,0) = descale(tmp6 + z2 + z3, 15);
    col(1,0) = descale(tmp7 + z1 + z4, 15);
  }
}

void quant(View<int,3> &obj, const Block<int,2> &quant) {
  for (int64_t i = 0; i < 8; i++) {
    for (int64_t j = 0; j < 8; j++) {
      int64_t v = obj(0,i,j);
      int64_t q = quant(i,j) * 8;
      if (v < 0) {
	v = -v;
	v += (q >> 1);
	v /= q;
	v = -v;
      } else {
	v += (q >> 1);
	v /= q;
      }
      obj(0,i,j) = v;
    }
  }
}

void read_ppm_header(FILE *fd, int &H, int &W, int &max_val) {
  char *buff = NULL;
  size_t n;
  ssize_t nbytes = getline(&buff, &n, fd);
  assert(buff[0] == 'P' && buff[1] == '6');
  nbytes = getline(&buff, &n, fd);
  char delim[] = " ";
  char *split = strtok(buff, delim);
  W = atoi(split);
  split = strtok(NULL, delim);  
  H = atoi(split);
  nbytes = getline(&buff, &n, fd);
  max_val = atoi(buff);
  free(buff);
}

void read_ppm_body(FILE *fd, uint8_t *image, int H, int W) {
  int sz = 3 * H * W;
  fread(image, 1, sz, fd); 
}

int main(int argc, char **argv) {
  if (argc != 3) {
    cerr << "Usage: ./jpeg <ppm> <jpg>" << endl;
    exit(-1);
  }

  // prep quant  
  Block<int,2> luma_quant({8,8}, _luma_quant);
  Block<int,2> chroma_quant({8,8}, _chroma_quant);
  scale_quant(luma_quant, 75);
  scale_quant(chroma_quant, 75);

  // prep bits
  FILE *jpg = fopen(argv[2], "wb");
  Bits bits(jpg);

  // prep huffman encoder
  Block<int,2> zigzag({8,8}, _zigzag);
  HuffmanCodes luma_codes = generate_codes(luma_DC_huffbits, luma_DC_huffvals, luma_AC_huffbits, luma_AC_huffvals);
  HuffmanCodes chroma_codes = generate_codes(chroma_DC_huffbits, chroma_DC_huffvals, chroma_AC_huffbits, chroma_AC_huffvals);  

  // read ppm
  string ppm = argv[1];
  FILE *ifd = fopen(ppm.c_str(), "r");
  int H, W, max_val;
  read_ppm_header(ifd, H, W, max_val);
  cout << "H " << H << endl;
  cout << "W " << W << endl;
  auto RGB = Block<uint8_t,3>({H, W, 3});
  read_ppm_body(ifd, RGB, H, W);
  fclose(ifd);
  int last_Y = 0;
  int last_Cb = 0;
  int last_Cr = 0;

  // start bits
  syntax_SOI(bits);
  syntax_JFIF(bits);
  syntax_quant_table(bits, luma_quant, zigzag, true);
  syntax_quant_table(bits, chroma_quant, zigzag, false);
  syntax_frame_header(bits, H, W);    
  syntax_huffman_table(bits,
		       luma_DC_huffbits, 17,
		       luma_DC_huffvals, 12,
		       true, 0);
  syntax_huffman_table(bits,
		       luma_AC_huffbits, 17,
		       luma_AC_huffvals, 162,
		       false, 0);
  syntax_huffman_table(bits,
		       chroma_DC_huffbits, 17,
		       chroma_DC_huffvals, 12,
		       true, 1);
  syntax_huffman_table(bits,
		       chroma_AC_huffbits, 17,
		       chroma_AC_huffvals, 162,
		       false, 1);
  syntax_scan_header(bits);
  
  auto YCbCr = Block<int,3>({3,8,8}); // don't make from mcu b/c different extents
  // break into an mcu grid
  for (int r = 0; r < H; r+=8) {
    for (int c = 0; c < W; c+=8) {
      auto mcu = RGB.view(slice(r,r+8,1),slice(c,c+8,1),slice(0,3,1));
      if (r+8>H || c+8>W) {
	// need padding
	int row_pad = 0;
	int col_pad = 0;
	if (r+8>H)
	  row_pad = 8 - (H%8);
	if (c+8>W)
	  col_pad = 8 - (W%8);
	// col major still
	Block<uint8_t,3> padded({8,8,3});
	int last_valid_row = 8 - row_pad;
	int last_valid_col = 8 - col_pad;
	// copy over original
	for (int x = 0; x < last_valid_row; x++) {
	  for (int y = 0; y < last_valid_col; y++) {
	    for (int z = 0; z < 3; z++) {
	      padded(x,y,z) = RGB(r+x,c+y,z);
	    }
	  }
	}
	// pad
	for (int x = 0; x < row_pad; x++) {
	  for (int y = 0; y < 8; y++) {
	    for (int z = 0; z < 3; z++) {
	      padded(x+last_valid_row,y,z) = padded(last_valid_row-1,y,z);
	    }
	  }
	}
	for (int x = 0; x < 8; x++) {
	  for (int y = 0; y < col_pad; y++) {
	    for (int z = 0; z < 3; z++) {
	      padded(x,y+last_valid_col,z) = padded(x,last_valid_col-1, z);
	    }
	  }
	}	
	color(padded, YCbCr);
      } else {
	// no padding needed
	color(mcu, YCbCr);	
      }
      // offset
      for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 8; j++) {
	  for (int k = 0; k < 8; k++) {
	    YCbCr(i,j,k) -= 128;
	  }
	}
      }
      auto Y = YCbCr.view(slice(0,1,1),slice(0,8,1),slice(0,8,1));
      auto Cb = YCbCr.view(slice(1,2,1),slice(0,8,1),slice(0,8,1));
      auto Cr = YCbCr.view(slice(2,3,1),slice(0,8,1),slice(0,8,1));
      dct(Y);
      dct(Cb);
      dct(Cr);
      quant(Y, luma_quant);
      quant(Cb, chroma_quant);
      quant(Cr, chroma_quant);
      huffman_encode_block(Y, last_Y, bits, zigzag, luma_codes);
      huffman_encode_block(Cb, last_Cb, bits, zigzag, chroma_codes);
      huffman_encode_block(Cr, last_Cr, bits, zigzag, chroma_codes);
      last_Y = Y(0);
      last_Cb = Cb(0);
      last_Cr = Cr(0);
    }
  }
  bits.complete_byte_and_stuff(1, 0xFF, 0);
  syntax_EOI(bits);
  bits.flush_bits();
  fflush(jpg);
  fclose(jpg);
  
}
