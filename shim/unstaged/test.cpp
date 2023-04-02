#include "unite.h"

using namespace shim;
using namespace unstaged;

#define A std::array

int main() {

/*  TensorBuilder<2> tb;
  tb.with_strides(T{1,2});
  tb.with_origin(T{2,3});
  tb.with_extents(T{3,4});  
  std::cout << tb.dump() << std::endl;
  Block<int,2> block = tb.to_block<int>();
  std::cout << block.dump() << std::endl;
  block.write(T{1,2}, 19);
  std::cout << block.dump() << std::endl;

  auto t = tb.to_tensor();
  std::tuple<int> tup1{8};
  std::tuple<int,int> tup2{8,9};
  std::tuple<int,int,int> tup3{8,9,10};
  auto tres1 = t.rank_mapping_extents<1>();
  std::cout << std::get<0>(tres1) << std::endl;
  auto tres2 = t.rank_mapping_extents<2>();
  std::cout << std::get<0>(tres2) << " " << std::get<1>(tres2) << std::endl;
  auto tres3 = t.rank_mapping_extents<3>();
  std::cout << std::get<0>(tres3) << " " << std::get<1>(tres3) << " " << std::get<2>(tres3) << std::endl;
  auto tres4 = t.rank_mapping_extents<4>();
  std::cout << std::get<0>(tres4) << " " << std::get<1>(tres4) << " " << std::get<2>(tres4) << " " << std::get<3>(tres4) << std::endl;
*/
  TensorBuilder<2> tb2;
  tb2.with_strides(A{1,1});
  tb2.with_origin(A{0,0});
  tb2.with_extents(A{3,4});  
  Block<int,2> block2 = tb2.to_block<int>();
  block2.write(std::array{2,1}, 19);
  auto view = block2.view();
  view.write(std::array{1,2},37);
  block2.dump();

  
  auto view2 = block2.partition(T{0,3,1}, T{0,4,1});
//  std::cout << view2.dump() << std::endl;

  auto view3 = block2.partition(T{1,3,1}, T{0,3,1});
  std::cout << view3.dump() << std::endl;

  auto view4 = block2.partition(T{1,3,1}, T{0,3,2});
  std::cout << view4.dump() << std::endl;

  auto view5 = view3.partition(T{0,2,1}, T{0,3,2});
  std::cout << view5.dump() << std::endl;
  view5.write(A{1,1}, 99);
  std::cout << view5.dump() << std::endl;
  std::cout << block2.dump() << std::endl;
}
