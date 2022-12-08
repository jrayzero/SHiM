// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "common/loop_type.h"
#include "fwddecls.h"

// wrappers for functions that we generate calls to in the generated code

namespace hmda {

builder::dyn_var<void(int)> hexit = builder::as_global("exit");
builder::dyn_var<loop_type(loop_type)> hfloor = builder::as_global("floor");

// casting functions (type isn't quite right, but don't care!)
// these are called on dyn_vars. If you want to cast a Ref, call rcast
#define CAST(dtype)							\
  builder::dyn_var<dtype(dtype)> cast_##dtype = builder::as_global("hmda::cast<"#dtype ">");
CAST(uint8_t);
CAST(uint16_t);
CAST(uint32_t);
CAST(uint64_t);
CAST(int16_t);
CAST(int32_t);
CAST(int64_t);
CAST(float);
CAST(double);

// Things for accessing the underlying data of blocks and views
#define READER(dtype)							\
  builder::dyn_var<dtype(dtype*,loop_type)> read_##dtype = builder::as_global("hmda::read<" # dtype ">");
#define WRITER(dtype)							\
  builder::dyn_var<void(dtype*,loop_type,dtype)> write_##dtype = builder::as_global("hmda::write<" # dtype ">");
#define HREADER(dtype)							\
  builder::dyn_var<dtype(HEAP_T<dtype>,loop_type)> read_heaparr_##dtype = builder::as_global("hmda::read<" # dtype ">");
#define HWRITER(dtype)							\
  builder::dyn_var<void(HEAP_T<dtype>,loop_type,dtype)> write_heaparr_##dtype = builder::as_global("hmda::write<" # dtype ">");
#define MEMSET(dtype)							\
  builder::dyn_var<void(dtype*,dtype,loop_type)> memset_##dtype = builder::as_global("hmda::hmemset<" # dtype ">");
#define HMEMSET(dtype)							\
  builder::dyn_var<void(HEAP_T<dtype>,dtype,loop_type)> memset_heaparr_##dtype = builder::as_global("hmda::hmemset<" # dtype ">");

READER(uint8_t);
READER(uint16_t);
READER(uint32_t);
READER(uint64_t);
READER(int16_t);
READER(int32_t);
READER(int64_t);
READER(float);
READER(double);

WRITER(uint8_t);
WRITER(uint16_t);
WRITER(uint32_t);
WRITER(uint64_t);
WRITER(int16_t);
WRITER(int32_t);
WRITER(int64_t);
WRITER(float);
WRITER(double);

HREADER(uint8_t);
HREADER(uint16_t);
HREADER(uint32_t);
HREADER(uint64_t);
HREADER(int16_t);
HREADER(int32_t);
HREADER(int64_t);
HREADER(float);
HREADER(double);

HWRITER(uint8_t);
HWRITER(uint16_t);
HWRITER(uint32_t);
HWRITER(uint64_t);
HWRITER(int16_t);
HWRITER(int32_t);
HWRITER(int64_t);
HWRITER(float);
HWRITER(double);

MEMSET(uint8_t);
MEMSET(uint16_t);
MEMSET(uint32_t);
MEMSET(uint64_t);
MEMSET(int16_t);
MEMSET(int32_t);
MEMSET(int64_t);
MEMSET(float);
MEMSET(double);

HMEMSET(uint8_t);
HMEMSET(uint16_t);
HMEMSET(uint32_t);
HMEMSET(uint64_t);
HMEMSET(int16_t);
HMEMSET(int32_t);
HMEMSET(int64_t);
HMEMSET(float);
HMEMSET(double);

// Things for building heaparrs
#define BUILDER(dtype)							\
  builder::dyn_var<HEAP_T<dtype>(loop_type)> build_heaparr_##dtype = builder::as_global("hmda::build_heaparr<" # dtype ">");

BUILDER(uint8_t);
BUILDER(uint16_t);
BUILDER(uint32_t);
BUILDER(uint64_t);
BUILDER(int16_t);
BUILDER(int32_t);
BUILDER(int64_t);
BUILDER(float);
BUILDER(double);

}
