#include <iostream>
#include <fstream>
#include <sstream>

#include "builder/dyn_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "hmda_staged.h" 

using namespace std;
using namespace hmda;
using builder::dyn_var;

using dint = dyn_var<int>;
template <int Rank>
using loc = dyn_var<int[Rank]>;

void scale_quant(Block<int,2> quant, int quality) {
  int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
  Iter<'i'> i;
  Iter<'j'> j;
  quant[i][j] = (quant[i][j]*scale+50)/100;
}

template <typename RGB_T>
void color(RGB_T RGB, Block<int,3> YCbCr) {
  Iter<'i'> i;
  Iter<'j'> j;
  // TODO I need a cast operation
  YCbCr[0][i][j] = RGB[i][j][0]*0.299     + RGB[i][j][1]*0.587     + RGB[i][j][2]*0.114;
  YCbCr[1][i][j] = RGB[i][j][0]*-0.168736 + RGB[i][j][1]*-0.33126  + RGB[i][j][2]*0.500002 + 128;
  YCbCr[2][i][j] = RGB[i][j][0]*0.5       + RGB[i][j][1]*-0.418688 + RGB[i][j][2]*-0.081312 + 128;
}

void jpeg_staged(dyn_var<uint8_t*> input, dyn_var<int> H, dyn_var<int> W) {

  // Tables
  Block<int,2> luma_quant({8,8}, dyn_var<int[]>{
      16,11,10,16,24,40,51,61,
      12,12,14,19,26,58,60,55,
      14,13,16,24,40,57,69,56,
      14,17,22,29,51,87,80,62,
      18,22,37,56,68,109,103,77,
      24,35,55,64,81,104,113,92,
      49,64,78,87,103,121,120,101,
      72,92,95,98,112,100,103,99
    });

  Block<int,2> chroma_quant({8,8}, dyn_var<int[]>{
      17,18,24,47,99,99,99,99,
      18,21,26,66,99,99,99,99,
      24,26,56,99,99,99,99,99,
      47,66,99,99,99,99,99,99,
      99,99,99,99,99,99,99,99,
      99,99,99,99,99,99,99,99,
      99,99,99,99,99,99,99,99,
      99,99,99,99,99,99,99,99
    });

  Block<int,2> zigzag({8,8}, dyn_var<int[]>{
      0,  1,  8, 16, 9, 2, 3, 10, 
      17, 24, 32, 25, 18, 11, 4,
      5,  12, 19, 26, 33, 40, 48, 
      41, 34, 27, 20, 13, 6,  7, 
      14, 21, 28, 35, 42, 49, 56, 
      57, 50, 43, 36, 29, 22, 15, 
      23, 30, 37, 44, 51, 58, 59, 
      52, 45, 38, 31, 39, 46, 53, 
      60, 61, 54, 47, 55, 62, 63
    });

  // prep quant  
  scale_quant(luma_quant, 75);
  scale_quant(chroma_quant, 75);

  // start it up
  Block<uint8_t,3> RGB({H, W, 3}, input);
  dint last_Y = 0;
  dint last_Cb = 0;
  dint last_Cr = 0;

  Block<int,3> YCbCr(loc<3>{3,8,8});

  for (dint r = 0; r < H; r = r + 8) {
    for (dint c = 0; c < W; c = c + 8) {
      auto mcu = RGB.view(slice(r,r+8,1),slice(c,c+8,1),slice(0,3,1));
      if (r+8>H || c+8>W) {
	// need padding
	dint row_pad = 0;
	dint col_pad = 0;
	if (r+8>H)
	  row_pad = 8 - (H%8);
	if (c+8>W)
	  col_pad = 8 - (W%8);
	// col major still
	Block<uint8_t,3> padded(loc<3>{8,8,3});
	dint last_valid_row = 8 - row_pad;
	dint last_valid_col = 8 - col_pad;
	// copy over original
	Iter<'x'> x;
	Iter<'y'> y;
	Iter<'z'> z;
	auto orig_padded = padded.view(slice(0,last_valid_row,1),slice(0,last_valid_col,1),slice(0,3,1));
	orig_padded[x][y][z] = RGB[x+r][y+c][z];
	// pad
	auto row_padding_area = padded.view(slice(last_valid_row,row_pad,1),slice(0,8,1),slice(0,3,1));
	auto col_padding_area = padded.view(slice(0,8,1), slice(last_valid_col,col_pad,1), slice(0,3,1));
	// TODO things are not happy with builder::builder, so I need to force a cast here to dyn_var
	// the builder ends up creating a variable decl with no definition. Trying to do within the 
	// realize function doesn't work
	row_padding_area[x][y][z] = padded[(dint)(last_valid_row-1)][y][z];
	col_padding_area[x][y][z] = padded[x][(dint)(last_valid_col-1)][z];
	color(padded, YCbCr);
      } else {
	// no padding needed
	color(mcu, YCbCr);	
      }
    }
  }

}

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage: ./sjpeg <output_fn>" << endl;
    exit(-1);
  }
  builder::builder_context context;
  auto ast = context.extract_function_ast(jpeg_staged, "jpeg");
  block::eliminate_redundant_vars(ast);	
  ofstream src;
  ofstream hdr;
  string source = string(argv[1]) + ".cpp";
  string header = string(argv[1]) + ".h";
  src.open(source);
  hdr.open(header);
  hdr << "#pragma once" << endl;
  hdr << "#include <cmath>" << endl;
  hdr << "void jpeg(uint8_t *input, int H, int W);" << endl;
  hdr.flush();
  hdr.close();
  src << "#include \"" << header << "\"" << endl;
  stringstream ss;
  block::c_code_generator::generate_code(ast, ss, 0);
  cout << ss.str() << endl;
  src << ss.str() << endl;
  src.flush();
  src.close();
}
