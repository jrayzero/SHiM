#include <sstream>
#include <iostream>
#include <map>
#include "staged/staged.h"
#include "staged/bitstream.h"
#include "typedefs.h"
#include "structs.h"

using namespace std;
using namespace cola;

// Optimization ideas to justify staging
// - determine alignment and pick whether to use aligned or unaligned parse

SeqParameterSet sps(Bitstream &);
PicParameterSet pps(Bitstream &);
//void slice_header(Bitstream &, map<int,SeqParameterSet&> &, map<int,PicParameterSet&> &);
//void slice_data(Bitstream &, map<int,SeqParameterSet&> &, map<int,PicParameterSet&> &);

// TODO wrapper for stringstream with dyn_var so can make better error messages
template <typename C, typename T, typename U>
void check(C cursor, T got, U should_be, string elem) {
  if (got != should_be) {
    printn("Invalid value for syntax element <", elem, "> starting at cursor <", cursor, ">. Got ", got, ", expected ", should_be);
    hexit(-1);
  }
}

// NEED THE REFERENCE HERE SINCE THE CURSOR INSIDE THE BITSTREAM CHANGES! This is unlike HMDAs
void nal_unit(Bitstream &bitstream) {
  if (bitstream.exists(8)==0) {
    print_string("Expected at least 8 bits for nal_unit header");
    hexit(-1);
  }
  check(bitstream.cursor, bitstream.pop(1), 0, "forbidden_zero_bit");
  dint nal_ref_idc = bitstream.pop<int>(2); 
  dint nal_unit_type = bitstream.pop<int>(5);
  printn("<nal_ref_idc> = ", nal_ref_idc);
  printn("<nal_unit_type> = ", nal_unit_type);
  // remove all the padding bytes and overwrite the payload  
  cursor payload_idx = bitstream.cursor / 8;
  printn("Saving cursor");
  cursor starting_cursor = bitstream.cursor;
  printn("Done saving cursor");
  dint bytes_in_payload = 0;
  dint cond = 1;
  while (cond) {
    if (bitstream.peek_aligned<int>(24) == 0x000001) {
      cond = 0;
    } else if (bitstream.peek_aligned<int>(32) == 0x00000001) {
      cond = 0;
    } else {
      if (bitstream.peek_aligned<int>(24) == 0x000003) {
	// emulation (stuffing) bits
	bitstream.bitstream[payload_idx] = bitstream.pop(8);
	bitstream.bitstream[payload_idx+1] = bitstream.pop(8);
	payload_idx = payload_idx + 2;
	bytes_in_payload = bytes_in_payload + 2;
	check(bitstream.cursor, bitstream.pop(8), 0x03, "emulation_prevention_three_byte");
      } else {
	bitstream.bitstream[payload_idx] = bitstream.pop(8);
	payload_idx = payload_idx + 1;
	bytes_in_payload = bytes_in_payload + 1;
      }
    }
  }
  printn("Read ", bytes_in_payload, " bytes in nal unit payload.");
  // save the cursor before we process the payload according to the nal unit type
  cursor upper_cursor = bitstream.cursor;
  printn("Setting cursor");
  bitstream.cursor = starting_cursor;
  printn("cursor start ", bitstream.cursor);
  // TODO PROCESS PAYLOAD
  if (nal_unit_type == 7) {
    SeqParameterSet seq = sps(bitstream);
//    dint id = seq->seq_parameter_set_id;
//    if (seqs.count(
//    seqs[] = move(seq);
  } else if (nal_unit_type == 8) {
//    PicParameterSet pic = pps(bitstream);
//    pics[pic->pic_parameter_set_id] = move(pic);
  } else {
    printn("Unsupported <nal_unit_type> = ", nal_unit_type);
    hexit(48);
  }
  // now reset the cursor to parse the next nal unit
  bitstream.cursor = upper_cursor;
  printn("Cursor ended at ", bitstream.cursor);
}

