# 启用 PAR evaluation debug 输出 Cutsize
set_debug_level PAR evaluation 1

read_lef /home/lzx/work/mypartition/tests/openroad_comparison/ariane/Nangate45/Nangate45_tech.lef
read_lef /home/lzx/work/mypartition/tests/openroad_comparison/ariane/Nangate45/Nangate45_stdcell.lef
read_lef /home/lzx/work/mypartition/tests/openroad_comparison/ariane/Nangate45/fakeram45_256x16.lef

read_liberty /home/lzx/work/mypartition/tests/openroad_comparison/ariane/Nangate45/Nangate45_typ.lib
read_liberty /home/lzx/work/mypartition/tests/openroad_comparison/ariane/Nangate45/fakeram45_256x16.lib

read_verilog /home/lzx/work/mypartition/tests/openroad_comparison/ariane/ariane.v
link_design ariane
read_sdc /home/lzx/work/mypartition/tests/openroad_comparison/ariane/ariane.sdc

triton_part_design -num_parts 5 -balance_constraint 2 \
  -seed 0 -top_n 100000 \
  -timing_aware_flag true -extra_delay 9.2e-10 \
  -guardband_flag true \
  -solution_file /home/lzx/work/mypartition/tests/openroad_comparison/ariane/results/openroad_result.part.5

exit
