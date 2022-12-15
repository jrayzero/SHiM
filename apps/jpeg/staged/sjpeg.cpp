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
using builder::static_var;

using dint = dyn_var<int>;
using sint = static_var<int>;
template <int Rank>
using loc = dyn_var<int[Rank]>;

Iter<'i'> i;
Iter<'j'> j;
Iter<'k'> k;

constexpr char huffmanCodesType[] = "const HuffmanCodes&";
constexpr char bitsType[] = "Bits&";
using HuffmanCodes = typename builder::name<huffmanCodesType>;
using Bits = typename builder::name<bitsType>;

// Question: things don't seem to be happy when I have instances of builder::builder.
// When should I use one vs the other???

void scale_quant(Block<int,2> quant, int quality) {
  int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
  quant[i][j] = (quant[i][j]*scale+50)/100;
}

template <typename RGB_T>
void color(RGB_T RGB, Block<int,3> YCbCr) {
  YCbCr[0][i][j] = 
    cast<int>(cast<double>(RGB[i][j][0])*0.299 + 
	      cast<double>(RGB[i][j][1])*0.587 + 
	      cast<double>(RGB[i][j][2])*0.114);
  YCbCr[1][i][j] = 
    cast<int>(cast<double>(RGB[i][j][0])*-0.168736 + 
	      cast<double>(RGB[i][j][1])*-0.33126 + 
	      cast<double>(RGB[i][j][2])*0.500002) + 128;
  YCbCr[2][i][j] = 
    cast<int>(cast<double>(RGB[i][j][0])*0.5 + 
	      cast<double>(RGB[i][j][1])*-0.418688 + 
	      cast<double>(RGB[i][j][2])*-0.081312) + 128;
}

template <typename RGB_T>
void color_one(RGB_T RGB, Block<int,3> YCbCr, dint component) {
  if (component = 0) {
    YCbCr[0][i][j] = 
      cast<int>(cast<double>(RGB[i][j][0])*0.299 + 
		cast<double>(RGB[i][j][1])*0.587 + 
		cast<double>(RGB[i][j][2])*0.114);
  } else if (component = 1) {
    YCbCr[1][i][j] = 
      cast<int>(cast<double>(RGB[i][j][0])*-0.168736 + 
		cast<double>(RGB[i][j][1])*-0.33126 + 
		cast<double>(RGB[i][j][2])*0.500002) + 128;
  } else {
    YCbCr[2][i][j] = 
      cast<int>(cast<double>(RGB[i][j][0])*0.5 + 
		cast<double>(RGB[i][j][1])*-0.418688 + 
		cast<double>(RGB[i][j][2])*-0.081312) + 128;
  }
}

// buildit doesn't have shift operators, so use these
dyn_var<int(int,int)> lshift = builder::as_global("lshift");
dyn_var<int(int,int)> rshift = builder::as_global("rshift");

// the external things to call for doing huffman
// these are completely the wrong types but w/e
dyn_var<void(HEAP_T<int>,int,int,void*,void*,void*)> huffman_encode_block = builder::as_global("huffman_encode_block");

