#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include "bits.h"

void Bits::flush_bits() {
  if (this->byte_idx == 0)
    return;
  fwrite(this->bits, 1, this->byte_idx, this->fd);
  this->byte_idx = 0;
}

void Bits::complete_byte_and_stuff(char stuff, int64_t stuff_on, int64_t stuff_val) {
  int rem = this->bit_idx % 8;
  if (rem > 0) {
    for (int i = 0; i < 8-rem; i++) {
      this->pack_and_stuff(stuff, 1, stuff_on, stuff_val);
    }
  }
}

int Bits::space_available() {
  return this->capacity - this->byte_idx;
}

void Bits::pack(int64_t val, int nbits) {
  if (nbits <= 0 || nbits >= 64) return;
  int64_t val2 = val;
  int64_t mask = ((int64_t)1 << nbits) - 1;
  val2 &= mask;
  int64_t shift_amt = 64 - nbits - this->bit_idx;
  val2 <<= shift_amt; 
  this->accum |= val2;
  this->bit_idx += nbits;
  while (this->bit_idx >= 8) {
    int64_t val3 = this->accum >> 56;
    this->bits[this->byte_idx] = val3;
    this->byte_idx++;
    if (space_available() == 0)
      flush_bits();
    this->bit_idx -= 8;
    this->accum <<= 8;
  }
}

void Bits::pack_and_stuff(int64_t val, int nbits, int64_t stuff_on, int64_t stuff_val) {
  if (nbits <= 0 || nbits >= 64) return;
  int64_t val2 = val;
  int64_t mask = ((int64_t)1 << nbits) - 1;
  val2 &= mask;
  int64_t shift_amt = 64 - nbits - this->bit_idx;
  val2 <<= shift_amt;
  this->accum |= val2;
  this->bit_idx += nbits;
  while (this->bit_idx >= 8) {
    int64_t val3 = this->accum >> 56;
    this->bits[this->byte_idx] = val3;
    this->byte_idx++;
    if (space_available() == 0)
      flush_bits();
    if (val3 == stuff_on) {
      this->bits[this->byte_idx] = stuff_val;
      this->byte_idx += 1;
      if (space_available() == 0)
	flush_bits();
    }
    this->bit_idx -= 8;
    this->accum <<= 8;
  }
}
