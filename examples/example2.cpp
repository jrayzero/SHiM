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

// data represents the placeholder for the underlying block/view buffer accessed
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
  // slice out one MCU from RGB
  auto mcu = RGB.view(slice(0,8,1),slice(0,8,1));
  // stage it
  stage(kernel_no_schedule, "kernel_no_schedule", mcu, YCbCr);

}
