// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"

using builder::static_var;
using builder::dyn_var;
using builder::dyn_arr;
using sint = static_var<int>;
using dint = dyn_var<int>;
using dshort = dyn_var<short>;
using dbool = dyn_var<bool>;

#define CLIP1Y(x) select((x) < 0, 0, select((x) > 128, 128, (x)))

#define PD(item) print(#item " = %d\\n", item)
#define RSHIFT_RND(x,a) ((a) > 0) ? (((((x) + ((1 << ((a)-1) ))) >> (a)))) : (((x) << (-(a))))
#define RSHIFT_RND_SF(x,a) (((x) + (1 << ((a)-1))) >> (a))
