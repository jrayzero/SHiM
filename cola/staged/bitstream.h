// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "fwrappers.h"

namespace cola {

struct Bitstream {
  
  Bitstream(builder::dyn_var<uint8_t*> bitstream, builder::dyn_var<uint64_t> length) : 
    bitstream(bitstream), length(length), cursor(0) { }

  ///
  /// Peek n bits in the bitstream
  /// n can be max 64 bits, but is not checked
  template <typename Ret=uint64_t>
  builder::dyn_var<Ret> peek(builder::dyn_var<int> n);

  ///
  /// Assume byte-aligned and peek n bits in the bitstream 
  /// n can be max 64 bits, but is not checked
  template <typename Ret=uint64_t>
  builder::dyn_var<Ret> peek_aligned(builder::dyn_var<int> n);

  ///
  /// Peek n bits in the bitstream and move the cursor n bits
  /// n can be max 64 bits, but is not checked
  template <typename Ret=uint64_t>
  builder::dyn_var<Ret> pop(builder::dyn_var<int> n);

  ///
  /// Assume byte-aligned and peek n bits in the bitstream and move the cursor n bits
  /// n can be max 64 bits, but is not checked
  template <typename Ret=uint64_t>
  builder::dyn_var<Ret> pop_aligned(builder::dyn_var<int> n);

  ///
  /// Peek n bits in the bitstream and move the cursor n bits.
  /// Check if value matches requested data and fail otherwise.
  /// n can be max 64 bits, but is not checked
  template <typename Ret=uint64_t>
  builder::dyn_var<Ret> pop_check(builder::dyn_var<int> n, builder::dyn_var<Ret> should_be);

  ///
  /// Assuem byte-aligned and peek n bits in the bitstream and move the cursor n bits.
  /// Check if value matches requested data and fail otherwise.
  /// n can be max 64 bits, but is not checked
  template <typename Ret=uint64_t>
  builder::dyn_var<Ret> pop_check_aligned(builder::dyn_var<int> n, builder::dyn_var<Ret> should_be);

  /// 
  /// Move the cursor n bits
  void skip(builder::dyn_var<unsigned int> n);

  ///
  /// Check if there are at least n bits left in the bitstream.
  /// 1 = at least n bits, 0 = nit at least n bits
  builder::dyn_var<int> exists(builder::dyn_var<unsigned int> n);

  builder::dyn_var<int> dummy_stuff() {
    for (builder::dyn_var<int> i = 0; i < 10; i = i+1) { }
    return 0;
  }

  builder::dyn_var<uint8_t*> bitstream;
  builder::dyn_var<uint64_t> length;
  builder::dyn_var<uint64_t> cursor;
};

template <typename Ret>
builder::dyn_var<Ret> Bitstream::peek(builder::dyn_var<int> n) {
  builder::dyn_var<uint64_t> bit_idx = cursor % 8;
  builder::dyn_var<uint8_t> peeked = 0;
  if (bit_idx == 0) {
    // this is byte aligned
    peeked = peek_aligned<Ret>(n);
  } else {
    // not byte aligned
    // read up to factor bits, where factor is how many bits til aligned
//    builder::dyn_var<Ret> peeked = 0;
    builder::dyn_var<uint64_t> byte_idx = cursor / 8;
    builder::dyn_var<int> bits_left = n;
    builder::dyn_var<uint8_t> byte = bitstream[byte_idx];
    byte_idx = byte_idx + 1;
    builder::dyn_var<int> factor = 0;
    if (bits_left < (8-bit_idx)) {
      factor = bits_left;
    } else {
      factor = 8 - bit_idx;
    }
//    builder::dyn_var<int> shift_amt = 8 - bit_idx - factor;
//    byte = cola::rshift(byte, shift_amt);
    builder::dyn_var<uint8_t> shift_val = 0xFF;
    builder::dyn_var<uint8_t> mask = cola::rshift(shift_val, 8-factor);
    peeked = band(byte, mask);
    bits_left = bits_left - factor;    
    // now we are aligned, so we can read the remainder
    peeked = cola::lshift(peeked, bits_left);
    // temporarily adjust the cursor though
    cursor = cursor + factor;
    peeked = bor(peeked, peek_aligned(bits_left));
    cursor = cursor - factor;
  }
  // if put this within the conditionals, codegen barfs
  return peeked;
}

template <typename Ret>
builder::dyn_var<Ret> Bitstream::peek_aligned(builder::dyn_var<int> n) {
  // read a byte at a time 
  builder::dyn_var<Ret> peeked = 0;
  builder::dyn_var<uint64_t> byte_idx = cursor / 8;
  builder::dyn_var<int> bits_left = n;
  builder::dyn_var<int> upper = n-8;
  builder::dyn_var<int> dummy = 0;
  for (builder::dyn_var<int> i = 0; i < upper; i = i + 8) {
    builder::dyn_var<uint8_t> byte = bitstream[byte_idx];
    byte_idx = byte_idx + 1;
    builder::dyn_var<Ret> shifted = cola::lshift(peeked, 8 * dummy);
    peeked = bor(shifted, byte);
    dummy = 1;
    bits_left = bits_left - 8;
  }

  // if n was not a multiple of 8, will have some stragglers here
  if (bits_left > 0) {
    peeked = cola::lshift(peeked, bits_left);
    builder::dyn_var<uint8_t> byte = bitstream[byte_idx];
    byte = cola::rshift(byte, 8-bits_left);
    peeked = bor(peeked, byte);
  }
  return peeked;
}

template <typename Ret>
builder::dyn_var<Ret> Bitstream::pop(builder::dyn_var<int> n) {
  builder::dyn_var<Ret> popped = peek<Ret>(n);
  skip(n);
  return popped;
}

template <typename Ret>
builder::dyn_var<Ret> Bitstream::pop_aligned(builder::dyn_var<int> n) {
  builder::dyn_var<Ret> popped = peek_aligned<Ret>(n);
  skip(n);
  return popped;
}

template <typename Ret>
builder::dyn_var<Ret> Bitstream::pop_check(builder::dyn_var<int> n, builder::dyn_var<Ret> should_be) {
  builder::dyn_var<Ret> popped = peek<Ret>(n);
  skip(n);
  if (popped != should_be) {
    // TODO better error message (print expected then got)
    cola::print_string("Invalid data");
    cola::hexit(-1);
  }
  return popped;
}

template <typename Ret>
builder::dyn_var<Ret> Bitstream::pop_check_aligned(builder::dyn_var<int> n, builder::dyn_var<Ret> should_be) {
  builder::dyn_var<Ret> popped = peek_aligned<Ret>(n);
  skip(n);
  if (popped != should_be) {
    // TODO better error message (print expected then got)
    cola::print_string("Invalid data");
    cola::hexit(-1);
  }
  return popped;
}

void Bitstream::skip(builder::dyn_var<unsigned int> n) {
  cursor = cursor + n;
}

builder::dyn_var<int> Bitstream::exists(builder::dyn_var<unsigned int> n) {
  builder::dyn_var<int> ok = 0;
  if (cursor+n < length) {
    ok = 1;
  }
  return ok;
}

}
