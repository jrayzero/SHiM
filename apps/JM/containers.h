// -*-c++-*-

#pragma once

#include "staged/object.h"

#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/dyn_var.h"
#include "builder/array.h"

// within JM
#include "defines.h"
#include "typedefs.h"


using namespace cola;
using builder::dyn_var;

constexpr char macroblock_name[] = "Macroblock";
constexpr char videoparam_name[] = "VideoParameters";
constexpr char inputparam_name[] = "InputParameters";
constexpr char storablepic_name[] = "StorablePicture";
constexpr char frameformat_name[] = "FrameFormat";
constexpr char slice_name[] = "Slice";
using macroblock_t = typename builder::name<macroblock_name>;
using videoparam_t = typename builder::name<videoparam_name>;
using inputparam_t = typename builder::name<inputparam_name>;
using storablepic_t = typename builder::name<storablepic_name>;
using frameformat_t = typename builder::name<frameformat_name>;
using slice_t = typename builder::name<slice_name>;

namespace builder {

template <>
class dyn_var<storablepic_t> : public dyn_var_impl<storablepic_t> {
public:
  DYN_VAR_BP(storablepic_t)
  dyn_var<imgpel**> p_curr_img = as_member_of(this, "p_curr_img");
};
DYN_VAR_PTR(storablepic_t)

template <>
class dyn_var<frameformat_t> : public dyn_var_impl<frameformat_t> {
public:
  DYN_VAR_BP(frameformat_t)
  dyn_var<short*> height = as_member_of(this, "height");
  dyn_var<short*> width = as_member_of(this, "width");
  dyn_var<short> mb_height = as_member_of(this, "mb_height");
  dyn_var<short> mb_width = as_member_of(this, "mb_width");
};

template <>
class dyn_var<videoparam_t> : public dyn_var_impl<videoparam_t> {
public:
  DYN_VAR_BP(videoparam_t)
  dyn_var<imgpel**> p_cur_img = as_member_of(this, "pCurImg");
  dyn_var<storablepic_t*> enc_picture = as_member_of(this, "enc_picture");
  dyn_var<short*> intra_block = as_member_of(this, "intra_block");
};
DYN_VAR_PTR(videoparam_t)

template <>
class dyn_var<inputparam_t> : public dyn_var_impl<inputparam_t> {
public:
  DYN_VAR_BP(inputparam_t)
  dyn_var<frameformat_t> source = as_member_of(this, "source");
  dyn_var<int> use_constrained_intra_pred = as_member_of(this, "UseConstrainedIntraPred");
  dyn_var<int> intra_disable_inter_only = as_member_of(this, "IntraDisableInterOnly");
  dyn_var<int> intra_16x16_hv_disable = as_member_of(this, "Intra16x16ParDisable");
  dyn_var<int> intra_16x16_plane_disable = as_member_of(this, "Intra16x16PlaneDisable");
};
DYN_VAR_PTR(inputparam_t)

template <>
class dyn_var<slice_t> : public dyn_var_impl<slice_t> {
public:
  DYN_VAR_BP(slice_t)
  dyn_var<int> P444_joined = as_member_of(this, "P444_joined");
  dyn_var<int> slice_type = as_member_of(this, "slice_type");
  dyn_var<imgpel****> mpr_16x16 = as_member_of(this, "mpr_16x16");
};
DYN_VAR_PTR(slice_t)

template <>
class dyn_var<macroblock_t> : public dyn_var_impl<macroblock_t> {
public:
  DYN_VAR_BP(macroblock_t)

  dyn_var<inputparam_t*> p_inp = as_member_of(this, "p_Inp");
  dyn_var<videoparam_t*> p_vid = as_member_of(this, "p_Vid");
  dyn_var<slice_t*> p_slice = as_member_of(this, "p_Slice");
  dyn_var<short> pix_y = as_member_of(this, "pix_y");
  dyn_var<short> pix_x = as_member_of(this, "pix_x");
  dyn_var<int> mb_addr_B_available = as_member_of(this, "mbAvailB");
  dyn_var<int> mb_addr_A_available = as_member_of(this, "mbAvailA");
  dyn_var<int> mb_addr_D_available = as_member_of(this, "mbAvailD");
  dyn_var<char> i16_mode = as_member_of(this, "i16mode");
};
DYN_VAR_PTR(macroblock_t)
  
}

using Macroblock = builder::dyn_var<macroblock_t*>;
