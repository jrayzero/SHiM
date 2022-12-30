#pragma once

#include "typedefs.h"

#define PRINT(name) \
  printn("<" #name "> = ", name);
  
#define PRINT_ARR(name, bound)			\
  print("<" #name "> = [");			\
  for (dint i = 0; i < bound; i=i+1) {		\
    if (i == 0) print(name[i]);			\
    else print(", ", name[i]);			\
  }						\
  printn("]");

struct SeqParameterSet {

  dint profile_idc = 0;
  dint constraint_set0_flag = 0;
  dint constraint_set1_flag = 0;
  dint constraint_set2_flag = 0;
  dint constraint_set3_flag = 0;
  dint constraint_set4_flag = 0;
  dint constraint_set5_flag = 0;
  dint level_idc = 0;
  dint seq_parameter_set_id = 0;  
  dint other_params = 0;
  dint chroma_format_idc = 0;
  dint separate_colour_plane_flag = 0;
  dint bit_depth_luma_minus8 = 0;
  dint bit_depth_chroma_minus8 = 0;
  dint qpprime_y_zero_transform_bypass_flag = 0;
  dint seq_scaling_matrix_present_flag = 0;  
  dint log2_max_frame_num_minus4 = 0;
  dint pic_order_cnt_type = 0;
  dint log2_max_pic_order_cnt_lsb_minus4 = 0;
  dint delta_pic_order_always_zero_flag = 0;
  dint offset_for_non_ref_pic = 0;
  dint offset_for_top_to_bottom_field = 0;
  dint num_ref_frames_in_pic_order_cnt_cycle = 0;
  darr<int,256> offset_for_ref_frame;
  dint max_num_ref_frames = 0;
  dint gaps_in_frame_num_value_allowed_flag = 0;
  dint pic_width_in_mbs_minus1 = 0;
  dint pic_height_in_map_units_minus1 = 0;
  dint frame_mbs_only_flag = 0;
  dint mb_adaptive_frame_field_flag = 0;
  dint direct_8x8_inference_flag = 0;
  dint frame_cropping_flag = 0;
  dint frame_crop_left_offset = 0;
  dint frame_crop_right_offset = 0;
  dint frame_crop_top_offset = 0;
  dint frame_crop_bottom_offset = 0;
  dint vui_parameters_present_flag = 0;

  void dump() {
    printn("=== SeqParameterSet");
    PRINT(profile_idc);
    PRINT(constraint_set0_flag);
    PRINT(constraint_set1_flag);
    PRINT(constraint_set2_flag);
    PRINT(constraint_set3_flag);
    PRINT(constraint_set4_flag);
    PRINT(constraint_set5_flag);
    PRINT(level_idc);
    PRINT(seq_parameter_set_id);  
    if (other_params) {
      PRINT(chroma_format_idc);
      PRINT(separate_colour_plane_flag);
      PRINT(bit_depth_luma_minus8);
      PRINT(bit_depth_chroma_minus8);
      PRINT(qpprime_y_zero_transform_bypass_flag);
      PRINT(seq_scaling_matrix_present_flag);
    }
    PRINT(log2_max_frame_num_minus4);
    PRINT(pic_order_cnt_type);
    PRINT(log2_max_pic_order_cnt_lsb_minus4);
    PRINT(delta_pic_order_always_zero_flag);
    PRINT(offset_for_non_ref_pic);
    PRINT(offset_for_top_to_bottom_field);
    PRINT(num_ref_frames_in_pic_order_cnt_cycle);
    PRINT_ARR(offset_for_ref_frame, num_ref_frames_in_pic_order_cnt_cycle);
    PRINT(max_num_ref_frames);
    PRINT(gaps_in_frame_num_value_allowed_flag);
    PRINT(pic_width_in_mbs_minus1);
    PRINT(pic_height_in_map_units_minus1);
    PRINT(frame_mbs_only_flag);
    PRINT(mb_adaptive_frame_field_flag);
    PRINT(direct_8x8_inference_flag);
    PRINT(frame_cropping_flag);
    PRINT(frame_crop_left_offset);
    PRINT(frame_crop_right_offset);
    PRINT(frame_crop_top_offset);
    PRINT(frame_crop_bottom_offset);
    PRINT(vui_parameters_present_flag);
  }

};

struct PicParameterSet {
  dint pic_parameter_set_id = 0;
  dint seq_parameter_set_id = 0;
  dint entropy_coding_mode_flag = 0;
  dint bottom_field_pic_order_in_frame_present_flag = 0;
  dint num_slice_groups_minus1 = 0;
  dint slice_group_map_type = 0;
  darr<int,8> run_length_minus1;
  darr<int,7> top_left;
  darr<int,7> bottom_right;
  dint slice_group_change_direction_flag = 0;
  dint slice_group_change_rate_minus1 = 0;
  dint pic_size_in_map_units_minus1 = 0;
  dptr<int> slice_group_id;
  dint num_ref_idx_l0_default_active_minus1 = 0;
  dint num_ref_idx_l1_default_active_minus1 = 0;
  dint weighted_pred_flag = 0;
  dint weighted_bipred_idc = 0;
  dint pic_init_qp_minus26 = 0;
  dint pic_init_qs_minus26 = 0;
  dint chroma_qp_index_offset = 0;
  dint deblocking_filter_control_present_flag = 0;
  dint constrained_intra_pred_flag = 0;
  dint redundant_pic_cnt_present_flag = 0;

  void dump() {
    printn("=== PicParameterSet");
    PRINT(pic_parameter_set_id);
    PRINT(seq_parameter_set_id);
    PRINT(entropy_coding_mode_flag);
    PRINT(bottom_field_pic_order_in_frame_present_flag);
    PRINT(num_slice_groups_minus1);
    if (num_slice_groups_minus1 > 0) {
      PRINT(slice_group_map_type);
      if (slice_group_map_type == 0) {
	PRINT_ARR(run_length_minus1, num_slice_groups_minus1+1);
      } else if (slice_group_map_type == 2) {
	PRINT_ARR(top_left, num_slice_groups_minus1);
	PRINT_ARR(bottom_right, num_slice_groups_minus1);
      } else if (slice_group_map_type == 3 || 
		 slice_group_map_type == 4 || 
		 slice_group_map_type == 5) {
	PRINT(slice_group_change_direction_flag);
	PRINT(slice_group_change_rate_minus1);
      } else if (slice_group_map_type == 6) {
	PRINT(pic_size_in_map_units_minus1);
	PRINT_ARR(slice_group_id, pic_size_in_map_units_minus1+1);
      }
    }
    PRINT(num_ref_idx_l0_default_active_minus1);
    PRINT(num_ref_idx_l1_default_active_minus1);
    PRINT(weighted_pred_flag);
    PRINT(weighted_bipred_idc);
    PRINT(pic_init_qp_minus26);
    PRINT(pic_init_qs_minus26);
    PRINT(chroma_qp_index_offset);
    PRINT(deblocking_filter_control_present_flag);
    PRINT(constrained_intra_pred_flag);
    PRINT(redundant_pic_cnt_present_flag);
  }

};
