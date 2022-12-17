#include <iostream>
#include <chrono>
#include <cstring>
#include "sjpeg_v1.h"
#include "bits.h"
#include "syntax.h"

using namespace std;

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
  fread(image, 1, sz, fd); 
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
    cerr << "Usage: ./jpeg <ppm> <jpg>" << endl;
    exit(-1);
  }

  // prep bits
  FILE *jpg = fopen(argv[2], "wb");
  Bits bits(jpg);

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
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
