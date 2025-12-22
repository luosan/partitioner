#!/bin/bash
#
# OpenROAD vs TritonPart 对比测试 - Ariane
# 支持: partition (分割) 和 evaluate (评估+生成超图)
# 支持: 实时输出和TCL交互模式
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

OPENROAD="/home/lzx/work/OpenROAD-flow-scripts/tools/install/OpenROAD/bin/openroad"
TRITONPART="${PROJECT_ROOT}/build/bin/tritonpart"

TEST_DIR="${SCRIPT_DIR}"

# 参数
NUM_PARTS=5
BALANCE=2
TOP_N=100000
EXTRA_DELAY="9.2e-10"

echo "=============================================="
echo "OpenROAD vs TritonPart 对比测试 - Ariane"
echo "=============================================="
echo "测试目录: ${TEST_DIR}"
echo "项目根目录: ${PROJECT_ROOT}"

# 检查 tritonpart
if [[ ! -f "$TRITONPART" ]]; then
    echo "编译 tritonpart..."
    mkdir -p "${PROJECT_ROOT}/build"
    cmake -S "${PROJECT_ROOT}" -B "${PROJECT_ROOT}/build" -DCMAKE_BUILD_TYPE=Release
    make -C "${PROJECT_ROOT}/build" tritonpart -j$(nproc)
fi

# 创建结果目录
mkdir -p "${SCRIPT_DIR}/results"

echo ""
echo "=== 1. 运行 OpenROAD triton_part_design ==="
cat > "${SCRIPT_DIR}/results/run_openroad.tcl" << EOF
# 启用 PAR evaluation debug 输出 Cutsize
set_debug_level PAR evaluation 1

read_lef ${TEST_DIR}/Nangate45/Nangate45_tech.lef
read_lef ${TEST_DIR}/Nangate45/Nangate45_stdcell.lef
read_lef ${TEST_DIR}/Nangate45/fakeram45_256x16.lef

read_liberty ${TEST_DIR}/Nangate45/Nangate45_typ.lib
read_liberty ${TEST_DIR}/Nangate45/fakeram45_256x16.lib

read_verilog ${TEST_DIR}/ariane.v
link_design ariane
read_sdc ${TEST_DIR}/ariane.sdc

triton_part_design -num_parts ${NUM_PARTS} -balance_constraint ${BALANCE} \\
  -seed 0 -top_n ${TOP_N} \\
  -timing_aware_flag true -extra_delay ${EXTRA_DELAY} \\
  -guardband_flag true \\
  -solution_file ${SCRIPT_DIR}/results/openroad_result.part.${NUM_PARTS}

exit
EOF

# 实时输出 + 保存日志
"$OPENROAD" "${SCRIPT_DIR}/results/run_openroad.tcl" 2>&1 | tee "${SCRIPT_DIR}/results/openroad_partition.log"

echo ""
echo "OpenROAD triton_part_design 结果:"
grep -E "vertices|hyperedges|timing paths|Cutsize" "${SCRIPT_DIR}/results/openroad_partition.log" | head -10

echo ""
echo "=== 2. 运行 OpenROAD evaluate_part_design_solution ==="

# 转换分割结果格式 (OpenROAD输出格式 -> hMETIS格式)
# OpenROAD格式: instance_name partition_id
# hMETIS格式: partition_id (每行一个)
if [[ -f "${SCRIPT_DIR}/results/openroad_result.part.${NUM_PARTS}" ]]; then
    awk '{print $2}' "${SCRIPT_DIR}/results/openroad_result.part.${NUM_PARTS}" > "${SCRIPT_DIR}/results/openroad_result.part.${NUM_PARTS}.update"
fi

cat > "${SCRIPT_DIR}/results/run_openroad_evaluate.tcl" << EOF
# 启用 PAR evaluation debug 输出 Cutsize
set_debug_level PAR evaluation 1

