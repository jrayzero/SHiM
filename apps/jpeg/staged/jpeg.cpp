#include "builder/dyn_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "hmda_staged.h" 

using namespace std;
using namespace hmda;
using builder::dyn_var;

using dint = dyn_var<int>;

void scale_quant(Block<int,2> quant, int quality) {
  int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
  Iter<'i'> i;
  Iter<'j'> j;
  quant[i][j] = (quant[i][j]*scale+50)/100;
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

  // TODO Ugh, fix whatever type inference issues I have. Try getting rid of Wrapped and replacing with tuple
  Block<int,3> YCbCr(Wrapped<3,loop_type>{3,8,8});

  for (dint r = 0; r < H; r = r + 8) {
    for (dint c = 0; c < W; c = c + 8) {
      // TODO need to implement .view(slices) on just block. Seemed to lose it at some point...
      auto mcu = RGB.view().view(slice(r,r+8,1),slice(c,c+8,1),slice(0,3,1));
      if (r+8>H || c+8>W) {
	// need padding
	dint row_pad = 0;
	dint col_pad = 0;
	if (r+8>H)
	  row_pad = 8 - (H%8);
	if (c+8>W)
	  col_pad = 8 - (W%8);
	// col major still
	Block<uint8_t,3> padded(Wrapped<3,loop_type>{8,8,3});
	dint last_valid_row = 8 - row_pad;
	dint last_valid_col = 8 - col_pad;
	// copy over original
	Iter<'x'> x;
	Iter<'y'> y;
	Iter<'z'> z;
	auto orig_padded = padded.view().view(slice(0,last_valid_row,1),slice(0,last_valid_col,1),slice(0,3,1));
	orig_padded[x][y][z] = RGB[x+r][y+c][z];
	// pad
	auto to_pad = padded.view().view(slice(last_valid_row,row_pad,1),slice(0,8,1),slice(0,3,1));
	// TODO left off here. Have issue with builder realization. see note in staged_blocklike::realize_each
//	to_pad[x][y][z] = padded[last_valid_row-1][y][z];
/*	for (dint x = last_valid_row; x < row_pad; x++) {
	  for (dint y = 0; y < 8; y++) {
	    for (dint z = 0; z < 3; z++) {
	      padded(x+last_valid_row,y,z) = padded(last_valid_row-1,y,z);
	    }
	  }
	}*/
//	for (dint x = 0; x < 8; x++) {
//	  for (dint y = 0; y < col_pad; y++) {
//	    for (dint z = 0; z < 3; z++) {
//	      padded(x,y+last_valid_col,z) = padded(x,last_valid_col-1, z);
//	    }
//	  }
//	}	
//	color(padded, YCbCr);
      } else {
	// no padding needed
//	color(mcu, YCbCr);	
      }
    }
  }

}

int main() {
  builder::builder_context context;
  auto ast = context.extract_function_ast(jpeg_staged, "jpeg");
  block::eliminate_redundant_vars(ast);	
  block::c_code_generator::generate_code(ast, std::cout, 0);
}
