#include "shim.hpp"

using namespace shim;
using namespace std;

// clang++ -O3 -std=c++17 -I../ -I. -I../../buildit/include -I../../buildit/build/gen_headers/ -L../../buildit/build -lbuildit test.cpp -o out

using builder::dyn_arr;
using namespace std;


template <int...Xs>
auto darr() {
  return dyn_arr<int,sizeof...(Xs)>{Xs...};
}

dyn_arr<int,3> wrap3(array<int,3> arr) {
  dyn_arr<int,3> da;
  da[0] = arr[0];
  da[1] = arr[1];
  da[2] = arr[2];
  return da;
}

dyn_arr<int,2> wrap2(array<int,2> arr) {
  dyn_arr<int,2> da;
  da[0] = arr[0];
  da[1] = arr[1];
  return da;
}

void my_func() {
  Properties<3> prop1;

  ASSERT(compare_arrays(prop1.extents(), {1,1,1}));			
  ASSERT(compare_arrays(prop1.strides(), {1,1,1}));			
  ASSERT(compare_arrays(prop1.refinement(), {1,1,1}));		
  ASSERT(compare_arrays(prop1.origin(), {0,0,0}));  
  ASSERT(compare_arrays(wrap3(prop1.permutations()), {0,1,2}));

  auto prop2 = prop1.with_extents({2,3,4});
  ASSERT(compare_arrays(prop2.extents(), {2,3,4}));
  ASSERT(compare_arrays(prop2.strides(), {1,1,1}));
  ASSERT(compare_arrays(prop2.refinement(), {1,1,1}));
  ASSERT(compare_arrays(prop2.origin(), {0,0,0}));
  ASSERT(compare_arrays(wrap3(prop2.permutations()), {0,1,2}));

  auto prop3 = prop2.with_strides({1,1,2});
  ASSERT(compare_arrays(prop3.extents(), {2,3,4}));
  ASSERT(compare_arrays(prop3.strides(), {1,1,2}));
  ASSERT(compare_arrays(prop3.refinement(), {1,1,1}));
  ASSERT(compare_arrays(prop3.origin(), {0,0,0}));
  ASSERT(compare_arrays(wrap3(prop3.permutations()), {0,1,2}));

  auto prop4 = prop3.with_refinement({3,1,2});
  ASSERT(compare_arrays(prop4.extents(), {2,3,4}));
  ASSERT(compare_arrays(prop4.strides(), {1,1,2}));
  ASSERT(compare_arrays(prop4.refinement(), {3,1,2}));
  ASSERT(compare_arrays(prop4.origin(), {0,0,0}));
  ASSERT(compare_arrays(wrap3(prop4.permutations()), {0,1,2}));

  auto prop5 = prop4.with_origin({7,8,9});
  ASSERT(compare_arrays(prop5.extents(), {2,3,4}));
  ASSERT(compare_arrays(prop5.strides(), {1,1,2}));
  ASSERT(compare_arrays(prop5.refinement(), {3,1,2}));
  ASSERT(compare_arrays(prop5.origin(), {7,8,9}));
  ASSERT(compare_arrays(wrap3(prop5.permutations()), {0,1,2}));

  // identity permutation
  auto prop6 = prop5.permute({0,1,2});
  ASSERT(compare_arrays(prop6.extents(), {2,3,4}));
  ASSERT(compare_arrays(prop6.strides(), {1,1,2}));
  ASSERT(compare_arrays(prop6.refinement(), {3,1,2}));
  ASSERT(compare_arrays(prop6.origin(), {7,8,9}));
  ASSERT(compare_arrays(wrap3(prop6.permutations()), {0,1,2}));

  auto prop7 = prop6.permute({2,0,1});
  ASSERT(compare_arrays(prop7.extents(), {4,2,3}));
  ASSERT(compare_arrays(prop7.strides(), {2,1,1}));
  ASSERT(compare_arrays(prop7.refinement(), {2,3,1}));
  ASSERT(compare_arrays(prop7.origin(), {9,7,8}));
  ASSERT(compare_arrays(wrap3(prop7.permutations()), {2,0,1}));

  auto prop8 = prop7.refine({1,1,2});
  ASSERT(compare_arrays(prop8.extents(), {4,2,6}));
  ASSERT(compare_arrays(prop8.strides(), {2,1,1}));
  ASSERT(compare_arrays(prop8.refinement(), {2,3,2}));
  ASSERT(compare_arrays(prop8.origin(), {9,7,16}));
  ASSERT(compare_arrays(wrap3(prop8.permutations()), {2,0,1}));

  dyn_var<int> idx1 = prop8.linearize({0,0,0});
  ASSERT(idx1 == 0);
  dyn_var<int> idx2 = prop8.linearize({1,2,3});
  ASSERT(idx2 == 27);

  // like the example in thesis
  auto prop9 = Properties<4>().with_permutations({2,0,3,1});
  auto prop10 = Properties<4>().with_permutations({3,1,0,2});
  auto prop11 = Properties<2>();
  auto res1 = prop9.pointwise_mapping({4,5,6,7}, prop10);
  auto res2 = prop11.pointwise_mapping({4,5}, prop9);
  auto res3 = prop9.pointwise_mapping({4,5,6,7}, prop11);
  ASSERT(compare_arrays(res1, {6,7,5,4}));
  ASSERT(compare_arrays(res2, {4,0,5,0}));
  ASSERT(compare_arrays(res3, {4,6}));

  auto prop12 = Properties<2>().
    with_origin({2,1}).
    with_strides({2,2}).
    with_extents({2,2});
  auto prop13 = prop12.refine({16,16}).permute({1,0});
  ASSERT(compare_arrays(prop12.pointwise_mapping({0,0}, prop13),{0,0}));
  ASSERT(compare_arrays(prop12.pointwise_mapping({1,0}, prop13),{0,16}));
  ASSERT(compare_arrays(prop12.pointwise_mapping({0,1}, prop13),{16,0}));
  ASSERT(compare_arrays(prop12.pointwise_mapping({1,1}, prop13),{16,16}));

  ASSERT(compare_arrays(prop13.pointwise_mapping({16,16}, prop12),{1,1}));
  ASSERT(compare_arrays(prop13.pointwise_mapping({16,24}, prop12),{1,1}));
  ASSERT(compare_arrays(prop13.pointwise_mapping({24,16}, prop12),{1,1}));
  ASSERT(compare_arrays(prop13.pointwise_mapping({24,24}, prop12),{1,1}));

  auto prop14 = prop12.slice(Range(0,4,1), Range(2,10,2));
  ASSERT(compare_arrays(prop14.extents(), {4,4}));
  ASSERT(compare_arrays(prop14.strides(), {2,4}));
  ASSERT(compare_arrays(prop14.refinement(), {1,1}));
  ASSERT(compare_arrays(prop14.origin(), {2,5}));

  auto res4 = prop12.hcolocate(prop13.permute({1,0}));
  auto res5 = prop12.colocate(prop13);
  auto res6 = prop13.colocate(prop12);
  auto res7 = prop13.hcolocate(prop12);
  ASSERT(compare_arrays(res4.extents(), prop12.extents()));
  ASSERT(compare_arrays(res4.strides(), prop12.strides()));
  ASSERT(compare_arrays(res4.refinement(), prop12.refinement()));
  ASSERT(compare_arrays(res4.origin(), prop12.origin()));
  ASSERT(compare_arrays(wrap2(res4.permutations()), wrap2(prop12.permutations())));

  ASSERT(compare_arrays(res5.extents(), prop13.extents()));
  ASSERT(compare_arrays(res5.strides(), prop13.strides()));
  ASSERT(compare_arrays(res5.refinement(), prop13.refinement()));
  ASSERT(compare_arrays(res5.origin(), prop13.origin()));
  ASSERT(compare_arrays(wrap2(res5.permutations()), wrap2(prop13.permutations())));

  ASSERT(compare_arrays(res6.extents(), prop12.extents()));
  ASSERT(compare_arrays(res6.strides(), prop12.strides()));
  ASSERT(compare_arrays(res6.refinement(), prop12.refinement()));
  ASSERT(compare_arrays(res6.origin(), prop12.origin()));
  ASSERT(compare_arrays(wrap2(res6.permutations()), wrap2(prop12.permutations())));

  ASSERT(compare_arrays(res7.extents(), prop13.extents()));
  ASSERT(compare_arrays(res7.strides(), prop13.strides()));
  ASSERT(compare_arrays(res7.refinement(), prop13.refinement()));
  ASSERT(compare_arrays(res7.origin(), prop13.origin()));
  ASSERT(compare_arrays(wrap2(res7.permutations()), wrap2(prop13.permutations())));

  auto block = heap<int>(Properties<2>().with_extents(2,2));
  block.dump();
  block.write(Property<2>{0,0},97);
  block.write(Property<2>{0,1},1);
  block.write(Property<2>{1,0},2);
  block.write(Property<2>{1,1},3);
  ASSERT(block(0,0) == 97);
  ASSERT(block(0,1) == 1);
  ASSERT(block(1,0) == 2);
  ASSERT(block(1,1) == 3);
  
  Iter<'i'> i;
  Iter<'j'> j;
  block[i][j] = i*2+j+4;
  ASSERT(block(0,0) == 4);
  ASSERT(block(0,1) == 5);
  ASSERT(block(1,0) == 6);
  ASSERT(block(1,1) == 7);
  
}

int main() {
  CompileOptions::isCPP = true;
  stage(my_func, "my_func", "my_func", "", "");
}
