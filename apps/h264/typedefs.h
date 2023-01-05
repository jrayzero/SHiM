#pragma once

using cursor = builder::dyn_var<uint64_t>;
using dint = builder::dyn_var<int>;
template <typename T>
using dptr = builder::dyn_var<T*>;
template <typename T, int Sz>
using darr = builder::dyn_var<T[Sz]>;