void dct(View<int,3> obj) { 
#define descale(x,n) rshift((x + (lshift(1, (n-1)))), n)
  sint FIX_0_298631336 = 2446;
  sint FIX_0_390180644 = 3196;
  sint FIX_0_541196100 = 4433;
  sint FIX_0_765366865 = 6270;
  sint FIX_0_899976223 = 7373;
  sint FIX_1_175875602 = 9633;
  sint FIX_1_501321110 = 12299;
  sint FIX_1_847759065 = 15137;
  sint FIX_1_961570560 = 16069;
  sint FIX_2_053119869 = 16819;
  sint FIX_2_562915447 = 20995;
  sint FIX_3_072711026 = 25172;
  
  for (dint r = 0; r < 8; r=r+1) {
    auto row = obj.view(slice(0,1,1),slice(r,r+1,1),slice(0,8,1));
    dint tmp0 = row(0,0,0) + row(0,0,7);
    dint tmp7 = row(0,0,0) - row(0,0,7);
    dint tmp1 = row(0,0,1) + row(0,0,6);
    dint tmp6 = row(0,0,1) - row(0,0,6);
    dint tmp2 = row(0,0,2) + row(0,0,5);
    dint tmp5 = row(0,0,2) - row(0,0,5);
    dint tmp3 = row(0,0,3) + row(0,0,4);
    dint tmp4 = row(0,0,3) - row(0,0,4);
    dint tmp10 = tmp0 + tmp3;
    dint tmp13 = tmp0 - tmp3;
    dint tmp11 = tmp1 + tmp2;
    dint tmp12 = tmp1 - tmp2;
    row[0][0][0] = lshift(tmp10 + tmp11, 2);
    row[0][0][4] = lshift(tmp10 - tmp11, 2);
    dint z1 = (tmp12 + tmp13) * FIX_0_541196100;
    row[0][0][2] = descale(z1 + tmp13 * FIX_0_765366865, 11);
    row[0][0][6] = descale(z1 + tmp12 * -FIX_1_847759065, 11);
    z1 = tmp4 + tmp7;
    dint z2 = tmp5 + tmp6;
    dint z3 = tmp4 + tmp6;
    dint z4 = tmp5 + tmp7;
    dint z5 = (z3 + z4) * FIX_1_175875602;
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
    row[0][0][7] = descale(tmp4 + z1 + z3, 11);
    row[0][0][5] = descale(tmp5 + z2 + z4, 11);
    row[0][0][3] = descale(tmp6 + z2 + z3, 11);
    row[0][0][1] = descale(tmp7 + z1 + z4, 11);
  }
  for (dint c = 0; c < 8; c=c+1) {
    auto col = obj.view(slice(0,1,1), slice(0,8,1), slice(c,c+1,1));    
    dint tmp0 = col(0,0,0) + col(0,7,0);
    dint tmp7 = col(0,0,0) - col(0,7,0);
    dint tmp1 = col(0,1,0) + col(0,6,0);
    dint tmp6 = col(0,1,0) - col(0,6,0);
    dint tmp2 = col(0,2,0) + col(0,5,0);
    dint tmp5 = col(0,2,0) - col(0,5,0);
    dint tmp3 = col(0,3,0) + col(0,4,0);
    dint tmp4 = col(0,3,0) - col(0,4,0);
    dint tmp10 = tmp0 + tmp3;
    dint tmp13 = tmp0 - tmp3;
    dint tmp11 = tmp1 + tmp2;
    dint tmp12 = tmp1 - tmp2;
    col[0][0][0] = descale(tmp10 + tmp11, 2);
    col[0][4][0] = descale(tmp10 - tmp11, 2);
    dint z1 = (tmp12 + tmp13) * FIX_0_541196100;
    col[0][2][0] = descale(z1 + tmp13 * FIX_0_765366865, 15);
    col[0][6][0] = descale(z1 + tmp12 * -FIX_1_847759065, 15);
    z1 = tmp4 + tmp7;
    dint z2 = tmp5 + tmp6;
    dint z3 = tmp4 + tmp6;
    dint z4 = tmp5 + tmp7;
    dint z5 = (z3 + z4) * FIX_1_175875602;
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
    col[0][7][0] = descale(tmp4 + z1 + z3, 15);
    col[0][5][0] = descale(tmp5 + z2 + z4, 15);
    col[0][3][0] = descale(tmp6 + z2 + z3, 15);
    col[0][1][0] = descale(tmp7 + z1 + z4, 15);
  }
}

void quant(View<int,3> obj, Block<int,2> quant) {
  for (dint i = 0; i < 8; i=i+1) {
    for (dint j = 0; j < 8; j=j+1) {
      dint v = obj(0,i,j);
      dint q = quant(i,j) * 8;
      if (v < 0) {
	v = -1*v;
	v = v + rshift(q, 1);
	v = v / q;
	v = -1*v;
      } else {
	v = v + rshift(q, 1);
	v = v / q;
      }
      obj[0][i][j] = v;
    }
  }
}

