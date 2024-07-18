############################################################
## This file is generated automatically by Vitis HLS.
## Please DO NOT edit it.
## Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
## Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
############################################################
open_project hls_moller_compton
set_top hls_moller_compton
add_files hls_moller_compton/hls_compton.cpp
add_files hls_moller_compton/hls_compton_scalers.cpp
add_files hls_moller_compton/hls_helicity.cpp
add_files hls_moller_compton/hls_moller_compton.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7vx550t-ffg1927-1}
create_clock -period 32 -name default
config_dataflow -default_channel fifo -fifo_depth 5
config_rtl -reset state
source "./hls_moller_compton/solution1/directives.tcl"
#csim_design
csynth_design
#cosim_design
export_design -format ip_catalog