void annexB(Bitstream &bitstream) {
  dint foo = 0;
  dint bar = 0;
  dint peek24 = bitstream.peek_aligned<int>(24);
  dint peek32 = bitstream.peek_aligned<int>(32);
  while (peek24 != 0x000001 && peek32 != 0x00000001) {
    peek24 = bitstream.peek_aligned<int>(24);
    peek32 = bitstream.peek_aligned<int>(32);
    check(bitstream.cursor, bitstream.pop(8), 0x00, "leading_zero_8bits");
  }
  if (bitstream.peek_aligned(24) != 0x000001) {
    check(bitstream.cursor, bitstream.pop(8), 0x00, "zero_byte");
  }
  check(bitstream.cursor, bitstream.pop(24), 0x000001, "start_code_prefix_one_3bytes");
  nal_unit(bitstream);
  peek24 = bitstream.peek_aligned<int>(24);
  peek32 = bitstream.peek_aligned<int>(32);
  while (peek24 != 0x000001 && peek32 != 0x00000001) {
    peek24 = bitstream.peek_aligned<int>(24);
    peek32 = bitstream.peek_aligned<int>(32);
    check(bitstream.cursor, bitstream.pop(8), 0x00, "trailing_zero_8bits");
  }
}

dint ue(Bitstream &bitstream) {
  dint leading_zero_bits = -1;
  dint b = 0;
  dint cond = 1;
  
  while (b == 0) {
    b = bitstream.pop(1);
    leading_zero_bits = leading_zero_bits+1;
  }
  // If I inline this into code_num, it seg faults.
  dint popped = bitstream.pop(leading_zero_bits);
  dint code_num = lshift(1, leading_zero_bits) - 1 + popped;
  return code_num;
}

dint se(Bitstream &bitstream) {
  dint code_num = ue(bitstream);
  return cola::pow(1, code_num+1) * cola::ceil(code_num/2);
}

SeqParameterSet sps(Bitstream &bitstream) {
  SeqParameterSet sps;
  printn("cursor ", bitstream.cursor);
  sps.profile_idc = bitstream.pop(8);
  sps.constraint_set0_flag = bitstream.pop(1);
  sps.constraint_set1_flag = bitstream.pop(1);
  sps.constraint_set2_flag = bitstream.pop(1);
  sps.constraint_set3_flag = bitstream.pop(1);
  sps.constraint_set4_flag = bitstream.pop(1);
  sps.constraint_set5_flag = bitstream.pop(1);
  check(bitstream.cursor, bitstream.pop(2), 0x0, "reserved_zero_2bits");
  sps.level_idc = bitstream.pop(8);
  sps.seq_parameter_set_id = ue(bitstream);
  dint pidc = sps.profile_idc;
  if (pidc == 100 || pidc == 110 || pidc == 122 || pidc == 244 ||
      pidc == 44 || pidc == 83 || pidc == 86 || pidc == 118 || 
      pidc == 128 || pidc == 138 || pidc == 139 || pidc == 134 ||
      pidc == 135) {
    sps.other_params = 1;
    sps.chroma_format_idc = ue(bitstream);
    if (sps.chroma_format_idc == 3) {
      sps.separate_colour_plane_flag = bitstream.pop(1);
    }
    sps.bit_depth_luma_minus8 = ue(bitstream);
    sps.bit_depth_chroma_minus8 = ue(bitstream);
    sps.qpprime_y_zero_transform_bypass_flag = bitstream.pop(1);
    sps.seq_scaling_matrix_present_flag = bitstream.pop(1);
    if (sps.seq_scaling_matrix_present_flag == 1) {
      printn("sps scaling matrix not supported.");
      hexit(48);
    }
  } else {
    sps.other_params = 0;
  }
  sps.log2_max_frame_num_minus4 = ue(bitstream);
  sps.pic_order_cnt_type = ue(bitstream);
  if (sps.pic_order_cnt_type == 0) {
    sps.log2_max_pic_order_cnt_lsb_minus4 = ue(bitstream);
  } else if (sps.pic_order_cnt_type == 1) {
    sps.delta_pic_order_always_zero_flag = ue(bitstream);
    sps.offset_for_non_ref_pic = bitstream.pop(1);
    sps.offset_for_top_to_bottom_field = se(bitstream);
    sps.num_ref_frames_in_pic_order_cnt_cycle = ue(bitstream);
    for (dint i = 0; i < sps.num_ref_frames_in_pic_order_cnt_cycle; i=i+1) {
      sps.offset_for_ref_frame[i] = se(bitstream);
    }
  }
  sps.max_num_ref_frames = ue(bitstream);
  sps.gaps_in_frame_num_value_allowed_flag = bitstream.pop(1);
  sps.pic_width_in_mbs_minus1 = ue(bitstream);
  sps.pic_height_in_map_units_minus1 = ue(bitstream);
  sps.frame_mbs_only_flag = bitstream.pop(1);
  if (!sps.frame_mbs_only_flag) {
    sps.mb_adaptive_frame_field_flag = bitstream.pop(1);
  }
  sps.direct_8x8_inference_flag = bitstream.pop(1);
  sps.frame_cropping_flag = bitstream.pop(1);
  if (sps.frame_cropping_flag) {
    sps.frame_crop_left_offset = ue(bitstream);
    sps.frame_crop_right_offset = ue(bitstream);
    sps.frame_crop_top_offset = ue(bitstream);
    sps.frame_crop_bottom_offset = ue(bitstream);
  }
  sps.vui_parameters_present_flag = bitstream.pop(1);
  if (sps.vui_parameters_present_flag) {
    printn("VUI parameters not supported");
    hexit(48);
  }
  
  
  sps.dump();
  return sps;
}

