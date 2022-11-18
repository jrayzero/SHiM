#include <chrono>

#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include "base.h"
#include "functors.h"
#include "staged/staged.h"
#include "unstaged/unstaged_blocklike.h"

using namespace std;
using namespace hmda;
using builder::dyn_var;

// like example 2, but with fusion

// data represents the placeholder for the underlying block/view buffer accessed
void kernel_fused(dyn_var<char*> RGB_data, dyn_var<int*> YCbCr_data, 
		  View<char,3> RGB, Block<int,3> YCbCr) {
  // associate the buffers with the unstaged block/views
  auto sRGB = RGB.stage(RGB_data);
  auto sYCbCr = YCbCr.stage(YCbCr_data);
  
  // Make inline iterators
  Iter<'i'> i;
  Iter<'j'> j;

  // get handles to the computation
  auto handle0 = 
    sYCbCr[0][i][j].lazy(sRGB[i][j][0]*0.299     + sRGB[i][j][1]*0.587     + sRGB[i][j][2]*0.114);
  auto handle1 = 
    sYCbCr[1][i][j].lazy(sRGB[i][j][0]*-0.168736 + sRGB[i][j][1]*-0.33126  + sRGB[i][j][2]*0.500002  + 128);
  auto handle2 =
    sYCbCr[2][i][j].lazy(sRGB[i][j][0]*0.5       + sRGB[i][j][1]*-0.418688 + sRGB[i][j][2]*-0.081312 + 128);

  // fuse the outer loop
  auto fused = outer_fuse(outer_fuse(handle0,handle1), handle2);
  
  // generate code for it
  fused.generate();
}

void kernel_no_schedule(dyn_var<char*> RGB_data, dyn_var<int*> YCbCr_data, 
			View<char,3> RGB, Block<int,3> YCbCr) {
  // associate the buffers with the unstaged block/views
  auto sRGB = RGB.stage(RGB_data);
  auto sYCbCr = YCbCr.stage(YCbCr_data);
  
  // Make inline iterators
  Iter<'i'> i;
  Iter<'j'> j;

  // perform the color conversion computation
  sYCbCr[0][i][j] = sRGB[i][j][0]*0.299     + sRGB[i][j][1]*0.587     + sRGB[i][j][2]*0.114;
  sYCbCr[1][i][j] = sRGB[i][j][0]*-0.168736 + sRGB[i][j][1]*-0.33126  + sRGB[i][j][2]*0.500002  + 128;
  sYCbCr[2][i][j] = sRGB[i][j][0]*0.5       + sRGB[i][j][1]*-0.418688 + sRGB[i][j][2]*-0.081312 + 128;
}

int main(int argc, char **argv) {
  
  if (argc != 3) {
    cerr << "Usage: ./example2 <H> <W>" << endl;
    exit(-1);
  }

  int H = stoi(argv[1]);
  int W = stoi(argv[2]);

  if (H % 8 != 0 || W % 8 != 0) {
    cerr << "H and W must be multiples of 8 for this example" << endl;
    exit(-1);
  }  

  // make our fake inputs
  auto RGB = Block<char,3>({H, W, 3});
  auto YCbCr = Block<int,3>({3, 8, 8});

  for (int i = 0; i < H; i++) {
    for (int j = 0; j < W; j++) {
      RGB(i,j,0) = rand() % 100;
      RGB(i,j,1) = rand() % 100;
      RGB(i,j,2) = rand() % 100;
    }
  }
  cout << (int)RGB(0,0,0) << endl;

  // slice out one MCU from RGB
  auto mcu = RGB.view(slice(0,8,1),slice(0,8,1));
  // stage it and run it
  //  stage(kernel_fused, "kernel_fused", mcu, YCbCr);
  cout << "Begin dynamic staging" << endl;
  auto start = chrono::high_resolution_clock::now();
  auto fptr = (void (*)(char*,int*))dynamic_stage(kernel_fused, mcu, YCbCr);
  auto stop = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::nanoseconds>(stop - start);
  cout << "Dynamic Staging time " << duration.count()/1e9 << endl;
  fptr(RGB, YCbCr);
  cout << (int)YCbCr(0,0,0) << endl;

  cout << "Begin staging" << endl;
  auto start2 = chrono::high_resolution_clock::now();
  stage(kernel_fused, "kernel", mcu, YCbCr);
  auto stop2 = chrono::high_resolution_clock::now();
  auto duration2 = chrono::duration_cast<chrono::nanoseconds>(stop2 - start2);
  cout << "Staging time " << duration2.count()/1e9 << endl;

}
