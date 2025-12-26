// SPDX-License-Identifier: BSD-3-Clause
// Simplified TritonPart core for standalone operation

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "adapter/NetlistAdapter.h"
#include "src/Hypergraph.h"
#include "src/Utilities.h"

namespace par {

// Forward declarations
class Coarsener;
class Multilevel;
class Partitioner;
class GoldenEvaluator;

// Simplified TritonPart core class
class TritonPartCore {
public:
    TritonPartCore();
    ~TritonPartCore();
    
    // Set adapter for netlist input
    void setAdapter(std::shared_ptr<NetlistAdapter> adapter);
    
    // Read input files through adapter
    bool readNetlist(const std::string& filename, const std::string& top_module = "");
    bool readLiberty(const std::string& filename);
    bool readSDC(const std::string& filename);
    
    // Build internal data structures
    bool buildHypergraph();
    bool extractTimingPaths();
    
    // Partitioning parameters
    void setNumPartitions(int num_parts) { num_parts_ = num_parts; }
    void setBalance(float balance) { balance_ = balance; }
    void setTimingAware(bool enable) { timing_aware_ = enable; }
    void setMaxIterations(int max_iter) { max_iterations_ = max_iter; }
    
    // Run partitioning
    bool partition();
    
    // Get results
    std::vector<int> getPartitionAssignment() const;
    float getCutsize() const;
    int getNumCuts() const;
    
    // Evaluation
    void evaluatePartition();
    void reportPartitionMetrics();
    
    // Write output
    bool writePartitionResult(const std::string& filename);
    
private:
    // Adapter for external netlist
    std::shared_ptr<NetlistAdapter> adapter_;
    
    // Core data structures
    std::shared_ptr<Hypergraph> hypergraph_;
    std::vector<int> partition_;
    
    // Partitioning components
    std::shared_ptr<Coarsener> coarsener_;
    std::shared_ptr<Multilevel> multilevel_;
    std::shared_ptr<Partitioner> partitioner_;
    std::shared_ptr<GoldenEvaluator> evaluator_;
    
    // Parameters
    int num_parts_ = 2;
    float balance_ = 1.1;  // 10% imbalance allowed
    bool timing_aware_ = true;
    int max_iterations_ = 10;
    int seed_ = 0;
    
    // Timing paths
    std::vector<TimingPath> timing_paths_;
    
    // Metrics
    float cutsize_ = 0;
    int num_cuts_ = 0;
    
    // Helper functions
    void initializePartitioner();
    void performSimplePartition();
    void performMultilevelPartition();
    void convertAdapterDataToHypergraph();
    void extractTimingPathsFromAdapter();
};

} // namespace par
