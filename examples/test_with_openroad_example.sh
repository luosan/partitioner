#!/bin/bash

# Test TritonPart with OpenROAD's timing-aware partitioning example

echo "========================================="
echo "测试 OpenROAD Ariane Example"  
echo "========================================="

# 路径设置
OPENROAD_EXAMPLE="/home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning"
PARTITION_DIR="$(dirname $(dirname $(realpath $0)))"
TRITONPART="${PARTITION_DIR}/build/bin/tritonpart"

# 检查文件
if [[ ! -f "${OPENROAD_EXAMPLE}/ariane.v" ]]; then
    echo "错误：找不到 ariane.v"
    echo "请确认路径: ${OPENROAD_EXAMPLE}"
    exit 1
fi

# 编译
echo "编译 TritonPart..."
cd "${PARTITION_DIR}/build"
if [[ ! -f "$TRITONPART" ]]; then
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make tritonpart -j4
fi
cd - > /dev/null

echo ""
echo "输入文件:"
echo "  Verilog: ${OPENROAD_EXAMPLE}/ariane.v"
echo "  Liberty: ${OPENROAD_EXAMPLE}/Nangate45/*.lib"
echo "  SDC: ${OPENROAD_EXAMPLE}/ariane.sdc"
echo ""

# 收集所有Liberty文件
LIBERTY_FILES=""
for lib in ${OPENROAD_EXAMPLE}/Nangate45/*.lib; do
    if [[ -f "$lib" ]]; then
        LIBERTY_FILES="$LIBERTY_FILES -l $lib"
    fi
done

# 运行5路时序感知分区
echo "运行5路时序感知分区..."
$TRITONPART \
    -v "${OPENROAD_EXAMPLE}/ariane.v" \
    $LIBERTY_FILES \
    -s "${OPENROAD_EXAMPLE}/ariane.sdc" \
    -n 5 \
    -b 2.0 \
    -t \
    -o ariane_5way_result.txt 2>&1 | tee ariane_partition.log

if [[ $? -eq 0 ]]; then
    echo ""
    echo "✅ 分区成功!"
    echo ""
    
    # 显示结果统计
    echo "分区统计:"
    grep -E "Number of vertices|Number of hyperedges|Cutsize" ariane_partition.log
    
    echo ""
    echo "与 OpenROAD 对比:"
    echo "  OpenROAD: 135,405 vertices, 136,227 hyperedges"
    echo "  我们的结果:"
    grep "Number of" ariane_partition.log | head -2
    
    # 显示分区平衡度
    echo ""
    echo "分区平衡:"
    grep "Partition.*size:" ariane_partition.log
else
    echo ""
    echo "❌ 分区失败"
    echo "查看日志: ariane_partition.log"
    tail -30 ariane_partition.log
fi
