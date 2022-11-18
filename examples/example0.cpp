#include "hmda.h"

using namespace std;
using namespace hmda;

int main() {

  auto block1d = Block<int,1>({5},{1},{0});
  auto view1d_0 = block1d.view(slice(0,End(),2));
  auto view1d_1 = block1d.view(slice(1,End(),2));
  cout << block1d.dump() << endl;
  cout << view1d_0.dump();
  cout << view1d_1.dump();

}
