// SPDX-License-Identifier: BSD-3-Clause
// TritonPart Core implementation - integrates adapters with partitioning algorithms

#include "TritonPartCore.h"
#include "adapter/NetlistAdapter.h"
#include "src/Hypergraph.h"
#include "src/Multilevel.h"
#include "src/Partitioner.h"
#include "src/Evaluator.h"
#include "src/Coarsener.h"
#include "src/KWayFMRefine.h"
#include "utils/Logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>

namespace par {

TritonPartCore::TritonPartCore() 
    : num_parts_(2),
      balance_(1.1),
      timing_aware_(false),
      max_iterations_(10),
      seed_(0),
      cutsize_(0),
      num_cuts_(0) {
}

TritonPartCore::~TritonPartCore() {
}

void TritonPartCore::setAdapter(std::shared_ptr<NetlistAdapter> adapter) {
    adapter_ = adapter;
}

bool TritonPartCore::readNetlist(const std::string& filename, const std::string& top_module) {
    if (!adapter_) {
        Logger::getInstance().error("No adapter set");
        return false;
    }
    return adapter_->readNetlist(filename, top_module);
}

bool TritonPartCore::readLiberty(const std::string& filename) {
    if (!adapter_) {
        Logger::getInstance().error("No adapter set");
        return false;
    }
    return adapter_->readLiberty(filename);
}

bool TritonPartCore::readSDC(const std::string& filename) {
    if (!adapter_) {
        Logger::getInstance().error("No adapter set");
        return false;
    }
    return adapter_->readSDC(filename);
}

bool TritonPartCore::buildHypergraph() {
    auto& logger = Logger::getInstance();
    
    if (!adapter_) {
        logger.error("No adapter set");
        return false;
    }
    
    logger.info("Building hypergraph from adapter...");
    
    // The input files should be read before calling buildHypergraph
    // This method just builds the hypergraph from already loaded data
    hypergraph_ = adapter_->buildHypergraph();
    
    if (!hypergraph_) {
        logger.error("Failed to build hypergraph");
        return false;
    }
    
    logger.info("Hypergraph built: " + 
                std::to_string(hypergraph_->GetNumVertices()) + " vertices, " +
                std::to_string(hypergraph_->GetNumHyperedges()) + " hyperedges");
    
    return true;
}

bool TritonPartCore::extractTimingPaths() {
    auto& logger = Logger::getInstance();
    
    if (!adapter_) {
        logger.error("No adapter set");
        return false;
    }
    
    if (!timing_aware_) {
        logger.info("Timing-aware partitioning disabled, skipping path extraction");
        return true;
    }
    
    logger.info("Extracting timing paths...");
    
    // Get critical paths from adapter
    // Use top_n = 100000 to match OpenROAD's default setting
    timing_paths_ = adapter_->getCriticalPaths(100000);  // Get top 100000 paths
    
    logger.info("Extracted " + std::to_string(timing_paths_.size()) + " timing paths");
    
    if (!timing_paths_.empty()) {
        logger.info("Worst slack: " + std::to_string(timing_paths_[0].slack));
    }
    
    // Add timing paths to hypergraph if available
    if (hypergraph_ && !timing_paths_.empty()) {
        // TODO: Add timing paths to hypergraph when method is available
        // hypergraph_->SetTimingPaths(timing_paths_);
        logger.info("Timing paths extracted (integration pending)");
    }
    
    return true;
}

bool TritonPartCore::partition() {
    auto& logger = Logger::getInstance();
    
    if (!hypergraph_) {
        logger.error("No hypergraph available for partitioning");
        return false;
    }
    
    logger.info("Starting " + std::to_string(num_parts_) + "-way partitioning...");
    
    // Initialize partitioner components
    initializePartitioner();
    
    // Reset partition vector
    partition_.clear();
    partition_.resize(hypergraph_->GetNumVertices(), 0);
    
    // Determine partition method based on size
    int num_vertices = hypergraph_->GetNumVertices();
    
    if (num_vertices < 100) {
        // For small hypergraphs, use simple random or greedy partitioning
        logger.info("Using simple partitioning for small hypergraph");
        performSimplePartition();
    } else {
        // For larger hypergraphs, use multilevel partitioning
        logger.info("Using multilevel partitioning");
        performMultilevelPartition();
    }
    
    // Evaluate partition quality
    evaluatePartition();
    
    logger.info("Partitioning completed");
    logger.info("  Cutsize: " + std::to_string(cutsize_));
    logger.info("  Number of cut hyperedges: " + std::to_string(num_cuts_));
    
    return true;
}

std::vector<int> TritonPartCore::getPartitionAssignment() const {
    return partition_;
}

float TritonPartCore::getCutsize() const {
    return cutsize_;
}

int TritonPartCore::getNumCuts() const {
    return num_cuts_;
}

void TritonPartCore::evaluatePartition() {
    auto& logger = Logger::getInstance();
    
    if (!hypergraph_ || partition_.empty()) {
        logger.warning("Cannot evaluate partition - no hypergraph or partition available");
        return;
    }
    
    // Count cut hyperedges
    num_cuts_ = 0;
    cutsize_ = 0.0;
    
    for (int e = 0; e < hypergraph_->GetNumHyperedges(); ++e) {
        std::set<int> parts_in_edge;
        
        // Get vertices in this hyperedge
        for (auto v : hypergraph_->Vertices(e)) {
            if (v < static_cast<int>(partition_.size())) {
                parts_in_edge.insert(partition_[v]);
            }
        }
        
        // If hyperedge spans multiple partitions, it's cut
        if (parts_in_edge.size() > 1) {
            num_cuts_++;
            cutsize_ += hypergraph_->GetHyperedgeWeights(e)[0];  // Use first weight dimension
        }
    }
    
    // Calculate balance
    std::vector<float> part_weights(num_parts_, 0.0);
    float total_weight = 0.0;
    
    for (int v = 0; v < hypergraph_->GetNumVertices(); ++v) {
        float weight = hypergraph_->GetVertexWeights(v)[0];  // Use first weight dimension
        part_weights[partition_[v]] += weight;
        total_weight += weight;
    }
    
    float avg_weight = total_weight / num_parts_;
    float max_imbalance = 0.0;
    
    for (int p = 0; p < num_parts_; ++p) {
        float imbalance = std::abs(part_weights[p] - avg_weight) / avg_weight;
        max_imbalance = std::max(max_imbalance, imbalance);
    }
    
    logger.info("Partition balance: max imbalance = " + std::to_string(max_imbalance * 100) + "%");
}

void TritonPartCore::reportPartitionMetrics() {
    auto& logger = Logger::getInstance();
    
    logger.report("========================================");
    logger.report("Partition Metrics:");
    logger.report("  Number of partitions: " + std::to_string(num_parts_));
    logger.report("  Number of vertices: " + std::to_string(hypergraph_->GetNumVertices()));
    logger.report("  Number of hyperedges: " + std::to_string(hypergraph_->GetNumHyperedges()));
    logger.report("  Cutsize: " + std::to_string(cutsize_));
    logger.report("  Cut hyperedges: " + std::to_string(num_cuts_));
    
    // Report partition sizes
    std::vector<int> part_sizes(num_parts_, 0);
    for (int p : partition_) {
        if (p >= 0 && p < num_parts_) {
            part_sizes[p]++;
        }
    }
    
    for (int p = 0; p < num_parts_; ++p) {
        logger.report("  Partition " + std::to_string(p) + " size: " + std::to_string(part_sizes[p]));
    }
    
    if (timing_aware_ && !timing_paths_.empty()) {
        // Report timing-related metrics
        int critical_cuts = 0;
        for (const auto& path : timing_paths_) {
            std::set<int> parts_in_path;
            for (int v : path.path) {
                if (v < static_cast<int>(partition_.size())) {
                    parts_in_path.insert(partition_[v]);
                }
            }
            if (parts_in_path.size() > 1) {
                critical_cuts++;
            }
        }
        logger.report("  Critical paths cut: " + std::to_string(critical_cuts) + 
                     " / " + std::to_string(timing_paths_.size()));
    }
    
    logger.report("========================================");
}

bool TritonPartCore::writePartitionResult(const std::string& filename) {
    auto& logger = Logger::getInstance();
    
    logger.info("Writing partition result to " + filename);
    
    std::ofstream out(filename);
    if (!out.is_open()) {
        logger.error("Failed to open output file: " + filename);
        return false;
    }
    
    // Write header
    out << "# TritonPart Partition Result" << std::endl;
    out << "# Partitions: " << num_parts_ << std::endl;
    out << "# Vertices: " << hypergraph_->GetNumVertices() << std::endl;
    out << "# Hyperedges: " << hypergraph_->GetNumHyperedges() << std::endl;
    out << "# Cutsize: " << cutsize_ << std::endl;
    out << "# Format: vertex_id partition_id" << std::endl;
    out << std::endl;
    
    // Write partition assignment
    for (size_t v = 0; v < partition_.size(); ++v) {
        out << v << " " << partition_[v] << std::endl;
    }
    
    // If we have instance names from adapter, write a second section
    if (adapter_) {
        auto instances = adapter_->getInstances();
        if (!instances.empty() && instances.size() == partition_.size()) {
            out << std::endl;
            out << "# Instance names mapping" << std::endl;
            out << "# Format: instance_name partition_id" << std::endl;
            for (size_t i = 0; i < instances.size() && i < partition_.size(); ++i) {
                out << instances[i].name << " " << partition_[i] << std::endl;
            }
        }
    }
    
    out.close();
    logger.info("Partition result written successfully");
    
    return true;
}

void TritonPartCore::initializePartitioner() {
    auto& logger = Logger::getInstance();
    
    // Create evaluator with default parameters
    std::vector<float> e_wt_factors = {1.0};      // hyperedge weight factors
    std::vector<float> v_wt_factors = {1.0};      // vertex weight factors
    std::vector<float> placement_wt_factors = {}; // no placement info
    float net_timing_factor = timing_aware_ ? 1.0 : 0.0;
    float path_timing_factor = timing_aware_ ? 1.0 : 0.0;
    float path_snaking_factor = timing_aware_ ? 0.1 : 0.0;
    float timing_exp_factor = 1.0;
    float extra_cut_delay = 0.0;
    
    evaluator_ = std::make_shared<GoldenEvaluator>(
        num_parts_,
        e_wt_factors,
        v_wt_factors,
        placement_wt_factors,
        net_timing_factor,
        path_timing_factor,
        path_snaking_factor,
        timing_exp_factor,
        extra_cut_delay,
        nullptr,  // timing_graph (not used for now)
        &logger
    );
    
    // TODO: Initialize full partitioner components when constructors are properly configured
    // For now, we use simple partitioning methods directly
}

void TritonPartCore::performSimplePartition() {
    auto& logger = Logger::getInstance();
    
    // Simple random or balanced partitioning
    std::mt19937 gen(seed_);
    std::uniform_int_distribution<> dis(0, num_parts_ - 1);
    
    // Try to balance partition sizes
    std::vector<float> part_weights(num_parts_, 0.0);
    float total_weight = 0.0;
    
    // Calculate total weight
    for (int v = 0; v < hypergraph_->GetNumVertices(); ++v) {
        total_weight += hypergraph_->GetVertexWeights(v)[0];
    }
    
    float target_weight = total_weight / num_parts_;
    
    // Assign vertices to partitions trying to balance
    for (int v = 0; v < hypergraph_->GetNumVertices(); ++v) {
        float vertex_weight = hypergraph_->GetVertexWeights(v)[0];
        
        // Find partition with minimum weight that can accept this vertex
        int best_part = 0;
        float min_weight = part_weights[0];
        
        for (int p = 1; p < num_parts_; ++p) {
            if (part_weights[p] < min_weight) {
                min_weight = part_weights[p];
                best_part = p;
            }
        }
        
        // Check if assignment maintains balance
        if (part_weights[best_part] + vertex_weight <= target_weight * balance_) {
            partition_[v] = best_part;
            part_weights[best_part] += vertex_weight;
        } else {
            // Random assignment if balance would be violated
            partition_[v] = dis(gen);
            part_weights[partition_[v]] += vertex_weight;
        }
    }
    
    logger.info("Simple balanced partitioning completed");
}

void TritonPartCore::performMultilevelPartition() {
    auto& logger = Logger::getInstance();
    
    // For now, use simple partitioning for "multilevel"
    // TODO: Implement full multilevel with proper constructor parameters
    logger.info("Using simplified partitioning (full multilevel pending)");
    performSimplePartition();
}

void TritonPartCore::convertAdapterDataToHypergraph() {
    // This is handled by adapter->buildHypergraph()
    // Additional conversion logic can be added here if needed
}

void TritonPartCore::extractTimingPathsFromAdapter() {
    // This is handled by extractTimingPaths()
    // Additional timing extraction logic can be added here if needed
}

} // namespace par