#if VERSION == 1
void jpeg_staged(dyn_var<uint8_t*> input, dyn_var<int> H, dyn_var<int> W, 
		 dyn_var<int*> luma_quant_arr, 
		 dyn_var<int*> chroma_quant_arr,
		 dyn_var<int*> zigzag, 
		 dyn_var<HuffmanCodes> luma_codes, 
		 dyn_var<HuffmanCodes> chroma_codes,
		 dyn_var<Bits> bits) {

  // Tables (these are already scaled)
  auto luma_quant = Block<int,2>::user(luma_quant_arr, 8, 8);
  auto chroma_quant = Block<int,2>::user(chroma_quant_arr, 8, 8);
    
  // start it up
  auto RGB = Block<uint8_t,3>::user(input, H, W, 3);
  dint last_Y = 0;
  dint last_Cb = 0;
  dint last_Cr = 0;

  auto YCbCr = Block<int,3>::stack<3,8,8>();
  
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
	auto padded = Block<uint8_t,3>::stack<8,8,3>();
	dint last_valid_row = 8 - row_pad;
	dint last_valid_col = 8 - col_pad;
	// copy over original
	auto orig_padded = padded.view(slice(0,last_valid_row,1),slice(0,last_valid_col,1),slice(0,3,1));
	orig_padded[i][j][k] = RGB[i+r][j+c][k];
	// pad
	auto row_padding_area = padded.view(slice(last_valid_row,last_valid_row+row_pad,1),slice(0,8,1),slice(0,3,1));
	auto col_padding_area = padded.view(slice(0,8,1), slice(last_valid_col,last_valid_col+col_pad,1), slice(0,3,1));
	row_padding_area[i][j][k] = padded[(last_valid_row-1)][j][k];
	col_padding_area[i][j][k] = padded[i][(last_valid_col-1)][k];
	color(padded, YCbCr);
      } else {
	// no padding needed
	color(mcu, YCbCr);	
      }
      // offset
      YCbCr[i][j][k] = YCbCr[i][j][k] - 128;
      // Y
      auto Y = YCbCr.view(slice(0,1,1),slice(0,8,1),slice(0,8,1));
      dct(Y);
      quant(Y, luma_quant);
      huffman_encode_block(Y.allocator->stack<3*8*8>(), 0, last_Y, bits, zigzag, luma_codes);
      last_Y = Y(0,0,0);
      // Cb
      auto Cb = YCbCr.view(slice(1,2,1),slice(0,8,1),slice(0,8,1));
      dct(Cb);
      quant(Cb, chroma_quant);
      huffman_encode_block(Cb.allocator->stack<3*8*8>(), 1, last_Cb, bits, zigzag, chroma_codes);
      last_Cb = Cb(0,0,0);
      // Cr
      auto Cr = YCbCr.view(slice(2,3,1),slice(0,8,1),slice(0,8,1));
      dct(Cr);
      quant(Cr, chroma_quant);      
      huffman_encode_block(Cr.allocator->stack<3*8*8>(), 2, last_Cr, bits, zigzag, chroma_codes);
      last_Cr = Cr(0,0,0);
    }
  }
}

