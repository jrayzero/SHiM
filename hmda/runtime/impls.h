#pragma once

#include "heaparray.h"

// there phony template parameters here so I can just define everything in the header

#define BUILDER_IMPL(dtype)				\
  template <bool dummy=true>				\
    HeapArray<dtype> build_heaparr_##dtype(loop_type sz) { \
    return HeapArray<dtype>(sz);			\
  }

#define READER_IMPL(dtype)				\
  template <bool dummy=true>				\
  dtype read_##dtype(dtype *data, loop_type lidx) {	\
    return data[lidx];					\
  } 

#define WRITER_IMPL(dtype)						\
  template <bool dummy=true>						\
  void write_##dtype(dtype *data, loop_type lidx, dtype val) {		\
    data[lidx] = val;							\
  }

#define HREADER_IMPL(dtype)						\
  template <bool dummy=true>						\
  dtype read_heaparr_##dtype(const HeapArray<dtype> &heaparr, loop_type lidx) { \
    return heaparr[lidx];						\
  }

#define HWRITER_IMPL(dtype)						\
  template <bool dummy=true>						\
  void write_heaparr_##dtype(HeapArray<dtype> &heaparr, loop_type lidx, dtype val) { \
    heaparr.write(lidx, val);						\
  }

#define MEMSET_IMPL(dtype)					\
  template <bool dummy=true>					\
    void memset_##dtype(dtype *data, dtype val, loop_type sz) {	\
    memset(data, val, sz);					\
  }

#define HMEMSET_IMPL(dtype)						\
  template <bool dummy=true>						\
    void memset_heaparr_##dtype(HeapArray<dtype> &data, dtype val, loop_type sz) { \
    memset(data.base->data, val, sz);					\
  }

namespace hmda {

BUILDER_IMPL(uint8_t);
BUILDER_IMPL(uint16_t);
BUILDER_IMPL(uint32_t);
BUILDER_IMPL(uint64_t);
BUILDER_IMPL(int16_t);
BUILDER_IMPL(int32_t);
BUILDER_IMPL(int64_t);
BUILDER_IMPL(float);
BUILDER_IMPL(double);

READER_IMPL(uint8_t);
READER_IMPL(uint16_t);
READER_IMPL(uint32_t);
READER_IMPL(uint64_t);
READER_IMPL(int16_t);
READER_IMPL(int32_t);
READER_IMPL(int64_t);
READER_IMPL(float);
READER_IMPL(double);

WRITER_IMPL(uint8_t);
WRITER_IMPL(uint16_t);
WRITER_IMPL(uint32_t);
WRITER_IMPL(uint64_t);
WRITER_IMPL(int16_t);
WRITER_IMPL(int32_t);
WRITER_IMPL(int64_t);
WRITER_IMPL(float);
WRITER_IMPL(double);

HREADER_IMPL(uint8_t);
HREADER_IMPL(uint16_t);
HREADER_IMPL(uint32_t);
HREADER_IMPL(uint64_t);
HREADER_IMPL(int16_t);
HREADER_IMPL(int32_t);
HREADER_IMPL(int64_t);
HREADER_IMPL(float);
HREADER_IMPL(double);

HWRITER_IMPL(uint8_t);
HWRITER_IMPL(uint16_t);
HWRITER_IMPL(uint32_t);
HWRITER_IMPL(uint64_t);
HWRITER_IMPL(int16_t);
HWRITER_IMPL(int32_t);
HWRITER_IMPL(int64_t);
HWRITER_IMPL(float);
HWRITER_IMPL(double);

MEMSET_IMPL(uint8_t);
MEMSET_IMPL(uint16_t);
MEMSET_IMPL(uint32_t);
MEMSET_IMPL(uint64_t);
MEMSET_IMPL(int16_t);
MEMSET_IMPL(int32_t);
MEMSET_IMPL(int64_t);
MEMSET_IMPL(float);
MEMSET_IMPL(double);

HMEMSET_IMPL(uint8_t);
HMEMSET_IMPL(uint16_t);
HMEMSET_IMPL(uint32_t);
HMEMSET_IMPL(uint64_t);
HMEMSET_IMPL(int16_t);
HMEMSET_IMPL(int32_t);
HMEMSET_IMPL(int64_t);
HMEMSET_IMPL(float);
HMEMSET_IMPL(double);

template <typename To, typename From>
To cast(From val) { return (To)val; }

}
