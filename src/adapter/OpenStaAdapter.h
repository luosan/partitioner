// SPDX-License-Identifier: BSD-3-Clause
// OpenSTA-based adapter for TritonPart

#pragma once

#include "NetlistAdapter.h"
#include <memory>

// Forward declarations
namespace sta {
class Sta;
class Network;
class Graph;
class Instance;
class Net;
class Pin;
class PathEnd;
}

namespace par {

// Forward declaration for implementation
class OpenStaImpl;

class OpenStaAdapter : public NetlistAdapter {
public:
    OpenStaAdapter();
    ~OpenStaAdapter();
    
    // NetlistAdapter interface
    bool readNetlist(const std::string& filename, const std::string& top_module = "") override;
    bool readNetlistGateLevel(const std::string& filename) override { 
        return readNetlist(filename); // OpenSTA handles both RTL and gate-level
    }
    bool readSDC(const std::string& filename) override;
    bool readLiberty(const std::string& filename) override;
    bool runTimingAnalysis() override;
    
    std::vector<Instance> getInstances() const override;
    std::vector<Net> getNets() const override;
    std::vector<Pin> getPins() const override;
    
    std::vector<TimingPath> getCriticalPaths(int max_paths = 100) const override;
    float getNetSlack(int net_id) const override;
    
    std::shared_ptr<Hypergraph> buildHypergraph() override;
    
    int getNumInstances() const override;
    int getNumNets() const override;
    int getNumPins() const override;
    
private:
    // OpenSTA implementation
    std::unique_ptr<OpenStaImpl> sta_impl_;
    
    // Helper functions  
    void clearCache();
    void buildDataCache() const;
    void extractNetlistData();
    
    // Cache for converted data
    mutable std::vector<Instance> instances_;
    mutable std::vector<Net> nets_;
    mutable std::vector<Pin> pins_;
    mutable bool data_cached_;
};

} // namespace par