read_lef ${TEST_DIR}/Nangate45/Nangate45_tech.lef
read_lef ${TEST_DIR}/Nangate45/Nangate45_stdcell.lef
read_lef ${TEST_DIR}/Nangate45/fakeram45_256x16.lef

read_liberty ${TEST_DIR}/Nangate45/Nangate45_typ.lib
read_liberty ${TEST_DIR}/Nangate45/fakeram45_256x16.lib

read_verilog ${TEST_DIR}/ariane.v
link_design ariane
read_sdc ${TEST_DIR}/ariane.sdc

evaluate_part_design_solution -num_parts ${NUM_PARTS} -balance_constraint ${BALANCE} \\
  -timing_aware_flag true -extra_delay ${EXTRA_DELAY} \\
  -top_n ${TOP_N} -guardband_flag false \\
  -solution_file ${SCRIPT_DIR}/results/openroad_result.part.${NUM_PARTS}.update \\
  -hypergraph_file ${SCRIPT_DIR}/results/openroad_ariane.hgr.wt \\
  -hypergraph_int_weight_file ${SCRIPT_DIR}/results/openroad_ariane.hgr.int

exit
EOF

# 实时输出 + 保存日志
"$OPENROAD" "${SCRIPT_DIR}/results/run_openroad_evaluate.tcl" 2>&1 | tee "${SCRIPT_DIR}/results/openroad_evaluate.log"

echo ""
echo "OpenROAD evaluate_part_design_solution 结果:"
grep -E "Cutsize|balance|hypergraph|timing" "${SCRIPT_DIR}/results/openroad_evaluate.log" | head -10

echo ""
echo "=== 3. 运行 TritonPart partition ==="
# 实时输出 + 保存日志
"$TRITONPART" partition \
    -v "${TEST_DIR}/ariane.v" \
    -l "${TEST_DIR}/Nangate45/Nangate45_typ.lib" \
    -l "${TEST_DIR}/Nangate45/fakeram45_256x16.lib" \
    -s "${TEST_DIR}/ariane.sdc" \
    -m ariane \
    -n ${NUM_PARTS} \
    -b ${BALANCE} \
    -t \
    --top_n ${TOP_N} \
    --extra_delay ${EXTRA_DELAY} \
    -o "${SCRIPT_DIR}/results/tritonpart_result.part.${NUM_PARTS}" \
    2>&1 | tee "${SCRIPT_DIR}/results/tritonpart_partition.log"

echo ""
echo "=== 4. 运行 TritonPart evaluate ==="
# 实时输出 + 保存日志
"$TRITONPART" evaluate \
    -v "${TEST_DIR}/ariane.v" \
    -l "${TEST_DIR}/Nangate45/Nangate45_typ.lib" \
    -l "${TEST_DIR}/Nangate45/fakeram45_256x16.lib" \
    -s "${TEST_DIR}/ariane.sdc" \
    -m ariane \
    -n ${NUM_PARTS} \
    -b ${BALANCE} \
    -t \
    --top_n ${TOP_N} \
    --extra_delay ${EXTRA_DELAY} \
    --solution "${SCRIPT_DIR}/results/tritonpart_result.part.${NUM_PARTS}" \
    --hypergraph "${SCRIPT_DIR}/results/tritonpart_ariane.hgr.wt" \
    --hypergraph_int "${SCRIPT_DIR}/results/tritonpart_ariane.hgr.int" \
    2>&1 | tee "${SCRIPT_DIR}/results/tritonpart_evaluate.log"

echo ""
echo "=== 5. 对比 ==="
OR_V=$(grep -oP '\d+(?= vertices)' "${SCRIPT_DIR}/results/openroad_partition.log" 2>/dev/null | head -1)
OR_E=$(grep -oP '\d+(?= hyperedges)' "${SCRIPT_DIR}/results/openroad_partition.log" 2>/dev/null | head -1)
OR_T=$(grep -oP '\d+(?= timing paths)' "${SCRIPT_DIR}/results/openroad_partition.log" 2>/dev/null | head -1)

