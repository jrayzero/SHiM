#pragma once

#include <cstdint>
#include <cassert>
#include <cstdio>

using namespace std;

#define IS_POWER_OF_TWO(x) (x) > 0 && (((x) & ((x) - 1)) == 0)

// Deletes bits when goes out of scope!
struct Bits {
  int capacity;
  int byte_idx;
  uint8_t bit_idx; // within the accumulator
  FILE *fd;
  uint64_t accum;
  uint8_t *bits;

  Bits(FILE *fd, int capacity=1024) : capacity(capacity), byte_idx(0), bit_idx(0),
    fd(fd), accum(0), bits(new uint8_t[capacity]) { 
    assert(IS_POWER_OF_TWO(capacity));
  }

  ~Bits() { delete[] bits; }

  void flush_bits();
  void complete_byte_and_stuff(char stuff, int64_t stuff_on, int64_t stuff_val);
  int space_available();
  void pack(int64_t val, int nbits);
  void pack_and_stuff(int64_t val, int nbits, int64_t stuff_on, int64_t stuff_val);

};

