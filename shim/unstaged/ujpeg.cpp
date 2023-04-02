#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

// clang++ -I../shim/unstaged -std=c++17 -I../shim/runtime/cpp -I../shim  -I. ../shim/unstaged/syntax.cpp ../shim/unstaged/bits.cpp ../shim/unstaged/huffman.cpp ../shim/unstaged/ujpeg.cpp -O3 -flto -o ujpeg

#include "unite.h"

#include "huffman.h"
#include "bits.h"
#include "syntax.h"

using namespace std;
using namespace shim;
using namespace unstaged;

template <typename Obj>
void scale_quant(Obj &quant, int quality) {
  int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      quant.write(std::array{i,j}, (quant.read(i,j)*scale+50)/100);
    }
  }
}

template <typename RGB_T, typename YCbCr_T>
void color(RGB_T &RGB, YCbCr_T &YCbCr) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      YCbCr.write(std::array{0,i,j}, 
		  (int)((double)(RGB.read(i,j,0))*0.299 + 
			(double)(RGB.read(i,j,1))*0.587 + 
			(double)(RGB.read(i,j,2))*0.114));
      YCbCr.write(std::array{1,i,j}, 
		  (int)((double)(RGB.read(i,j,0))*-0.168736 + 
			(double)(RGB.read(i,j,1))*-0.33126 + 
			(double)(RGB.read(i,j,2))*0.500002) + 128);
      YCbCr.write(std::array{2,i,j}, 
		  (int)((double)(RGB.read(i,j,0))*0.5 + 
			(double)(RGB.read(i,j,1))*-0.418688 + 
			(double)(RGB.read(i,j,2))*-0.081312) + 128);
    }
  }
}

// the external things to call for doing huffman
// these are completely the wrong types but w/e

template <typename Obj>
void dct(Obj &obj) { 
#define descale(x,n) ((x + ((1 << (n-1)))) >> n)
  int FIX_0_298631336 = 2446;
  int FIX_0_390180644 = 3196;
  int FIX_0_541196100 = 4433;
  int FIX_0_765366865 = 6270;
  int FIX_0_899976223 = 7373;
  int FIX_1_175875602 = 9633;
  int FIX_1_501321110 = 12299;
  int FIX_1_847759065 = 15137;
  int FIX_1_961570560 = 16069;
  int FIX_2_053119869 = 16819;
  int FIX_2_562915447 = 20995;
  int FIX_3_072711026 = 25172;
  
  for (int r = 0; r < 8; r=r+1) {
    auto row = obj.partition(std::array{0,1,1},
			     std::array{r,r+1,1},
			     std::array{0,8,1});
    int tmp0 = row.read(0,0,0) + row.read(0,0,7);
    int tmp7 = row.read(0,0,0) - row.read(0,0,7);
    int tmp1 = row.read(0,0,1) + row.read(0,0,6);
    int tmp6 = row.read(0,0,1) - row.read(0,0,6);
    int tmp2 = row.read(0,0,2) + row.read(0,0,5);
    int tmp5 = row.read(0,0,2) - row.read(0,0,5);
    int tmp3 = row.read(0,0,3) + row.read(0,0,4);
    int tmp4 = row.read(0,0,3) - row.read(0,0,4);
    int tmp10 = tmp0 + tmp3;
    int tmp13 = tmp0 - tmp3;
    int tmp11 = tmp1 + tmp2;
    int tmp12 = tmp1 - tmp2;
    row.write(std::array{0,0,0}, (tmp10 + tmp11) << 2);
    row.write(std::array{0,0,4}, (tmp10 - tmp11) << 2);
    int z1 = (tmp12 + tmp13) * FIX_0_541196100;
    row.write(std::array{0,0,2}, descale(z1 + tmp13 * FIX_0_765366865, 11));
    row.write(std::array{0,0,6}, descale(z1 + tmp12 * -FIX_1_847759065, 11));
    z1 = tmp4 + tmp7;
    int z2 = tmp5 + tmp6;
    int z3 = tmp4 + tmp6;
    int z4 = tmp5 + tmp7;
    int z5 = (z3 + z4) * FIX_1_175875602;
    tmp4 = tmp4 * FIX_0_298631336;
    tmp5 = tmp5 * FIX_2_053119869;
    tmp6 = tmp6 * FIX_3_072711026;
    tmp7 = tmp7 * FIX_1_501321110;
    z1 = z1 * -FIX_0_899976223;
    z2 = z2 * -FIX_2_562915447;
    z3 = z3 * -FIX_1_961570560;
    z4 = z4 * -FIX_0_390180644;
    z3 = z3 + z5; 
    z4 = z4 + z5;
    row.write(std::array{0,0,7}, descale(tmp4 + z1 + z3, 11));
    row.write(std::array{0,0,5}, descale(tmp5 + z2 + z4, 11));
    row.write(std::array{0,0,3}, descale(tmp6 + z2 + z3, 11));
    row.write(std::array{0,0,1}, descale(tmp7 + z1 + z4, 11));
  }
  for (int c = 0; c < 8; c=c+1) {
    auto col = obj.partition(std::array{0,1,1}, 
			     std::array{0,8,1}, 
			     std::array{c,c+1,1});    
    int tmp0 = col.read(0,0,0) + col.read(0,7,0);
    int tmp7 = col.read(0,0,0) - col.read(0,7,0);
    int tmp1 = col.read(0,1,0) + col.read(0,6,0);
    int tmp6 = col.read(0,1,0) - col.read(0,6,0);
    int tmp2 = col.read(0,2,0) + col.read(0,5,0);
    int tmp5 = col.read(0,2,0) - col.read(0,5,0);
    int tmp3 = col.read(0,3,0) + col.read(0,4,0);
    int tmp4 = col.read(0,3,0) - col.read(0,4,0);
    int tmp10 = tmp0 + tmp3;
    int tmp13 = tmp0 - tmp3;
    int tmp11 = tmp1 + tmp2;
    int tmp12 = tmp1 - tmp2;
    col.write(std::array{0,0,0}, descale(tmp10 + tmp11, 2));
    col.write(std::array{0,4,0}, descale(tmp10 - tmp11, 2));
    int z1 = (tmp12 + tmp13) * FIX_0_541196100;
    col.write(std::array{0,2,0}, descale(z1 + tmp13 * FIX_0_765366865, 15));
    col.write(std::array{0,6,0}, descale(z1 + tmp12 * -FIX_1_847759065, 15));
    z1 = tmp4 + tmp7;
    int z2 = tmp5 + tmp6;
    int z3 = tmp4 + tmp6;
    int z4 = tmp5 + tmp7;
    int z5 = (z3 + z4) * FIX_1_175875602;
    tmp4 = tmp4 * FIX_0_298631336;
    tmp5 = tmp5 * FIX_2_053119869;
    tmp6 = tmp6 * FIX_3_072711026;
    tmp7 = tmp7 * FIX_1_501321110;
    z1 = z1 * -FIX_0_899976223;
    z2 = z2 * -FIX_2_562915447;
    z3 = z3 * -FIX_1_961570560;
    z4 = z4 * -FIX_0_390180644;
    z3 = z3 + z5;
    z4 = z4 + z5;
    col.write(std::array{0,7,0}, descale(tmp4 + z1 + z3, 15));
    col.write(std::array{0,5,0}, descale(tmp5 + z2 + z4, 15));
    col.write(std::array{0,3,0}, descale(tmp6 + z2 + z3, 15));
    col.write(std::array{0,1,0}, descale(tmp7 + z1 + z4, 15));
  }
}

