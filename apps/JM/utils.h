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

#define PD(item) print(#item " = %d\\n", item)
#define RSHIFT_RND(x,a) ((a) > 0) ? (crshift((((x) + (clshift(1, ((a)-1) ))), (a)))) : (clshift((x), (-(a))))
#define RSHIFT_RND_SF(x,a) crshift(((x) + clshift(1, ((a)-1))), (a))

dint clip1Y(dint x);
