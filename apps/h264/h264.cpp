#include <iostream>
#include "sh264.h"

using namespace std;

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage: ./h264d <annexB_264>" << endl;
    exit(-1);
  }
  struct timespec begin, end;
  clock_gettime(CLOCK_MONOTONIC, &begin);
  FILE *annexB = fopen(argv[1], "rb");

  fseek(annexB, 0, SEEK_END);
  size_t size = ftell(annexB);
  fseek(annexB, 0, SEEK_SET);
  uint8_t *buffer = new uint8_t[size];
  assert(fread(buffer, 1, size, annexB) == size);
  fclose(annexB);
  parse_h264(buffer, size);
  clock_gettime(CLOCK_MONOTONIC, &end);
  long seconds = end.tv_sec - begin.tv_sec;
  long nanoseconds = end.tv_nsec - begin.tv_nsec;
  double elapsed = seconds + nanoseconds*1e-9;
  std::cout << "Program took " << elapsed << "s" << std::endl;  
}