template <typename In, typename Out>
void quant(In &obj, Out &quant) {
  for (int i = 0; i < 8; i=i+1) {
    for (int j = 0; j < 8; j=j+1) {
      int v = obj.read(0,i,j);
      int q = quant.read(i,j) * 8;
      int mult = v < 0 ? -1 : 1;
      v = v * mult;
      v = v + (q >> 1);
      if (v >= q) {
	v = v / q;
	v = v * mult;
      } else {
	v = 0;
      }
      obj.write(std::array{0,i,j}, v);
    }
  }
}

void jpeg(uint8_t* input, int H, int W, 
	  int *luma_quant_arr, 
	  int *chroma_quant_arr,
	  int *zigzag, 
	  HuffmanCodes &luma_codes, 
	  HuffmanCodes &chroma_codes,
	  Bits &bits) {

  // Tables (these are already scaled)
  auto luma_quant = Block<int,2,int*>(TensorBuilder<2>().with_extents(std::array{8,8}).to_tensor(),
				      luma_quant_arr);
  auto chroma_quant = Block<int,2,int*>(TensorBuilder<2>().with_extents(std::array{8,8}).to_tensor(),
					chroma_quant_arr);
    
  // start it up
  auto RGB = Block<uint8_t,3,uint8_t*>(TensorBuilder<3>().with_extents(std::array{H,W,3}).to_tensor(),
				       input);
  int last_Y = 0;
  int last_Cb = 0;
  int last_Cr = 0;
  
  int _YCbCr[192];
  int _padded[192];
  for (int r = 0; r < H; r = r + 8) {
    for (int c = 0; c < W; c = c + 8) {
      auto mcu = RGB.partition(std::array{r,r+8,1},
			       std::array{c,c+8,1},
			       std::array{0,3,1});
      auto YCbCr = mcu.ppermute<int>(std::array{2,0,1}, _YCbCr);
      if (r+8>H || c+8>W) {
	auto padded = mcu.to_block(_padded);
	// need padding
	int row_pad = 0;
	int col_pad = 0;
	if (r+8>H)
	  row_pad = 8 - (H%8);
	if (c+8>W)
	  col_pad = 8 - (W%8);
	// col major still
	int last_valid_row = 8 - row_pad;
	int last_valid_col = 8 - col_pad;
	// copy over original
	auto orig_padded = padded.partition(std::array{0,last_valid_row,1},
					    std::array{0,last_valid_col,1},
					    std::array{0,3,1});
	for (int i = 0; i < last_valid_row; i++) {
	  for (int j = 0; j < last_valid_col; j++) {
	    for (int k = 0; k < 3; k++) {
	      orig_padded.write(std::array{i,j,k}, RGB.read(i+r,j+c,k));
	    }
	  }
	}
	// pad
	auto row_padding_area = padded.partition(std::array{last_valid_row,last_valid_row+row_pad,1},
						 std::array{0,8,1},
						 std::array{0,3,1});
	auto col_padding_area = padded.partition(std::array{0,8,1}, 
						 std::array{last_valid_col,last_valid_col+col_pad,1}, 
						 std::array{0,3,1});

	for (int i = 0; i < row_pad; i++) {
	  for (int j = 0; j < 8; j++) {
	    for (int k = 0; k < 3; k++) {
	      row_padding_area.write(std::array{i,j,k}, padded.read((last_valid_row-1),j,k));
	    }
	  }
	}
	for (int i = 0; i < 8; i++) {
	  for (int j = 0; j < col_pad; j++) {
	    for (int k = 0; k < 3; k++) {
	      col_padding_area.write(std::array{i,j,k}, padded.read(i,(last_valid_col-1),k));
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
	    YCbCr.write(std::array{i,j,k}, YCbCr.read(i,j,k) - 128);
	  }
	}
      }
      auto Y = YCbCr.partition(std::array{0,1,1},std::array{0,8,1},std::array{0,8,1});
      auto Cb = YCbCr.partition(std::array{1,2,1},std::array{0,8,1},std::array{0,8,1});
      auto Cr = YCbCr.partition(std::array{2,3,1},std::array{0,8,1},std::array{0,8,1});
      dct(Y); 
      dct(Cb);
      dct(Cr);
      quant(Y, luma_quant);
      quant(Cb, chroma_quant);
      quant(Cr, chroma_quant);
      // get the base lidx for each
      huffman_encode_block(_YCbCr, 0, last_Y, bits, zigzag, luma_codes);
      huffman_encode_block(_YCbCr, 1, last_Cb, bits, zigzag, chroma_codes);
      huffman_encode_block(_YCbCr, 2, last_Cr, bits, zigzag, chroma_codes);
      last_Y = Y.read(0,0,0);
      last_Cb = Cb.read(0,0,0);
      last_Cr = Cr.read(0,0,0);
    }
  }
}

int zigzag[] = {
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

int luma_quant[] = {
  16,11,10,16,24,40,51,61,
  12,12,14,19,26,58,60,55,
  14,13,16,24,40,57,69,56,
  14,17,22,29,51,87,80,62,
  18,22,37,56,68,109,103,77,
  24,35,55,64,81,104,113,92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103,99
};

int chroma_quant[] = {
  17,18,24,47,99,99,99,99,
  18,21,26,66,99,99,99,99,
  24,26,56,99,99,99,99,99,
  47,66,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99};

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
  size_t read = fread(image, 1, sz, fd);
  assert(read == sz);
}

void scale_quant(int quant[], int quality) {
  int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      quant[i*8+j] = (quant[i*8+j]*scale+50)/100;
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    cerr << "Usage: ./ujpeg <ppm> <jpg>" << endl;
    exit(-1);
  }
  std::cerr << "Running UNSTAGED jpeg" << std::endl;
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  // prep bits
  FILE *jpg = fopen(argv[2], "wb");
  Bits bits(jpg);
  // prep huffman encoder
  HuffmanCodes luma_codes = generate_codes(luma_DC_huffbits, luma_DC_huffvals, luma_AC_huffbits, luma_AC_huffvals);
  HuffmanCodes chroma_codes = generate_codes(chroma_DC_huffbits, chroma_DC_huffvals, chroma_AC_huffbits, chroma_AC_huffvals); 

  string ppm = argv[1];
  FILE *ifd = fopen(ppm.c_str(), "r");
  int H, W, max_val;
  read_ppm_header(ifd, H, W, max_val);
  uint8_t *RGB = new uint8_t[H*W*3];
  read_ppm_body(ifd, RGB, H, W);
  fclose(ifd);

  // prep quant
  scale_quant(luma_quant, 75);
  scale_quant(chroma_quant, 75);

  // write bitstream headers
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

  // staged code
  jpeg(RGB, H, W, luma_quant, chroma_quant, zigzag, luma_codes, chroma_codes, bits);
  bits.complete_byte_and_stuff(1, 0xFF, 0);
  syntax_EOI(bits);
  bits.flush_bits();
  fflush(jpg);
  fclose(jpg);
  std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
  std::cout << "Program took " << std::chrono::duration_cast<std::chrono::nanoseconds> (stop - start).count()/1e9 << "s" << std::endl;
}
