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
builder::dyn_var<void(void*)> cprint = builder::as_global("printf");
builder::dyn_var<void(void*)> printn = builder::as_global("cola::printn");
builder::dyn_var<void*(void*,void*)> scat_items = builder::as_global("cola::scat_items");
builder::dyn_var<int(int,int)> lshift = builder::as_global("cola::lshift");
builder::dyn_var<int(int,int)> rshift = builder::as_global("cola::rshift");
builder::dyn_var<int(int,int)> clshift = builder::as_global("lshift");
builder::dyn_var<int(int,int)> crshift = builder::as_global("rshift");
builder::dyn_var<int(int,int)> bor = builder::as_global("cola::bor");
builder::dyn_var<int(int,int)> band = builder::as_global("cola::band");
builder::dyn_var<int(int,int)> pow = builder::as_global("cola::pow");
builder::dyn_var<int(int)> ceil = builder::as_global("cola::ceil");
builder::dyn_var<int(int)> cabs = builder::as_global("abs");
builder::dyn_var<void(bool,char*)> hassert = builder::as_global("cola::cola_assert");

// Note: the arg tyoe for these casts isn't right, but I don't want to write every combination
// of dyn_var<to(from)>, and buildit doesn't seem to care.
builder::dyn_var<uint8_t(uint8_t)> cast_uint8_t = builder::as_global("cola::cast<uint8_t>");
builder::dyn_var<uint16_t(uint16_t)> cast_uint16_t = builder::as_global("cola::cast<uint16_t>");
builder::dyn_var<uint32_t(uint32_t)> cast_uint32_t = builder::as_global("cola::cast<uint32_t>");
builder::dyn_var<uint64_t(uint64_t)> cast_uint64_t = builder::as_global("cola::cast<uint64_t>");
builder::dyn_var<char(char)> cast_char = builder::as_global("cola::cast<char>");
builder::dyn_var<int8_t(int8_t)> cast_int8_t = builder::as_global("cola::cast<int8_t>");
builder::dyn_var<int16_t(int16_t)> cast_int16_t = builder::as_global("cola::cast<int16_t>");
builder::dyn_var<int32_t(int32_t)> cast_int32_t = builder::as_global("cola::cast<int32_t>");
builder::dyn_var<int64_t(int64_t)> cast_int64_t = builder::as_global("cola::cast<int64_t>");
builder::dyn_var<float(float)> cast_float = builder::as_global("cola::cast<float>");
builder::dyn_var<double(double)> cast_double = builder::as_global("cola::cast<double>");

builder::dyn_var<void(uint8_t*,uint8_t,loop_type)> memset_uint8_t = builder::as_global("cola::hmemset<uint8_t>");
builder::dyn_var<void(uint16_t*,uint16_t,loop_type)> memset_uint16_t = builder::as_global("cola::hmemset<uint16_t>");
builder::dyn_var<void(uint32_t*,uint32_t,loop_type)> memset_uint32_t = builder::as_global("cola::hmemset<uint32_t>");
builder::dyn_var<void(uint64_t*,uint64_t,loop_type)> memset_uint64_t = builder::as_global("cola::hmemset<uint64_t>");
builder::dyn_var<void(char*,char,loop_type)> memset_char = builder::as_global("cola::hmemset<char>");
builder::dyn_var<void(int8_t*,int8_t,loop_type)> memset_int8_t = builder::as_global("cola::hmemset<int8_t>");
builder::dyn_var<void(int16_t*,int16_t,loop_type)> memset_int16_t = builder::as_global("cola::hmemset<int16_t>");
builder::dyn_var<void(int32_t*,int32_t,loop_type)> memset_int32_t = builder::as_global("cola::hmemset<int32_t>");
builder::dyn_var<void(int64_t*,int64_t,loop_type)> memset_int64_t = builder::as_global("cola::hmemset<int64_t>");
builder::dyn_var<void(float*,float,loop_type)> memset_float = builder::as_global("cola::hmemset<float>");
builder::dyn_var<void(double*,double,loop_type)> memset_double = builder::as_global("cola::hmemset<double>");

