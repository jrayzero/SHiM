// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "common/loop_type.h"
#include "fwddecls.h"

// These are the wrappers for various functions that we want to generate calls for in the generated code.

// I think these need to be generated outside the staging itself, so I don't really know what a better way
// to write these is other than manually enumerating them.
namespace cola {

builder::dyn_var<void(int)> hexit = builder::as_global("exit");
builder::dyn_var<loop_type(loop_type)> hfloor = builder::as_global("floor");
builder::dyn_var<void(void)> print_newline = builder::as_global("cola::print_newline");
builder::dyn_var<void(char*)> print_string = builder::as_global("cola::print_string");
builder::dyn_var<void(void*)> print = builder::as_global("cola::print");
builder::dyn_var<void(void*)> printn = builder::as_global("cola::printn");
builder::dyn_var<void*(void*,void*)> scat_items = builder::as_global("cola::scat_items");
builder::dyn_var<int(int,int)> lshift = builder::as_global("cola::lshift");
builder::dyn_var<int(int,int)> rshift = builder::as_global("cola::rshift");
builder::dyn_var<int(int,int)> bor = builder::as_global("cola::bor");
builder::dyn_var<int(int,int)> band = builder::as_global("cola::band");
builder::dyn_var<int(int,int)> pow = builder::as_global("cola::pow");
builder::dyn_var<int(int,int)> ceil = builder::as_global("cola::ceil");

// Note: the arg tyoe for these casts isn't right, but I don't want to write every combination
// of dyn_var<to(from)>, and buildit doesn't seem to care.
builder::dyn_var<uint8_t(uint8_t)> cast_uint8_t = builder::as_global("cola::cast<uint8_t>");
builder::dyn_var<uint16_t(uint16_t)> cast_uint16_t = builder::as_global("cola::cast<uint16_t>");
builder::dyn_var<uint32_t(uint32_t)> cast_uint32_t = builder::as_global("cola::cast<uint32_t>");
builder::dyn_var<uint64_t(uint64_t)> cast_uint64_t = builder::as_global("cola::cast<uint64_t>");
builder::dyn_var<int16_t(int16_t)> cast_int16_t = builder::as_global("cola::cast<int16_t>");
builder::dyn_var<int32_t(int32_t)> cast_int32_t = builder::as_global("cola::cast<int32_t>");
builder::dyn_var<int64_t(int64_t)> cast_int64_t = builder::as_global("cola::cast<int64_t>");
builder::dyn_var<float(float)> cast_float = builder::as_global("cola::cast<float>");
builder::dyn_var<double(double)> cast_double = builder::as_global("cola::cast<double>");

builder::dyn_var<uint8_t(uint8_t*,loop_type)> read_uint8_t = builder::as_global("cola::read<uint8_t>");
builder::dyn_var<uint16_t(uint16_t*,loop_type)> read_uint16_t = builder::as_global("cola::read<uint16_t>");
builder::dyn_var<uint32_t(uint32_t*,loop_type)> read_uint32_t = builder::as_global("cola::read<uint32_t>");
builder::dyn_var<uint64_t(uint64_t*,loop_type)> read_uint64_t = builder::as_global("cola::read<uint64_t>");
builder::dyn_var<int16_t(int16_t*,loop_type)> read_int16_t = builder::as_global("cola::read<int16_t>");
builder::dyn_var<int32_t(int32_t*,loop_type)> read_int32_t = builder::as_global("cola::read<int32_t>");
builder::dyn_var<int64_t(int64_t*,loop_type)> read_int64_t = builder::as_global("cola::read<int64_t>");
builder::dyn_var<float(float*,loop_type)> read_float = builder::as_global("cola::read<float>");
builder::dyn_var<double(double*,loop_type)> read_double = builder::as_global("cola::read<double>");

builder::dyn_var<void(uint8_t*,loop_type,uint8_t)> write_uint8_t = builder::as_global("cola::write<uint8_t>");
builder::dyn_var<void(uint16_t*,loop_type,uint16_t)> write_uint16_t = builder::as_global("cola::write<uint16_t>");
builder::dyn_var<void(uint32_t*,loop_type,uint32_t)> write_uint32_t = builder::as_global("cola::write<uint32_t>");
builder::dyn_var<void(uint64_t*,loop_type,uint64_t)> write_uint64_t = builder::as_global("cola::write<uint64_t>");
builder::dyn_var<void(int16_t*,loop_type,int16_t)> write_int16_t = builder::as_global("cola::write<int16_t>");
builder::dyn_var<void(int32_t*,loop_type,int32_t)> write_int32_t = builder::as_global("cola::write<int32_t>");
builder::dyn_var<void(int64_t*,loop_type,int64_t)> write_int64_t = builder::as_global("cola::write<int64_t>");
builder::dyn_var<void(float*,loop_type,float)> write_float = builder::as_global("cola::write<float>");
builder::dyn_var<void(double*,loop_type,double)> write_double = builder::as_global("cola::write<double>");

builder::dyn_var<uint8_t(HEAP_T<uint8_t>,loop_type)> read_heaparr_uint8_t = builder::as_global("cola::read<uint8_t>");
builder::dyn_var<uint16_t(HEAP_T<uint16_t>,loop_type)> read_heaparr_uint16_t = builder::as_global("cola::read<uint16_t>");
builder::dyn_var<uint32_t(HEAP_T<uint32_t>,loop_type)> read_heaparr_uint32_t = builder::as_global("cola::read<uint32_t>");
builder::dyn_var<uint64_t(HEAP_T<uint64_t>,loop_type)> read_heaparr_uint64_t = builder::as_global("cola::read<uint64_t>");
builder::dyn_var<int16_t(HEAP_T<int16_t>,loop_type)> read_heaparr_int16_t = builder::as_global("cola::read<int16_t>");
builder::dyn_var<int32_t(HEAP_T<int32_t>,loop_type)> read_heaparr_int32_t = builder::as_global("cola::read<int32_t>");
builder::dyn_var<int64_t(HEAP_T<int64_t>,loop_type)> read_heaparr_int64_t = builder::as_global("cola::read<int64_t>");
builder::dyn_var<float(HEAP_T<float>,loop_type)> read_heaparr_float = builder::as_global("cola::read<float>");
builder::dyn_var<double(HEAP_T<double>,loop_type)> read_heaparr_double = builder::as_global("cola::read<double>");

builder::dyn_var<void(HEAP_T<uint8_t>,loop_type,uint8_t)> write_heaparr_uint8_t = builder::as_global("cola::write<uint8_t>");
builder::dyn_var<void(HEAP_T<uint16_t>,loop_type,uint16_t)> write_heaparr_uint16_t = builder::as_global("cola::write<uint16_t>");
builder::dyn_var<void(HEAP_T<uint32_t>,loop_type,uint32_t)> write_heaparr_uint32_t = builder::as_global("cola::write<uint32_t>");
builder::dyn_var<void(HEAP_T<uint64_t>,loop_type,uint64_t)> write_heaparr_uint64_t = builder::as_global("cola::write<uint64_t>");
builder::dyn_var<void(HEAP_T<int16_t>,loop_type,int16_t)> write_heaparr_int16_t = builder::as_global("cola::write<int16_t>");
builder::dyn_var<void(HEAP_T<int32_t>,loop_type,int32_t)> write_heaparr_int32_t = builder::as_global("cola::write<int32_t>");
builder::dyn_var<void(HEAP_T<int64_t>,loop_type,int64_t)> write_heaparr_int64_t = builder::as_global("cola::write<int64_t>");
builder::dyn_var<void(HEAP_T<float>,loop_type,float)> write_heaparr_float = builder::as_global("cola::write<float>");
builder::dyn_var<void(HEAP_T<double>,loop_type,double)> write_heaparr_double = builder::as_global("cola::write<double>");

builder::dyn_var<void(uint8_t*,uint8_t,loop_type)> memset_uint8_t = builder::as_global("cola::hmemset<uint8_t>");
builder::dyn_var<void(uint16_t*,uint16_t,loop_type)> memset_uint16_t = builder::as_global("cola::hmemset<uint16_t>");
builder::dyn_var<void(uint32_t*,uint32_t,loop_type)> memset_uint32_t = builder::as_global("cola::hmemset<uint32_t>");
builder::dyn_var<void(uint64_t*,uint64_t,loop_type)> memset_uint64_t = builder::as_global("cola::hmemset<uint64_t>");
builder::dyn_var<void(int16_t*,int16_t,loop_type)> memset_int16_t = builder::as_global("cola::hmemset<int16_t>");
builder::dyn_var<void(int32_t*,int32_t,loop_type)> memset_int32_t = builder::as_global("cola::hmemset<int32_t>");
builder::dyn_var<void(int64_t*,int64_t,loop_type)> memset_int64_t = builder::as_global("cola::hmemset<int64_t>");
builder::dyn_var<void(float*,float,loop_type)> memset_float = builder::as_global("cola::hmemset<float>");
builder::dyn_var<void(double*,double,loop_type)> memset_double = builder::as_global("cola::hmemset<double>");

builder::dyn_var<void(HEAP_T<uint8_t>,uint8_t,loop_type)> memset_heaparr_uint8_t = builder::as_global("cola::hmemset<uint8_t>");
builder::dyn_var<void(HEAP_T<uint16_t>,uint16_t,loop_type)> memset_heaparr_uint16_t = builder::as_global("cola::hmemset<uint16_t>");
builder::dyn_var<void(HEAP_T<uint32_t>,uint32_t,loop_type)> memset_heaparr_uint32_t = builder::as_global("cola::hmemset<uint32_t>");
builder::dyn_var<void(HEAP_T<uint64_t>,uint64_t,loop_type)> memset_heaparr_uint64_t = builder::as_global("cola::hmemset<uint64_t>");
builder::dyn_var<void(HEAP_T<int16_t>,int16_t,loop_type)> memset_heaparr_int16_t = builder::as_global("cola::hmemset<int16_t>");
builder::dyn_var<void(HEAP_T<int32_t>,int32_t,loop_type)> memset_heaparr_int32_t = builder::as_global("cola::hmemset<int32_t>");
builder::dyn_var<void(HEAP_T<int64_t>,int64_t,loop_type)> memset_heaparr_int64_t = builder::as_global("cola::hmemset<int64_t>");
builder::dyn_var<void(HEAP_T<float>,float,loop_type)> memset_heaparr_float = builder::as_global("cola::hmemset<float>");
builder::dyn_var<void(HEAP_T<double>,double,loop_type)> memset_heaparr_double = builder::as_global("cola::hmemset<double>");

builder::dyn_var<HEAP_T<uint8_t>(loop_type)> build_heaparr_uint8_t = builder::as_global("cola::build_heaparr<uint8_t>");
builder::dyn_var<HEAP_T<uint16_t>(loop_type)> build_heaparr_uint16_t = builder::as_global("cola::build_heaparr<uint16_t>");
builder::dyn_var<HEAP_T<uint32_t>(loop_type)> build_heaparr_uint32_t = builder::as_global("cola::build_heaparr<uint32_t>");
builder::dyn_var<HEAP_T<uint64_t>(loop_type)> build_heaparr_uint64_t = builder::as_global("cola::build_heaparr<uint64_t>");
builder::dyn_var<HEAP_T<int16_t>(loop_type)> build_heaparr_int16_t = builder::as_global("cola::build_heaparr<int16_t>");
builder::dyn_var<HEAP_T<int32_t>(loop_type)> build_heaparr_int32_t = builder::as_global("cola::build_heaparr<int32_t>");
builder::dyn_var<HEAP_T<int64_t>(loop_type)> build_heaparr_int64_t = builder::as_global("cola::build_heaparr<int64_t>");
builder::dyn_var<HEAP_T<float>(loop_type)> build_heaparr_float = builder::as_global("cola::build_heaparr<float>");
builder::dyn_var<HEAP_T<double>(loop_type)> build_heaparr_double = builder::as_global("cola::build_heaparr<double>");

builder::dyn_var<void(uint8_t)> print_elem_uint8_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(uint16_t)> print_elem_uint16_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(uint32_t)> print_elem_uint32_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(uint64_t)> print_elem_uint64_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(int16_t)> print_elem_int16_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(int32_t)> print_elem_int32_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(int64_t)> print_elem_int64_t = builder::as_global("cola::print_elem");
builder::dyn_var<void(float)> print_elem_float = builder::as_global("cola::print_elem");
builder::dyn_var<void(double)> print_elem_double = builder::as_global("cola::print_elem");

//template <typename Arg0, typename...Args>
//buil scat_inner(builder::dyn_var<void*> &cur, Arg0 arg0, Args...args) {
//  cur = scat_items(cur, arg0);
//}

template <typename Arg0, typename...Args>
builder::dyn_var<void*> scat(Arg0 arg0, Args...args) {
  if constexpr (sizeof...(Args) == 0) {
    return arg0;
  } else {
    return scat_items(arg0, scat(args...));
  }
}

}
