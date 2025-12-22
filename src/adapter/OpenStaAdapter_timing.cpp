// SPDX-License-Identifier: BSD-3-Clause
// Enhanced timing path extraction for OpenStaAdapter

#include "OpenStaAdapter.h"
#include "../src/Hypergraph.h"
#include "../utils/Logger.h"

// OpenSTA headers for timing path extraction
#include "sta/Sta.hh"
#include "sta/Network.hh"
#include "sta/Search.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Path.hh"
#include "sta/TimingArc.hh"
#include "sta/Corner.hh"
#include "sta/MinMax.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Units.hh"

#include <map>
#include <set>
#include <algorithm>

namespace par {

// Helper function to extract timing paths with real OpenSTA
// This implementation follows OpenROAD's BuildTimingPaths() method
std::vector<TimingPath> extractTimingPathsFromSTA(
    sta::Sta* sta, 
    const std::map<sta::Instance*, int>& inst_to_id,
    const std::map<sta::Net*, int>& net_to_id,
    int max_paths,
    Logger& logger) {
    
    std::vector<TimingPath> timing_paths;
    
    if (!sta) {
        logger.warning("STA not initialized for timing path extraction");
        return timing_paths;
    }
    
    try {
        // Ensure timing graph is built and updated (same as OpenROAD)
        sta->ensureGraph();
        sta->searchPreamble();
        sta->ensureLevelized();
        
        sta::Search* search = sta->search();
        if (!search) {
            logger.error("Search engine not available");
            return timing_paths;
        }
        
        // Parameters matching OpenROAD's BuildTimingPaths():
        // - group_count = top_n_ (default 100000)
        // - endpoint_count = 1 (one path per endpoint)
        // - include_unconstrained = false
        // - get_max = true (setup check only, not hold)
        // - unique_pins = true
        int group_count = max_paths;  // Number of paths in total
        int endpoint_count = 1;       // Number of paths per endpoint
        bool include_unconstrained = false;
        bool get_max = true;          // max for setup check
        
        // Find timing path endpoints using same parameters as OpenROAD
        sta::PathEndSeq path_ends = search->findPathEnds(
            nullptr,  // e_from: return paths from any source
            nullptr,  // e_thrus: no through constraints
            nullptr,  // e_to: return paths to any endpoint
            include_unconstrained,  // unconstrained paths
            sta->cmdCorner(),       // corner
            get_max ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),  // setup (max) or hold (min)
            group_count,     // group_count: total number of paths
            endpoint_count,  // endpoint_count: paths per endpoint
            true,            // unique_pins
            -sta::INF,       // slack_min
            sta::INF,        // slack_max
            true,            // sort_by_slack
            nullptr,         // group_names
            get_max,         // setup
            !get_max,        // hold
            false,           // recovery
            false,           // removal
            false,           // clk_gating_setup
            false            // clk_gating_hold
        );
        
        logger.info("Found " + std::to_string(path_ends.size()) + " path endpoints");
        
        int critical_count = 0;
        int non_critical_count = 0;
        float slack_threshold = -0.01f; // Consider paths with slack < -0.01 as critical
        
        // Process each path endpoint
        for (sta::PathEnd* path_end : path_ends) {
            if (!path_end) continue;
            
            sta::Path* path = path_end->path();
            if (!path) continue;
            
            // Get slack value
            float slack = path_end->slack(sta);
            bool is_critical = (slack < slack_threshold);
            
            if (is_critical) {
                critical_count++;
            } else {
                non_critical_count++;
            }
            
            // Expand the path to get all pins/instances along it
            sta::PathExpanded expanded(path, sta);
            
            // Build the vertex path (instances)
            std::vector<int> path_vertices;
            std::set<int> unique_vertices; // To avoid duplicates
            
            // Build the arc path (nets)  
            std::vector<int> path_arcs;
            std::set<int> unique_arcs; // To avoid duplicates
            
            // Traverse the expanded path
            for (size_t i = 0; i < expanded.size(); i++) {
                // PathExpanded provides direct access to the path at index
                const sta::Path* path_at_i = expanded.path(i);
                if (!path_at_i) continue;
                
                // Get the pin at this path point
                const sta::Pin* pin = path_at_i->pin(sta);
                if (!pin) continue;
                
                // Get the instance for this pin
                sta::Network* network = sta->network();
                sta::Instance* inst = network->instance(pin);
                
                // Map instance to vertex ID
                if (inst) {
                    auto it = inst_to_id.find(inst);
                    if (it != inst_to_id.end()) {
                        int vertex_id = it->second;
                        if (unique_vertices.find(vertex_id) == unique_vertices.end()) {
                            path_vertices.push_back(vertex_id);
                            unique_vertices.insert(vertex_id);
                        }
                    }
                }
                
                // Get the net connected to this pin
                sta::Net* net = network->net(pin);
                if (net) {
                    auto it = net_to_id.find(net);
                    if (it != net_to_id.end()) {
                        int arc_id = it->second;
                        if (unique_arcs.find(arc_id) == unique_arcs.end()) {
                            path_arcs.push_back(arc_id);
                            unique_arcs.insert(arc_id);
                        }
                    }
                }
            }
            
            // Create TimingPath object if we have valid path
            if (!path_vertices.empty()) {
                TimingPath timing_path(path_vertices, path_arcs, slack);
                timing_paths.push_back(timing_path);
            }
        }
        
        logger.info("Extracted " + std::to_string(timing_paths.size()) + " timing paths");
        logger.info("  Critical paths: " + std::to_string(critical_count));
        logger.info("  Non-critical paths: " + std::to_string(non_critical_count));
        
        // PathEnds are owned by Search PathGroups and deleted automatically
        // Do NOT delete them manually
        
    } catch (const std::exception& e) {
        logger.error("Exception during timing path extraction: " + std::string(e.what()));
    }
    