#elif VERSION == 2
// This parallelizes on the color component
void jpeg_staged(dyn_var<uint8_t*> input, dyn_var<int> H, dyn_var<int> W, 
		 dyn_var<int*> luma_quant_arr, 
		 dyn_var<int*> chroma_quant_arr,
		 dyn_var<int*> zigzag, 
		 dyn_var<HuffmanCodes> luma_codes, 
		 dyn_var<HuffmanCodes> chroma_codes,
		 dyn_var<Bits> bits) {

  // Tables (these are already scaled)
  auto luma_quant = Block<int,2>::user(luma_quant_arr, 8, 8);
  auto chroma_quant = Block<int,2>::user(chroma_quant_arr, 8, 8);
    
  // start it up
  auto RGB = Block<uint8_t,3>::user(input, H, W, 3);
  dint last_Y = 0;
  dint last_Cb = 0;
  dint last_Cr = 0;

  auto YCbCr = Block<int,3>::stack<3,8,8>();
  
  for (dint r = 0; r < H; r = r + 8) {
    for (dint c = 0; c < W; c = c + 8) {
      // pad and color convert the whole thing in one operation without parallel
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
	auto padded = Block<uint8_t,3>::stack<8,8,3>();
	dint last_valid_row = 8 - row_pad;
	dint last_valid_col = 8 - col_pad;
	// copy over original
	auto orig_padded = padded.view(slice(0,last_valid_row,1),slice(0,last_valid_col,1),slice(0,3,1));
	orig_padded[i][j][k] = RGB[i+r][j+c][k];
	// pad
	auto row_padding_area = padded.view(slice(last_valid_row,last_valid_row+row_pad,1),slice(0,8,1),slice(0,3,1));
	auto col_padding_area = padded.view(slice(0,8,1), slice(last_valid_col,last_valid_col+col_pad,1), slice(0,3,1));
	row_padding_area[i][j][k] = padded[(last_valid_row-1)][j][k];
	col_padding_area[i][j][k] = padded[i][(last_valid_col-1)][k];
	for (dint comp = 0; comp < 3; comp = comp + 1) {
	  color_one(padded, YCbCr, comp);
	}
      } else {
	// no padding needed
	for (dint comp = 0; comp < 3; comp = comp + 1) {
	  color_one(mcu, YCbCr, comp);	
	}
      }
      // can't do huffman encoding within this because it depends on the order (unless I want to play with parallelizing bit
      // packing)
      for (dint comp = 0; comp < 3; comp = comp + 1) {
	// offset
	YCbCr[comp][j][k] = YCbCr[comp][j][k] - 128;
	auto clr = YCbCr.view(slice(comp,comp+1,1),slice(0,8,1),slice(0,8,1));
	dct(clr);
	quant(clr, comp == 0 ? luma_quant : chroma_quant);
      }
      huffman_encode_block(YCbCr.allocator->stack<3*8*8>(), 0, last_Y, bits, zigzag, luma_codes);
      last_Y = YCbCr(0,0,0);
      huffman_encode_block(YCbCr.allocator->stack<3*8*8>(), 1, last_Cb, bits, zigzag, chroma_codes);
      last_Cb = YCbCr(1,0,0);
      huffman_encode_block(YCbCr.allocator->stack<3*8*8>(), 2, last_Cr, bits, zigzag, chroma_codes);
      last_Cr = YCbCr(2,0,0);
    }
  }
}
#else
static_assert(false, "Must specify VERSION=1||2");
#endif


int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage: ./sjpeg <output_fn>" << endl;
    exit(-1);
  }
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  builder::builder_context context;
  auto ast = context.extract_function_ast(jpeg_staged, "jpeg");
//  block::eliminate_redundant_vars(ast);	
  ofstream src;
  ofstream hdr;
  string source = string(argv[1]) + ".cpp";
  string header = string(argv[1]) + ".h";
  src.open(source);
  hdr.open(header);
  hdr << "#pragma once" << endl;
  hdr << "#include <cmath>" << endl;
  hdr << "#include \"huffman.h\"" << endl;
  hdr << "#include \"bits.h\"" << endl;
  hdr << "void jpeg(unsigned char*, int, int, int*, int*, int*, const HuffmanCodes&, const HuffmanCodes&, Bits&);" << endl;
  hdr.flush();
  hdr.close();
  src << "#include \"" << header << "\"" << endl;
  src << "#include \"hmda_runtime.h\"" << endl;
  src << "int lshift(int a, int b) { return a << b; }" << endl;
  src << "int rshift(int a, int b) { return a >> b; }" << endl;  
  stringstream ss;
  hmda::hmda_cpp_code_generator::generate_code(ast, ss, 0);
  src << ss.str() << endl;
  src.flush();
  src.close();
  std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
  std::cout << "Staging took " << std::chrono::duration_cast<std::chrono::nanoseconds> (stop - start).count()/1e9 << "s" << std::endl;  
}
