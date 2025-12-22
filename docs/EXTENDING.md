# Extending TritonPart Standalone

This document describes how to extend the current implementation to support real netlists and advanced features.

## Current Implementation Status

### ✅ Completed
- Core partitioning algorithms extracted from OpenROAD
- Adapter pattern for netlist abstraction
- Simplified OpenSTA adapter with dummy data
- Command-line interface
- Basic testing framework

### 🚧 Pending Full Implementation

## 1. Full OpenSTA Integration

### Current State
The `OpenStaAdapter_simple.cpp` uses dummy data. The full `OpenStaAdapter.cpp` contains the complete implementation but needs API fixes.

### Steps to Complete

#### Step 1: Fix OpenSTA API Compatibility
```cpp
// In OpenStaAdapter.cpp, update deprecated APIs:

// Old API
sta_->linkDesign("*");

// New API  
sta_->linkDesign(nullptr);

// Old API
sta_->readSdc(filename);

// New API
sta_->sourceTcl("source " + filename);
```

#### Step 2: Initialize OpenSTA Properly
```cpp
bool OpenStaAdapter::initialize() {
    // Initialize STA
    sta::initSta();
    sta_ = std::make_unique<sta::Sta>();
    
    // Create network
    network_ = sta_->makeConcreteNetwork();
    
    // Set units and other defaults
    sta_->setTclInterp(nullptr);  // or create Tcl interp if needed
    
    return true;
}
```

#### Step 3: Build Complete CMake Integration
```cmake
# In src/CMakeLists.txt
if(BUILD_WITH_OPENSTA)
  find_package(OpenSTA REQUIRED)
  target_compile_definitions(tritonpart_adapter PRIVATE USE_OPENSTA)
  target_link_libraries(tritonpart_adapter PUBLIC OpenSTA::opensta)
endif()
```

## 2. Complete Multilevel Implementation

### Current Limitation
The multilevel partitioner constructor requires many parameters. Current implementation falls back to simple partitioning.

### Solution
```cpp
// Create factory method for easier construction
std::shared_ptr<MultilevelPartitioner> 
TritonPartCore::createMultilevelPartitioner() {
    // Create with sensible defaults
    return std::make_shared<MultilevelPartitioner>(
        num_parts_,
        false,  // v_cycle_flag
        10,     // num_initial_solutions
        1,      // num_best_initial_solutions
        50,     // refiner_iters
        10,     // greedy_refiner_iters
        10,     // ilp_refiner_iters
        seed_,
        coarsener_,
        initial_partitioner_,
        fm_refiner_,
        pm_refiner_,
        greedy_refiner_,
        ilp_refiner_,
        evaluator_,
        &Logger::getInstance()
    );
}
```

## 3. Hypergraph Format Support

### Add HGR File Reader
```cpp
class HypergraphReader {
public:
    static std::shared_ptr<Hypergraph> readHGR(const std::string& filename);
    static bool writeHGR(const Hypergraph& hg, const std::string& filename);
};
```

### Format Example
```
% Hypergraph file format
10 5  % num_vertices num_hyperedges
1.0 2 3 4  % weight vertex_list
2.0 5 6 7
1.5 1 8 9
```

## 4. Advanced Timing Features

### Timing Path Integration
```cpp
void TritonPartCore::integrateTimingPaths() {
    // Get paths from adapter
    auto paths = adapter_->getCriticalPaths(1000);
    
    // Convert to hypergraph representation
    for (const auto& path : paths) {
        // Add timing edges to hypergraph
        hypergraph_->addTimingEdge(path);
        
        // Update weights based on slack
        float weight = computeTimingWeight(path.slack);
        hypergraph_->updateEdgeWeight(path.arcs, weight);
    }
}
```

### Slack Propagation
```cpp
void propagateSlack(Hypergraph& hg, const TimingPaths& paths) {
    // Implement slack propagation algorithm
    // Update vertex/edge weights based on timing criticality
}
```

## 5. Python Bindings

### Using pybind11
```cpp
// python_bindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "TritonPartCore.h"

namespace py = pybind11;

PYBIND11_MODULE(tritonpart, m) {
    m.doc() = "TritonPart Python bindings";
    
    py::class_<TritonPartCore>(m, "TritonPartCore")
        .def(py::init<>())
        .def("setNumPartitions", &TritonPartCore::setNumPartitions)
        .def("setBalance", &TritonPartCore::setBalance)
        .def("partition", &TritonPartCore::partition)
        .def("getCutsize", &TritonPartCore::getCutsize);
}
```

### Usage Example
```python
import tritonpart

# Create partitioner
tp = tritonpart.TritonPartCore()
tp.setNumPartitions(4)
tp.setBalance(1.1)

# Read netlist (when implemented)
tp.readNetlist("design.v")

# Partition
tp.partition()

# Get results
print(f"Cutsize: {tp.getCutsize()}")
```

## 6. Parallel Processing

### OpenMP Integration
```cpp
void TritonPartCore::parallelEvaluate() {
    #pragma omp parallel for
    for (int i = 0; i < solutions.size(); ++i) {
        evaluator_->evaluate(solutions[i]);
    }
}
```

### Thread Pool for Coarsening
```cpp
class ParallelCoarsener : public Coarsener {
    std::thread_pool pool_;
    
    void coarsen() override {
        // Distribute work across threads
        auto futures = pool_.submit_batch(coarsen_tasks);
        // Collect results
    }
};
```

## 7. Quality Improvements

### Advanced Refinement
1. Implement V-cycle refinement
2. Add simulated annealing option
3. Implement genetic algorithm variant

### Constraint Handling
```cpp
class ConstraintManager {
    // Fixed vertices
    std::map<int, int> fixed_vertices_;
    
    // Group constraints
    std::vector<VertexGroup> groups_;
    
    // Timing constraints
    std::vector<TimingConstraint> timing_constraints_;
    
    bool validate(const Partition& p);
};
```

## 8. Testing Infrastructure

### Unit Tests with GoogleTest
```cpp
TEST(TritonPartCore, BasicPartitioning) {
    TritonPartCore core;
    core.setNumPartitions(2);
    
    // Create test hypergraph
    auto hg = createTestHypergraph(100, 200);
    
    EXPECT_TRUE(core.partition());
    EXPECT_GT(core.getCutsize(), 0);
}
```

### Benchmarking Suite
```cpp
class PartitionBenchmark {
    void run() {
        for (auto& testcase : benchmarks) {
            auto start = std::chrono::high_resolution_clock::now();
            runPartition(testcase);
            auto end = std::chrono::high_resolution_clock::now();
            
            reportMetrics(testcase, end - start);
        }
    }
};
```

## Development Priorities

1. **High Priority**
   - Complete OpenSTA adapter for real netlists
   - Fix multilevel partitioner initialization
   - Add HGR format support

2. **Medium Priority**
   - Python bindings
   - Timing path integration
   - Parallel processing

3. **Low Priority**
   - Advanced refinement algorithms
   - GUI visualization
   - Cloud deployment

## Contributing

To contribute to any of these extensions:

1. Fork the repository
2. Create a feature branch
3. Implement with tests
4. Submit pull request

## Resources

- [OpenSTA Documentation](https://github.com/The-OpenROAD-Project/OpenSTA)
- [hMETIS Format Specification](http://glaros.dtc.umn.edu/gkhome/metis/hmetis/overview)
- [OR-Tools Documentation](https://developers.google.com/optimization)
