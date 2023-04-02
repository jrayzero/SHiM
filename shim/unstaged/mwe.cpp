#include "unite.h"

using namespace shim;
using namespace unstaged;

int main() {

  auto rgb = TensorBuilder<3>().with_extents(std::array{4,4,3}).to_block<int>();
  auto ycbcr = rgb.view().ppermute(std::array{2,0,1});
  
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
	ycbcr.write(std::array{i,j,k}, i*4*4 + j*4 + k);
      }
    }
  }

  std::cout << ycbcr.dump() << std::endl;
  std::cout << ycbcr.view().dump() << std::endl;
  
}

