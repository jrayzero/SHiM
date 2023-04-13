// -*-c++-*-

#pragma once

#include <cstdint>
#include "builder/dyn_var.h"
#include "common/loop_type.hpp"
//#include "fwddecls.hpp"

// These are the wrappers for various functions that we want to generate calls for in the generated code.

// I think these need to be generated outside the staging itself, so I don't really know what a better way
// to write these is other than manually enumerating them.
namespace shim {

builder::dyn_var<void(int)> hexit = builder::as_global("exit");
builder::dyn_var<loop_type(loop_type)> hfloor = builder::as_global("floor");
builder::dyn_var<void(void)> print_newline = builder::as_global("print_newline");
builder::dyn_var<void(void*)> print = builder::as_global("printf");
builder::dyn_var<int(int,int)> pow = builder::as_global("pow");
builder::dyn_var<int(int)> ceil = builder::as_global("ceil");
builder::dyn_var<int(int)> cabs = builder::as_global("abs");
builder::dyn_var<void(bool,char*)> hassert = builder::as_global("SHIM_ASSERT");

builder::dyn_var<void*(void*,void*,void*)> ternary_cond_wrapper = builder::as_global("TERNARY");

builder::dyn_var<uint8_t(uint8_t)> cast_uint8_t = builder::as_global("SHIM_CAST_UINT8_T");
builder::dyn_var<uint16_t(uint16_t)> cast_uint16_t = builder::as_global("SHIM_CAST_UINT16_T");
builder::dyn_var<uint32_t(uint32_t)> cast_uint32_t = builder::as_global("SHIM_CAST_UINT32_T");
builder::dyn_var<uint64_t(uint64_t)> cast_uint64_t = builder::as_global("SHIM_CAST_UINT64_T");
builder::dyn_var<char(char)> cast_char = builder::as_global("SHIM_CAST_CHAR");
builder::dyn_var<int8_t(int8_t)> cast_int8_t = builder::as_global("SHIM_CAST_INT8_T");
builder::dyn_var<int16_t(int16_t)> cast_int16_t = builder::as_global("SHIM_CAST_INT16_T");
builder::dyn_var<int32_t(int32_t)> cast_int32_t = builder::as_global("SHIM_CAST_INT32_T");
builder::dyn_var<int64_t(int64_t)> cast_int64_t = builder::as_global("SHIM_CAST_INT64_T");
builder::dyn_var<float(float)> cast_float = builder::as_global("SHIM_CAST_FLOAT");
builder::dyn_var<double(double)> cast_double = builder::as_global("SHIM_CAST_DOUBLE");

builder::dyn_var<void(uint8_t*,uint8_t,loop_type)> memset_uint8_t = builder::as_global("shim::hmemset<uint8_t>");
builder::dyn_var<void(uint16_t*,uint16_t,loop_type)> memset_uint16_t = builder::as_global("shim::hmemset<uint16_t>");
builder::dyn_var<void(uint32_t*,uint32_t,loop_type)> memset_uint32_t = builder::as_global("shim::hmemset<uint32_t>");
builder::dyn_var<void(uint64_t*,uint64_t,loop_type)> memset_uint64_t = builder::as_global("shim::hmemset<uint64_t>");
builder::dyn_var<void(char*,char,loop_type)> memset_char = builder::as_global("shim::hmemset<char>");
builder::dyn_var<void(int8_t*,int8_t,loop_type)> memset_int8_t = builder::as_global("shim::hmemset<int8_t>");
builder::dyn_var<void(int16_t*,int16_t,loop_type)> memset_int16_t = builder::as_global("shim::hmemset<int16_t>");
builder::dyn_var<void(int32_t*,int32_t,loop_type)> memset_int32_t = builder::as_global("shim::hmemset<int32_t>");
builder::dyn_var<void(int64_t*,int64_t,loop_type)> memset_int64_t = builder::as_global("shim::hmemset<int64_t>");
builder::dyn_var<void(float*,float,loop_type)> memset_float = builder::as_global("shim::hmemset<float>");
builder::dyn_var<void(double*,double,loop_type)> memset_double = builder::as_global("shim::hmemset<double>");

/*builder::dyn_var<void(HEAP_T<uint8_t>,uint8_t,loop_type)> memset_heaparr_uint8_t = builder::as_global("shim::hmemset<uint8_t>");
builder::dyn_var<void(HEAP_T<uint16_t>,uint16_t,loop_type)> memset_heaparr_uint16_t = builder::as_global("shim::hmemset<uint16_t>");
builder::dyn_var<void(HEAP_T<uint32_t>,uint32_t,loop_type)> memset_heaparr_uint32_t = builder::as_global("shim::hmemset<uint32_t>");
builder::dyn_var<void(HEAP_T<uint64_t>,uint64_t,loop_type)> memset_heaparr_uint64_t = builder::as_global("shim::hmemset<uint64_t>");
builder::dyn_var<void(HEAP_T<char>,char,loop_type)> memset_heaparr_char = builder::as_global("shim::hmemset<char>");
builder::dyn_var<void(HEAP_T<int8_t>,int8_t,loop_type)> memset_heaparr_int8_t = builder::as_global("shim::hmemset<int8_t>");
builder::dyn_var<void(HEAP_T<int32_t>,int32_t,loop_type)> memset_heaparr_int32_t = builder::as_global("shim::hmemset<int32_t>");
builder::dyn_var<void(HEAP_T<int64_t>,int64_t,loop_type)> memset_heaparr_int64_t = builder::as_global("shim::hmemset<int64_t>");
builder::dyn_var<void(HEAP_T<float>,float,loop_type)> memset_heaparr_float = builder::as_global("shim::hmemset<float>");
builder::dyn_var<void(HEAP_T<double>,double,loop_type)> memset_heaparr_double = builder::as_global("shim::hmemset<double>");

builder::dyn_var<HEAP_T<uint8_t>(loop_type)> build_heaparr_uint8_t = builder::as_global("shim::build_heaparr<uint8_t>");
builder::dyn_var<HEAP_T<uint16_t>(loop_type)> build_heaparr_uint16_t = builder::as_global("shim::build_heaparr<uint16_t>");
builder::dyn_var<HEAP_T<uint32_t>(loop_type)> build_heaparr_uint32_t = builder::as_global("shim::build_heaparr<uint32_t>");
builder::dyn_var<HEAP_T<uint64_t>(loop_type)> build_heaparr_uint64_t = builder::as_global("shim::build_heaparr<uint64_t>");
builder::dyn_var<HEAP_T<char>(loop_type)> build_heaparr_char = builder::as_global("shim::build_heaparr<char>");
builder::dyn_var<HEAP_T<int8_t>(loop_type)> build_heaparr_int8_t = builder::as_global("shim::build_heaparr<int8_t>");
builder::dyn_var<HEAP_T<int16_t>(loop_type)> build_heaparr_int16_t = builder::as_global("shim::build_heaparr<int16_t>");
builder::dyn_var<HEAP_T<int32_t>(loop_type)> build_heaparr_int32_t = builder::as_global("shim::build_heaparr<int32_t>");
builder::dyn_var<HEAP_T<int64_t>(loop_type)> build_heaparr_int64_t = builder::as_global("shim::build_heaparr<int64_t>");
builder::dyn_var<HEAP_T<float>(loop_type)> build_heaparr_float = builder::as_global("shim::build_heaparr<float>");
builder::dyn_var<HEAP_T<double>(loop_type)> build_heaparr_double = builder::as_global("shim::build_heaparr<double>");
*/
builder::dyn_var<uint8_t*(loop_type)> build_stackarr_uint8_t = builder::as_global("SHIM_BUILD_STACK_UINT8_T");
builder::dyn_var<uint16_t*(loop_type)> build_stackarr_uint16_t = builder::as_global("SHIM_BUILD_STACK_UINT16_T");
builder::dyn_var<uint32_t*(loop_type)> build_stackarr_uint32_t = builder::as_global("SHIM_BUILD_STACK_UINT32_T");
builder::dyn_var<uint64_t*(loop_type)> build_stackarr_uint64_t = builder::as_global("SHIM_BUILD_STACK_UINT64_T");
builder::dyn_var<char*(loop_type)> build_stackarr_char = builder::as_global("SHIM_BUILD_STACK_CHAR");
builder::dyn_var<int8_t*(loop_type)> build_stackarr_int8_t = builder::as_global("SHIM_BUILD_STACK_INT8_T");
builder::dyn_var<int16_t*(loop_type)> build_stackarr_int16_t = builder::as_global("SHIM_BUILD_STACK_INT16_T");
builder::dyn_var<int32_t*(loop_type)> build_stackarr_int32_t = builder::as_global("SHIM_BUILD_STACK_INT32_T");
builder::dyn_var<int64_t*(loop_type)> build_stackarr_int64_t = builder::as_global("SHIM_BUILD_STACK_INT64_T");
builder::dyn_var<float*(loop_type)> build_stackarr_float = builder::as_global("SHIM_BUILD_STACK_FLOAT");
builder::dyn_var<double*(loop_type)> build_stackarr_double = builder::as_global("SHIM_BUILD_STACK_DOUBLE");

builder::dyn_var<void(uint8_t)> print_elem_uint8_t = builder::as_global("shim::print_elem<uint8_t>");
builder::dyn_var<void(uint16_t)> print_elem_uint16_t = builder::as_global("shim::print_elem<uint16_t>");
builder::dyn_var<void(uint32_t)> print_elem_uint32_t = builder::as_global("shim::print_elem<uint32_t>");
builder::dyn_var<void(unsigned long)> print_elem_unsigned = builder::as_global("shim::print_elem<unsigned long>");
builder::dyn_var<void(uint64_t)> print_elem_uint64_t = builder::as_global("shim::print_elem<uint64_t>");
builder::dyn_var<void(char)> print_elem_char = builder::as_global("shim::print_elem<char>");
builder::dyn_var<void(int8_t)> print_elem_int8_t = builder::as_global("shim::print_elem<int8_t>");
builder::dyn_var<void(int16_t)> print_elem_int16_t = builder::as_global("shim::print_elem<int16_t>");
builder::dyn_var<void(int32_t)> print_elem_int32_t = builder::as_global("shim::print_elem<int32_t>");
builder::dyn_var<void(int64_t)> print_elem_int64_t = builder::as_global("shim::print_elem<int64_t>");
builder::dyn_var<void(float)> print_elem_float = builder::as_global("shim::print_elem<float>");
builder::dyn_var<void(double)> print_elem_double = builder::as_global("shim::print_elem<double>");

}
