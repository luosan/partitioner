// SPDX-License-Identifier: BSD-3-Clause
// Adapter interface for converting external netlist formats to TritonPart hypergraph

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace par {

// Forward declarations
class Hypergraph;
struct TimingPath;  // Defined in Hypergraph.h

// Basic data structures for netlist representation
struct Instance {
    std::string name;
    std::string cell_type;
    int id;
    bool is_sequential;
    bool is_macro;
    float area;
};

struct Net {
    std::string name;
    int id;
    std::vector<int> instances;  // Instance IDs connected to this net
    std::vector<std::string> pins;  // Pin names
    float weight;
};

struct Pin {
    std::string name;
    int instance_id;
    int net_id;
    bool is_input;
    bool is_output;
};

// Use TimingPath from Hypergraph.h which is already defined there
// struct TimingPath is defined in src/Hypergraph.h

// Abstract base class for netlist adapters
class NetlistAdapter {
public:
    virtual ~NetlistAdapter() = default;
    
    // Read netlist from file
    virtual bool readNetlist(const std::string& filename, const std::string& top_module = "") = 0;
    
    // Read gate-level netlist (for netlists with standard cells)
    virtual bool readNetlistGateLevel(const std::string& filename) {
        // Default implementation just calls readNetlist
        return readNetlist(filename);
    }
    
    // Read timing constraints (SDC)
    virtual bool readSDC(const std::string& filename) = 0;
    
    // Read library (Liberty)
    virtual bool readLiberty(const std::string& filename) = 0;
    
    // Perform timing analysis
    virtual bool runTimingAnalysis() = 0;
    
    // Get netlist components
    virtual std::vector<Instance> getInstances() const = 0;
    virtual std::vector<Net> getNets() const = 0;
    virtual std::vector<Pin> getPins() const = 0;
    
    // Get timing paths
    virtual std::vector<TimingPath> getCriticalPaths(int max_paths = 100) const = 0;
    virtual float getNetSlack(int net_id) const = 0;
    
    // Convert to TritonPart hypergraph
    virtual std::shared_ptr<Hypergraph> buildHypergraph() = 0;
    
    // Get statistics
    virtual int getNumInstances() const = 0;
    virtual int getNumNets() const = 0;
    virtual int getNumPins() const = 0;
};

} // namespace par
