#!/bin/bash

# TritonPart Standalone - Run partitioning with OpenSTA

usage() {
    echo "Usage: $0 -v <verilog> -l <liberty> -s <sdc> [-n num_parts] [-o output]"
    echo "  -v: Verilog netlist file"
    echo "  -l: Liberty library file(s) (can be specified multiple times)"
    echo "  -s: SDC constraints file"  
    echo "  -n: Number of partitions (default: 2)"
    echo "  -o: Output file (default: partition_result.txt)"
    echo "  -t: Enable timing-aware partitioning"
    echo "  -b: Balance constraint (default: 2.0)"
    exit 1
}

# Default values
NUM_PARTS=2
OUTPUT="partition_result.txt"
BALANCE=2.0
TIMING_AWARE=false
VERILOG=""
SDC=""
LIBERTY_FILES=""

# Parse arguments
while getopts "v:l:s:n:o:b:th" opt; do
    case $opt in
        v) VERILOG="$OPTARG" ;;
        l) LIBERTY_FILES="$LIBERTY_FILES -l $OPTARG" ;;
        s) SDC="$OPTARG" ;;
        n) NUM_PARTS="$OPTARG" ;;
        o) OUTPUT="$OPTARG" ;;
        b) BALANCE="$OPTARG" ;;
        t) TIMING_AWARE=true ;;
        h) usage ;;
        *) usage ;;
    esac
done

# Check required arguments
if [[ -z "$VERILOG" ]]; then
    echo "Error: Verilog file is required"
    usage
fi

# Build path
PARTITION_DIR="$(dirname $(dirname $(realpath $0)))"
TRITONPART="${PARTITION_DIR}/build/bin/tritonpart"

# Check if tritonpart exists
if [[ ! -f "$TRITONPART" ]]; then
    echo "Error: tritonpart not found. Please build first:"
    echo "  cd ${PARTITION_DIR}/build"
    echo "  cmake .."
    echo "  make tritonpart"
    exit 1
fi

# Build command
CMD="$TRITONPART -v $VERILOG"

if [[ -n "$LIBERTY_FILES" ]]; then
    CMD="$CMD $LIBERTY_FILES"
fi

if [[ -n "$SDC" ]]; then
    CMD="$CMD -s $SDC"
fi

CMD="$CMD -n $NUM_PARTS -b $BALANCE -o $OUTPUT"

if [[ "$TIMING_AWARE" == true ]]; then
    CMD="$CMD -t"
fi

# Run partitioning
echo "========================================="
echo "TritonPart Standalone Partitioner"
echo "========================================="
echo "Command: $CMD"
echo "========================================="

$CMD

if [[ $? -eq 0 ]]; then
    echo ""
    echo "✅ Partitioning completed successfully!"
    echo "Output written to: $OUTPUT"
else
    echo ""
    echo "❌ Partitioning failed"
    exit 1
fi