PicParameterSet pps(Bitstream &bitstream) {
  PicParameterSet pps;
  pps.pic_parameter_set_id = ue(bitstream);
  pps.seq_parameter_set_id = ue(bitstream);
  pps.entropy_coding_mode_flag = bitstream.pop(1);
  pps.bottom_field_pic_order_in_frame_present_flag = bitstream.pop(1);
  pps.num_slice_groups_minus1 = ue(bitstream);
  if (pps.num_slice_groups_minus1 > 0) {
    printn("More than 1 slice group unsupported.");
    hexit(48);
  }
  pps.num_ref_idx_l0_default_active_minus1 = ue(bitstream);
  pps.num_ref_idx_l1_default_active_minus1 = ue(bitstream);
  pps.weighted_pred_flag = bitstream.pop(1);
  pps.weighted_bipred_idc = bitstream.pop(2);
  pps.pic_init_qp_minus26 = se(bitstream);
  pps.pic_init_qs_minus26 = se(bitstream);
  pps.chroma_qp_index_offset = se(bitstream);
  pps.deblocking_filter_control_present_flag = bitstream.pop(1);
  pps.constrained_intra_pred_flag = bitstream.pop(1);
  pps.redundant_pic_cnt_present_flag = bitstream.pop(1);
  if (bitstream.peek(1) == 0) {
    printn("Other pps stuff not supported");
    hexit(48);
  } // else rbsp trailing bits
  pps.dump();
  return pps; 
}

//void slice_layer_without_partition_rbsp(Bitstream &bitstream, map<int, SeqParameterSet> &seqs, map<int, PicParameterSet> &pics) {
//  slice_header(bitstream, seqs, pics);
//  slice_data(bitstream, seqs, pics);
//}

void parse_h264(builder::dyn_var<uint8_t*> buffer, builder::dyn_var<uint64_t> length) {
  auto bitstream = Bitstream(buffer, length);
  while (bitstream.exists(8) == 1) {
    annexB(bitstream);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage: ./sh264 <output_fn>" << endl;
    exit(-1);
  }
  chrono::steady_clock::time_point start = chrono::steady_clock::now();
  ofstream src;
  ofstream hdr;
  string source = string(argv[1]) + ".cpp";
  string header = string(argv[1]) + ".h";
  src.open(source);
  hdr.open(header);
  hdr << "#pragma once" << endl;
  hdr << "#include <cmath>" << endl;
  hdr << "void parse_h264(uint8_t *data, uint64_t length);" << endl;
  hdr.flush();
  hdr.close();
  src << "#include \"" << header << "\"" << endl;
  src << "#include \"runtime/runtime.h\"" << endl;
  stringstream ss;
  stage(parse_h264, "parse_h264", ss);
  src << ss.str() << endl;
  src.flush();
  src.close();
  chrono::steady_clock::time_point stop = chrono::steady_clock::now();
  cout << "Staging took " << chrono::duration_cast<chrono::nanoseconds> (stop - start).count()/1e9 << "s" << endl;    
}