    return timing_paths;
}

// Helper to build instance to ID mapping
std::map<sta::Instance*, int> buildInstanceToIdMap(
    sta::Network* network,
    const std::vector<Instance>& instances) {
    
    std::map<sta::Instance*, int> inst_to_id;
    
    if (!network) return inst_to_id;
    
    sta::Instance* top_inst = network->topInstance();
    if (!top_inst) return inst_to_id;
    
    // Map instance names to IDs
    std::map<std::string, int> name_to_id;
    for (size_t i = 0; i < instances.size(); i++) {
        name_to_id[instances[i].name] = i;
    }
    
    // Iterate through all leaf instances and map them
    sta::LeafInstanceIterator* iter = network->leafInstanceIterator();
    while (iter->hasNext()) {
        sta::Instance* inst = iter->next();
        std::string inst_name = network->pathName(inst);
        
        auto it = name_to_id.find(inst_name);
        if (it != name_to_id.end()) {
            inst_to_id[inst] = it->second;
        }
    }
    delete iter;
    
    return inst_to_id;
}

// Helper to build net to ID mapping  
// Uses highestConnectedNet to match the net extraction logic
std::map<sta::Net*, int> buildNetToIdMap(
    sta::Network* network,
    const std::vector<Net>& nets) {
    
    std::map<sta::Net*, int> net_to_id;
    
    if (!network) return net_to_id;
    
    sta::Instance* top_inst = network->topInstance();
    if (!top_inst) return net_to_id;
    
    // Map net names to IDs
    std::map<std::string, int> name_to_id;
    for (size_t i = 0; i < nets.size(); i++) {
        name_to_id[nets[i].name] = i;
    }
    
    // Function to recursively process nets
    // We need to map both the original net and its highestConnectedNet
    std::function<void(sta::Instance*)> processNets = [&](sta::Instance* inst) {
        sta::NetIterator* iter = network->netIterator(inst);
        while (iter->hasNext()) {
            sta::Net* net = iter->next();
            
            // Get the highest connected net (same as in extractNetlistData)
            const sta::Net* highest_net = network->highestConnectedNet(net);
            std::string net_name = network->pathName(highest_net);
            
            auto it = name_to_id.find(net_name);
            if (it != name_to_id.end()) {
                // Map both the original net and the highest net to the same ID
                net_to_id[net] = it->second;
                net_to_id[const_cast<sta::Net*>(highest_net)] = it->second;
            }
        }
        delete iter;
        
        // Process child instances
        sta::InstanceChildIterator* child_iter = network->childIterator(inst);
        while (child_iter->hasNext()) {
            sta::Instance* child = child_iter->next();
            processNets(child);
        }
        delete child_iter;
    };
    
    // Start from top instance
    processNets(top_inst);
    
    return net_to_id;
}

} // namespace par