TP_V=$(grep -oP '\d+(?= vertices)' "${SCRIPT_DIR}/results/tritonpart_partition.log" 2>/dev/null | head -1)
TP_E=$(grep -oP '\d+(?= hyperedges)' "${SCRIPT_DIR}/results/tritonpart_partition.log" 2>/dev/null | head -1)
TP_T=$(grep -oP 'Extracted \K\d+(?= timing paths)' "${SCRIPT_DIR}/results/tritonpart_partition.log" 2>/dev/null | head -1)

printf "%-20s %12s %12s %10s\n" "指标" "OpenROAD" "TritonPart" "差异"
printf "%-20s %12s %12s %10s\n" "--------------------" "------------" "------------" "----------"
printf "%-20s %12s %12s %10s\n" "Vertices" "${OR_V:-N/A}" "${TP_V:-N/A}" "$((${TP_V:-0} - ${OR_V:-0}))"
printf "%-20s %12s %12s %10s\n" "Hyperedges" "${OR_E:-N/A}" "${TP_E:-N/A}" "$((${TP_E:-0} - ${OR_E:-0}))"
printf "%-20s %12s %12s %10s\n" "Timing Paths" "${OR_T:-N/A}" "${TP_T:-N/A}" "$((${TP_T:-0} - ${OR_T:-0}))"

echo ""
echo "=== 6. 超图文件对比 ==="
if [[ -f "${SCRIPT_DIR}/results/openroad_ariane.hgr.wt" ]]; then
    OR_HGR_LINES=$(wc -l < "${SCRIPT_DIR}/results/openroad_ariane.hgr.wt")
    echo "OpenROAD 超图文件: ${OR_HGR_LINES} 行"
else
    echo "OpenROAD 超图文件: 未生成"
fi

if [[ -f "${SCRIPT_DIR}/results/tritonpart_ariane.hgr.wt" ]]; then
    TP_HGR_LINES=$(wc -l < "${SCRIPT_DIR}/results/tritonpart_ariane.hgr.wt")
    echo "TritonPart 超图文件: ${TP_HGR_LINES} 行"
else
    echo "TritonPart 超图文件: 未生成"
fi

echo ""
echo "=== 7. 分析 ==="
if [[ "${TP_V:-0}" == "${OR_V:-0}" ]]; then
    echo "✅ Vertices 匹配 (${TP_V})"
else
    echo "⚠️  Vertices 差异: TritonPart=${TP_V}, OpenROAD=${OR_V}"
fi

if [[ "${TP_E:-0}" == "${OR_E:-0}" ]]; then
    echo "✅ Hyperedges 匹配 (${TP_E})"
else
    echo "⚠️  Hyperedges 差异: TritonPart=${TP_E}, OpenROAD=${OR_E}"
fi

if [[ "${TP_T:-0}" == "${OR_T:-0}" ]]; then
    echo "✅ Timing Paths 匹配 (${TP_T})"
else
    echo "⚠️  Timing Paths 差异: TritonPart=${TP_T}, OpenROAD=${OR_T}"
fi

echo ""
echo "日志文件:"
echo "  ${SCRIPT_DIR}/results/openroad_partition.log"
echo "  ${SCRIPT_DIR}/results/openroad_evaluate.log"
echo "  ${SCRIPT_DIR}/results/tritonpart_partition.log"
echo "  ${SCRIPT_DIR}/results/tritonpart_evaluate.log"
echo ""
echo "超图文件:"
echo "  ${SCRIPT_DIR}/results/openroad_ariane.hgr.wt"
echo "  ${SCRIPT_DIR}/results/openroad_ariane.hgr.int"
echo "  ${SCRIPT_DIR}/results/tritonpart_ariane.hgr.wt"
echo "  ${SCRIPT_DIR}/results/tritonpart_ariane.hgr.int"
