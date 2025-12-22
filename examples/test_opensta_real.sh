#!/bin/bash

# Test the real OpenSTA integration with Ariane example
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/../build"
BIN_DIR="${BUILD_DIR}/bin"
OPENROAD_EXAMPLE="/home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning"

# Check if tritonpart exists
if [ ! -f "${BIN_DIR}/tritonpart" ]; then
    echo "Building tritonpart..."
    cd "${BUILD_DIR}"
    make tritonpart
fi

echo "================================================"
echo "Testing OpenSTA Integration with Ariane Example"
echo "================================================"

# Test 1: Read Verilog only
echo ""
echo "Test 1: Reading Verilog file only (with top module)..."
"${BIN_DIR}/tritonpart" \
    -v "${OPENROAD_EXAMPLE}/ariane.v" \
    -m ariane \
    -n 2 \
    -o test_ariane_verilog_only.txt 2>&1 | tee test1_output.log

echo ""
echo "Checking instance count..."
grep -i "instance\|vertex\|vertices" test1_output.log || true

# Test 2: Read Verilog + Liberty  
echo ""
echo "Test 2: Reading Verilog + Liberty files..."
"${BIN_DIR}/tritonpart" \
    -v "${OPENROAD_EXAMPLE}/ariane.v" \
    -m ariane \
    -l "${OPENROAD_EXAMPLE}/Nangate45/Nangate45_typ.lib" \
    -n 2 \
    -o test_ariane_with_lib.txt 2>&1 | tee test2_output.log

echo ""
echo "Checking instance and net counts..."
grep -i "instance\|net\|vertex\|hyperedge" test2_output.log || true

# Test 3: Full test with all files (if SDC is supported)
echo ""
echo "Test 3: Full test with Verilog + Liberty + SDC..."
"${BIN_DIR}/tritonpart" \
    -v "${OPENROAD_EXAMPLE}/ariane.v" \
    -m ariane \
    -l "${OPENROAD_EXAMPLE}/Nangate45/Nangate45_typ.lib" \
    -s "${OPENROAD_EXAMPLE}/ariane.sdc" \
    -n 4 \
    -t \
    -o test_ariane_full.txt 2>&1 | tee test3_output.log

echo ""
echo "Final statistics:"
echo "================="
echo "Expected from OpenROAD: 135,405 vertices, 136,227 hyperedges"
echo ""
echo "Our results:"
grep -E "vertices|hyperedge|instance|net" test3_output.log | tail -5 || true

echo ""
echo "================================================"
echo "Test completed. Check the output files for details."
echo "================================================"
