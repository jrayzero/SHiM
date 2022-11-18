#include "hmda.h"

using namespace std;
using namespace hmda;

int main() {

  // Create a type-based two level loop
  using Loop_T1 = LoopLevel<0, 
			    LoopLevel<1, 
				      StmtLevel<0, NullLevel, LoopDFSIter<0>, LoopDFSIter<1>>, 
				      NullLevel, Extent<1,0>>, 
			    NullLevel, Extent<0,0>>;
  
  // and another one
  using Loop_T2 = LoopLevel<0, 
			    LoopLevel<1, 
				      StmtLevel<0, NullLevel, LoopDFSIter<0>, LoopDFSIter<1>>, 
				      NullLevel, Extent<1,0>>, 
			    NullLevel, Extent<0,0>>;
  // fuse them
  auto fused = schedule::test_fuse<Loop_T1, Loop_T2>();
  cout << decltype(fused)::dump() << endl;
}
