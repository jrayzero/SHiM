#include "utils.h"

dint clip1Y(dint x) {
  if (x < 0) {
    return 0;
  } else if (x > 128) {
    return 128;
  } else {
    return x;
  }
}
