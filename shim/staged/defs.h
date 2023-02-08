// -*-c++-*-

#pragma once

// If 1, apply optimizations. Otherwise skip them
#define ENABLE_OPTS 1

#include <array>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "builder/array.h"
#include "common/loop_type.h"

namespace shim {

#ifndef UNSTAGED
//#define dvar  builder::dyn_var
//#define svar builder::static_var
//#define darr builder::dyn_arr
template <typename T>
using dvar = builder::dyn_var<T>;

template <typename T>
using svar = builder::static_var<T>;

template <typename T, int Rank>
using darr = builder::dyn_arr<T,Rank>;
#else
template <typename T>
using dvar = T;

template <typename T>
using svar = T;

template <typename T, int Rank>
using darr = std::array<T,Rank>;
#endif

}