builder::dyn_var<void(HEAP_T<uint8_t>,uint8_t,loop_type)> memset_heaparr_uint8_t = builder::as_global("cola::hmemset<uint8_t>");
builder::dyn_var<void(HEAP_T<uint16_t>,uint16_t,loop_type)> memset_heaparr_uint16_t = builder::as_global("cola::hmemset<uint16_t>");
builder::dyn_var<void(HEAP_T<uint32_t>,uint32_t,loop_type)> memset_heaparr_uint32_t = builder::as_global("cola::hmemset<uint32_t>");
builder::dyn_var<void(HEAP_T<uint64_t>,uint64_t,loop_type)> memset_heaparr_uint64_t = builder::as_global("cola::hmemset<uint64_t>");
builder::dyn_var<void(HEAP_T<char>,char,loop_type)> memset_heaparr_char = builder::as_global("cola::hmemset<char>");
builder::dyn_var<void(HEAP_T<int8_t>,int8_t,loop_type)> memset_heaparr_int8_t = builder::as_global("cola::hmemset<int8_t>");
builder::dyn_var<void(HEAP_T<int32_t>,int32_t,loop_type)> memset_heaparr_int32_t = builder::as_global("cola::hmemset<int32_t>");
builder::dyn_var<void(HEAP_T<int64_t>,int64_t,loop_type)> memset_heaparr_int64_t = builder::as_global("cola::hmemset<int64_t>");
builder::dyn_var<void(HEAP_T<float>,float,loop_type)> memset_heaparr_float = builder::as_global("cola::hmemset<float>");
builder::dyn_var<void(HEAP_T<double>,double,loop_type)> memset_heaparr_double = builder::as_global("cola::hmemset<double>");

builder::dyn_var<HEAP_T<uint8_t>(loop_type)> build_heaparr_uint8_t = builder::as_global("cola::build_heaparr<uint8_t>");
builder::dyn_var<HEAP_T<uint16_t>(loop_type)> build_heaparr_uint16_t = builder::as_global("cola::build_heaparr<uint16_t>");
builder::dyn_var<HEAP_T<uint32_t>(loop_type)> build_heaparr_uint32_t = builder::as_global("cola::build_heaparr<uint32_t>");
builder::dyn_var<HEAP_T<uint64_t>(loop_type)> build_heaparr_uint64_t = builder::as_global("cola::build_heaparr<uint64_t>");
builder::dyn_var<HEAP_T<char>(loop_type)> build_heaparr_char = builder::as_global("cola::build_heaparr<char>");
builder::dyn_var<HEAP_T<int8_t>(loop_type)> build_heaparr_int8_t = builder::as_global("cola::build_heaparr<int8_t>");
builder::dyn_var<HEAP_T<int16_t>(loop_type)> build_heaparr_int16_t = builder::as_global("cola::build_heaparr<int16_t>");
builder::dyn_var<HEAP_T<int32_t>(loop_type)> build_heaparr_int32_t = builder::as_global("cola::build_heaparr<int32_t>");
builder::dyn_var<HEAP_T<int64_t>(loop_type)> build_heaparr_int64_t = builder::as_global("cola::build_heaparr<int64_t>");
builder::dyn_var<HEAP_T<float>(loop_type)> build_heaparr_float = builder::as_global("cola::build_heaparr<float>");
builder::dyn_var<HEAP_T<double>(loop_type)> build_heaparr_double = builder::as_global("cola::build_heaparr<double>");

builder::dyn_var<void(uint8_t)> print_elem_uint8_t = builder::as_global("cola::print_elem<uint8_t>");
builder::dyn_var<void(uint16_t)> print_elem_uint16_t = builder::as_global("cola::print_elem<uint16_t>");
builder::dyn_var<void(uint32_t)> print_elem_uint32_t = builder::as_global("cola::print_elem<uint32_t>");
builder::dyn_var<void(unsigned long)> print_elem_unsigned = builder::as_global("cola::print_elem<unsigned long>");
builder::dyn_var<void(uint64_t)> print_elem_uint64_t = builder::as_global("cola::print_elem<uint64_t>");
builder::dyn_var<void(char)> print_elem_char = builder::as_global("cola::print_elem<char>");
builder::dyn_var<void(int8_t)> print_elem_int8_t = builder::as_global("cola::print_elem<int8_t>");
builder::dyn_var<void(int16_t)> print_elem_int16_t = builder::as_global("cola::print_elem<int16_t>");
builder::dyn_var<void(int32_t)> print_elem_int32_t = builder::as_global("cola::print_elem<int32_t>");
builder::dyn_var<void(int64_t)> print_elem_int64_t = builder::as_global("cola::print_elem<int64_t>");
builder::dyn_var<void(float)> print_elem_float = builder::as_global("cola::print_elem<float>");
builder::dyn_var<void(double)> print_elem_double = builder::as_global("cola::print_elem<double>");

template <typename Arg0, typename...Args>
builder::dyn_var<void*> scat(Arg0 arg0, Args...args) {
  if constexpr (sizeof...(Args) == 0) {
    return arg0;
  } else {
    return scat_items(arg0, scat(args...));
  }
}

}
