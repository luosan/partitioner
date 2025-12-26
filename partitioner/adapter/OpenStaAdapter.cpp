// SPDX-License-Identifier: BSD-3-Clause
// Fixed version of OpenStaAdapter with proper hierarchical extraction

#include "OpenStaAdapter.h"
#include "../src/Hypergraph.h"
#include "../utils/Logger.h"

// OpenSTA headers
#include "sta/Sta.hh"
#include "sta/Network.hh"
#include "sta/Liberty.hh"
#include "sta/Corner.hh"
#include "sta/MinMax.hh"
#include "sta/PortDirection.hh"
#include "sta/StaMain.hh"

#include <tcl.h>
#include <memory>
#include <map>
#include <set>
#include <functional>

// External TCL init array from OpenSTA
namespace sta {
extern const char *tcl_inits[];
}

namespace par {

// Private implementation class
class OpenStaImpl {
public:
    sta::Sta* sta_;
    sta::Network* network_;
    sta::Corner* corner_;
    Tcl_Interp* tcl_interp_;
    bool initialized_;
    
    OpenStaImpl() : initialized_(false) {
        auto& logger = Logger::getInstance();
        
        try {
            // Step 1: Create and initialize TCL interpreter
            logger.info("Initializing TCL interpreter...");
            tcl_interp_ = Tcl_CreateInterp();
            if (!tcl_interp_ || Tcl_Init(tcl_interp_) != TCL_OK) {
                throw std::runtime_error("Failed to initialize TCL interpreter");
            }
            
            // Step 2: Initialize OpenSTA core
            logger.info("Initializing OpenSTA core...");
            sta::initSta();
            sta::PortDirection::init();
            
            // Step 3: Create Sta instance and register as singleton
            sta_ = new sta::Sta;
            sta::Sta::setSta(sta_);
            
            // Step 4: Build components
            sta_->makeComponents();
            
            // Step 5: Bind TCL interpreter to STA
            sta_->setTclInterp(tcl_interp_);
            
            // Step 6: Load OpenSTA TCL scripts
            logger.info("Loading OpenSTA TCL scripts...");
            sta::evalTclInit(tcl_interp_, sta::tcl_inits);
            if (Tcl_Eval(tcl_interp_, "init_sta_cmds") != TCL_OK) {
                logger.warning("Failed to init_sta_cmds (may be missing in build)");
            }
            
            // Get network and corner interfaces
            network_ = sta_->network();
            corner_ = sta_->cmdCorner();
            
            initialized_ = true;
            logger.info("OpenSTA initialized successfully with full TCL support");
            
        } catch (const std::exception& e) {
            logger.error("Failed to initialize OpenSTA: " + std::string(e.what()));
            throw;
        }
    }
    
    ~OpenStaImpl() {
        delete sta_;
        if (tcl_interp_) {
            Tcl_DeleteInterp(tcl_interp_);
        }
    }
    
    // Helper to execute TCL commands
    bool executeTcl(const std::string& cmd, std::string* result = nullptr) {
        int status = Tcl_Eval(tcl_interp_, cmd.c_str());
        if (result) {
            *result = Tcl_GetStringResult(tcl_interp_);
        }
        return status == TCL_OK;
    }
};

OpenStaAdapter::OpenStaAdapter() 
    : data_cached_(false) {
    try {
        sta_impl_ = std::make_unique<OpenStaImpl>();
        Logger::getInstance().info("OpenSTA initialized successfully");
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to initialize OpenSTA: " + std::string(e.what()));
        sta_impl_ = nullptr;
    }
}

OpenStaAdapter::~OpenStaAdapter() = default;

bool OpenStaAdapter::readLiberty(const std::string& filename) {
    auto& logger = Logger::getInstance();
    
    if (!sta_impl_) {
        logger.error("OpenSTA not initialized");
        return false;
    }
    
    logger.info("Reading Liberty file: " + filename);
    
    try {
        sta::Sta* sta = sta_impl_->sta_;
        sta::LibertyLibrary* lib = sta->readLiberty(
            filename.c_str(),
            sta_impl_->corner_,
            sta::MinMaxAll::all(),
            true
        );
        
        if (lib) {
            logger.info("Liberty library loaded: " + std::string(lib->name()));
            clearCache();
            return true;
        } else {
            logger.error("Failed to read Liberty file");
            return false;
        }
    } catch (const std::exception& e) {
        logger.error("Exception reading Liberty: " + std::string(e.what()));
        return false;
    }
}

bool OpenStaAdapter::readNetlist(const std::string& filename, const std::string& top_module) {
    auto& logger = Logger::getInstance();
    
    if (!sta_impl_ || !sta_impl_->initialized_) {
        logger.error("OpenSTA not initialized");
        return false;
    }
    
    logger.info("Reading Verilog file: " + filename);
    
    // Determine top module name
    std::string module_name = top_module;
    if (module_name.empty()) {
        // Extract from filename as fallback
        size_t last_slash = filename.find_last_of("/\\");
        size_t last_dot = filename.find_last_of(".");
        if (last_dot != std::string::npos) {
            size_t start = (last_slash != std::string::npos) ? last_slash + 1 : 0;
            module_name = filename.substr(start, last_dot - start);
        }
    }
    
    logger.info("Top module: " + module_name);
    
    try {
        // Read Verilog file using C++ API
        bool read_success = sta_impl_->sta_->readVerilog(filename.c_str());
        if (!read_success) {
            logger.error("Failed to read Verilog file");
            return false;
        }
        
        logger.info("Verilog file read successfully");
        
        // Now with proper initialization, linkDesign should work!
        logger.info("Attempting linkDesign with proper initialization...");
        bool link_success = sta_impl_->sta_->linkDesign(module_name.c_str(), true);
        if (!link_success) {
            logger.error("Failed to link design with top module: " + module_name);
            
            // Try TCL command as fallback
            logger.info("Trying TCL command as fallback...");
            std::string tcl_result;
            if (sta_impl_->executeTcl("link_design " + module_name, &tcl_result)) {
                logger.info("Design linked via TCL command");
                link_success = true;
            } else {
                logger.error("TCL link failed: " + tcl_result);
            }
        }
        
        if (link_success) {
            logger.info("Design linked successfully");
            
            // Get and report statistics
            sta::Network* network = sta_impl_->network_;
            sta::Instance* top_inst = network->topInstance();
            if (top_inst) {
                logger.info("Top instance: " + std::string(network->name(top_inst)));
                
                // Count instances using TCL (more reliable)
                std::string inst_count;
                if (sta_impl_->executeTcl("llength [get_cells -hierarchical *]", &inst_count)) {
                    logger.info("Number of instances: " + inst_count);
                }
                
                // Count nets using TCL
                std::string net_count;
                if (sta_impl_->executeTcl("llength [get_nets -hierarchical *]", &net_count)) {
                    logger.info("Number of nets: " + net_count);
                }
            }
            
            // Extract netlist data
            extractNetlistData();
            
            return true;
        } else {
            logger.error("Failed to link design");
            return false;
        }
    } catch (const std::exception& e) {
        logger.error("Exception reading Verilog: " + std::string(e.what()));
        return false;
    }
}

void OpenStaAdapter::extractNetlistData() {
    auto& logger = Logger::getInstance();
    
    if (!sta_impl_) return;
    
    logger.info("Extracting netlist data from OpenSTA...");
    
    sta::Network* network = sta_impl_->network_;
    sta::Instance* top_inst = network->topInstance();
    
    if (!top_inst) {
        logger.warning("No top instance found - using dummy data");
        
        // Create dummy data for testing
        instances_.clear();
        nets_.clear();
        pins_.clear();
        
        for (int i = 0; i < 10; i++) {
            Instance inst;
            inst.id = i;
            inst.name = "dummy_inst_" + std::to_string(i);
            inst.cell_type = "DUMMY_CELL";
            inst.is_sequential = false;
            inst.is_macro = false;
            inst.area = 1.0f;
            instances_.push_back(inst);
        }
        
        for (int i = 0; i < 8; i++) {
            Net net;
            net.id = i;
            net.name = "dummy_net_" + std::to_string(i);
            net.weight = 1.0f;
            net.instances.push_back(i);
            net.instances.push_back((i + 1) % 10);
            nets_.push_back(net);
        }
        
        data_cached_ = true;
        return;
    }
    
    // Clear existing data
    instances_.clear();
    nets_.clear();
    pins_.clear();
    
    int inst_id = 0;
    std::map<sta::Instance*, int> inst_to_id;
    
    // ========================================
    // Step 1: Add top-level ports as vertices FIRST (like OpenROAD does)
    // OpenROAD 先添加 BTerms（端口），再添加实例
    // ========================================
    sta::Cell* top_cell = network->cell(top_inst);
    int port_count = 0;
    std::map<sta::Port*, int> port_to_id;  // 端口到 vertex ID 的映射
    
    if (top_cell) {
        sta::CellPortIterator* port_iter = network->portIterator(top_cell);
        while (port_iter->hasNext()) {
            sta::Port* port = port_iter->next();
            
            // 跳过 bus 端口，只处理 bit 端口
            // OpenROAD 处理的是展开后的端口
            if (network->isBus(port)) {
                // 遍历 bus 的每个 bit
                sta::PortMemberIterator* member_iter = network->memberIterator(port);
                while (member_iter->hasNext()) {
                    sta::Port* bit_port = member_iter->next();
                    
                    Instance port_inst;
                    port_inst.id = inst_id;
                    port_inst.name = network->name(bit_port);
                    port_inst.cell_type = "PORT";
                    port_inst.is_sequential = false;
                    port_inst.is_macro = false;
                    port_inst.area = 0.0f;  // IO port has no area
                    
                    instances_.push_back(port_inst);
                    port_to_id[bit_port] = inst_id;
                    inst_id++;
                    port_count++;
                }
                delete member_iter;
            } else {
                // 非 bus 端口直接添加
                Instance port_inst;
                port_inst.id = inst_id;
                port_inst.name = network->name(port);
                port_inst.cell_type = "PORT";
                port_inst.is_sequential = false;
                port_inst.is_macro = false;
                port_inst.area = 0.0f;  // IO port has no area
                
                instances_.push_back(port_inst);
                port_to_id[port] = inst_id;
                inst_id++;
                port_count++;
            }
        }
        delete port_iter;
    }
    
    logger.info("Extracted " + std::to_string(port_count) + " ports");
    
    // ========================================
    // Step 2: Extract all leaf instances (cells with Liberty)
    // ========================================
    int skipped_no_liberty = 0;
    sta::LeafInstanceIterator* leaf_iter = network->leafInstanceIterator();
    while (leaf_iter->hasNext()) {
        sta::Instance* sta_inst = leaf_iter->next();
        
        // 检查是否有 Liberty cell（OpenROAD 的过滤条件）
        sta::LibertyCell* lib_cell = network->libertyCell(sta_inst);
        if (lib_cell == nullptr) {
            skipped_no_liberty++;
            continue;  // 跳过没有 Liberty cell 的实例
        }
        
        Instance inst;
        inst.id = inst_id;
        inst.name = network->pathName(sta_inst);
        
        // Get cell info
        sta::Cell* cell = network->cell(sta_inst);
        if (cell) {
            inst.cell_type = network->name(cell);
        }
        
        inst.is_sequential = lib_cell->hasSequentials();
        inst.is_macro = lib_cell->isMacro();
        inst.area = lib_cell->area();
        
        instances_.push_back(inst);
        inst_to_id[sta_inst] = inst_id;
        inst_id++;
        
        // Log progress every 10000 instances
        if ((inst_id - port_count) % 10000 == 0) {
            logger.info("Extracted " + std::to_string(inst_id - port_count) + " instances...");
        }
    }
    delete leaf_iter;
    
    if (skipped_no_liberty > 0) {
        logger.info("Skipped " + std::to_string(skipped_no_liberty) + " instances without Liberty cell");
    }
    
    logger.info("Extracted " + std::to_string(instances_.size()) + " vertices (" + 
                std::to_string(port_count) + " ports + " + 
                std::to_string(inst_id - port_count) + " cells)");
    
    // ========================================
    // Step 3: Extract nets/hyperedges
    // 关键：OpenSTA 的网络是按层次结构组织的
    // 我们需要遍历所有叶子实例的引脚，收集它们连接的网络
    // 使用 highestConnectedNet 获取最高层级的网络，避免重复
    // ========================================
    logger.info("Starting net extraction...");
    int net_id = 0;
    int skipped_single = 0;
    int skipped_no_driver = 0;
    int skipped_power = 0;
    
    // 使用 set 来避免重复处理同一个网络
    // 使用 highestConnectedNet 作为 key，确保同一个逻辑网络只处理一次
    std::set<const sta::Net*> processed_nets;
    std::map<const sta::Net*, int> net_to_id_map;
    
    // 方法：遍历所有叶子实例的引脚，收集它们连接的网络
    // 使用 highestConnectedNet 获取最高层级的网络
    sta::LeafInstanceIterator* leaf_iter2 = network->leafInstanceIterator();
    while (leaf_iter2->hasNext()) {
        sta::Instance* sta_inst = leaf_iter2->next();
        
        // 只处理有 Liberty cell 的实例
        if (inst_to_id.find(sta_inst) == inst_to_id.end()) {
            continue;
        }
        
        // 遍历实例的所有引脚
        sta::InstancePinIterator* pin_iter = network->pinIterator(sta_inst);
        while (pin_iter->hasNext()) {
            sta::Pin* pin = pin_iter->next();
            sta::Net* pin_net = network->net(pin);
            
            if (pin_net) {
                // 获取最高层级的网络，确保同一个逻辑网络只处理一次
                const sta::Net* highest_net = network->highestConnectedNet(pin_net);
                
                if (processed_nets.find(highest_net) == processed_nets.end()) {
                    processed_nets.insert(highest_net);
                    
                    // 跳过电源网络
                    if (network->isPower(highest_net) || network->isGround(highest_net)) {
                        skipped_power++;
                        continue;
                    }
                    
                    // 检查网络连接
                    int driver_id = -1;
                    std::set<int> loads_id;
                    
                    // 遍历网络连接的所有引脚（包括层次结构中的所有引脚）
                    sta::NetConnectedPinIterator* conn_iter = network->connectedPinIterator(highest_net);
                    while (conn_iter->hasNext()) {
                        const sta::Pin* conn_pin = conn_iter->next();
                        sta::Instance* conn_inst = network->instance(conn_pin);
                        
                        // 检查是否是叶子实例（有 Liberty cell 的实例）
                        if (conn_inst && inst_to_id.find(conn_inst) != inst_to_id.end()) {
                            int vertex_id = inst_to_id[conn_inst];
                            
                            // 判断是 driver 还是 load
                            sta::PortDirection* dir = network->direction(conn_pin);
                            if (dir->isOutput()) {
                                driver_id = vertex_id;
                            } else {
                                loads_id.insert(vertex_id);
                            }
                        }
                        // 检查是否是顶层端口
                        else if (network->isTopLevelPort(conn_pin)) {
                            // 获取端口
                            sta::Port* port = network->port(conn_pin);
                            if (port && port_to_id.find(port) != port_to_id.end()) {
                                int vertex_id = port_to_id[port];
                                // 顶层 input port 是 driver，output port 是 load
                                sta::PortDirection* dir = network->direction(conn_pin);
                                if (dir->isInput()) {
                                    driver_id = vertex_id;
                                } else {
                                    loads_id.insert(vertex_id);
                                }
                            }
                        }
                    }
                    delete conn_iter;
                    
                    // 构建超边：driver + loads
                    std::vector<int> hyperedge;
                    if (driver_id != -1 && !loads_id.empty()) {
                        hyperedge.push_back(driver_id);
                        for (int load_id : loads_id) {
                            if (load_id != driver_id) {
                                hyperedge.push_back(load_id);
                            }
                        }
                    }
                    
                    // 只保留连接 2 个以上顶点的网络
                    if (hyperedge.size() >= 2) {
                        Net net_obj;
                        net_obj.id = net_id;
                        net_obj.name = network->pathName(highest_net);
                        net_obj.weight = 1.0f;
                        net_obj.instances = hyperedge;
                        
                        nets_.push_back(net_obj);
                        net_to_id_map[highest_net] = net_id;
                        net_id++;
                        
                        if (net_id % 10000 == 0) {
                            logger.info("Extracted " + std::to_string(net_id) + " nets...");
                        }
                    } else if (hyperedge.size() == 1) {
                        skipped_single++;
                    } else {
                        skipped_no_driver++;
                    }
                }
            }
        }
        delete pin_iter;
    }
    delete leaf_iter2;
    
    logger.info("Extracted " + std::to_string(nets_.size()) + " nets");
    logger.info("Skipped " + std::to_string(skipped_power) + " power nets, " +
                std::to_string(skipped_single) + " single-vertex nets, " +
                std::to_string(skipped_no_driver) + " no-driver nets");
    
    data_cached_ = true;
}

bool OpenStaAdapter::readSDC(const std::string& filename) {
    auto& logger = Logger::getInstance();
    
    if (!sta_impl_) {
        logger.error("OpenSTA not initialized");
        return false;
    }
    
    logger.info("Reading SDC file: " + filename);
    
    try {
        // Use TCL to source the SDC file
        std::string cmd = "source " + filename;
        std::string result;
        if (sta_impl_->executeTcl(cmd, &result)) {
            logger.info("SDC file loaded successfully");
            return true;
        } else {
            logger.error("Failed to load SDC: " + result);
            return false;
        }
    } catch (const std::exception& e) {
        logger.error("Exception reading SDC: " + std::string(e.what()));
        return false;
    }
}

bool OpenStaAdapter::runTimingAnalysis() {
    auto& logger = Logger::getInstance();
    
    if (!sta_impl_) {
        logger.error("OpenSTA not initialized");
        return false;
    }
    
    logger.info("Running timing analysis...");
    
    try {
        sta::Sta* sta = sta_impl_->sta_;
        
        // Ensure graph is built
        sta->ensureGraph();
        
        // Search preamble
        sta->searchPreamble();
        
        // Update timing
        sta->updateTiming(false);
        
        logger.info("Timing analysis completed");
        return true;
    } catch (const std::exception& e) {
        logger.error("Timing analysis failed: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<Hypergraph> OpenStaAdapter::buildHypergraph() {
    auto& logger = Logger::getInstance();
    logger.info("Building hypergraph from adapter...");
    
    if (!data_cached_) {
        logger.error("No netlist data available");
        return nullptr;
    }
    
    // Prepare data structures for Hypergraph constructor
    // 超图定义：
    //   - Vertices (顶点) = Instances (实例/单元)
    //   - Hyperedges (超边) = Nets (网络)，每个网络连接多个实例
    int vertex_dimensions = 1;
    int hyperedge_dimensions = 1;
    int placement_dimensions = 0;
    
    // Build vertex weights from instances
    // 顶点权重 = 实例面积
    std::vector<std::vector<float>> vertex_weights;
    for (const auto& inst : instances_) {
        float weight = inst.area > 0 ? inst.area : 1.0f;
        vertex_weights.push_back({weight});
    }
    
    // Build hyperedges from nets
    // 超边 = 网络连接的实例列表
    std::vector<std::vector<int>> hyperedges;
    std::vector<std::vector<float>> hyperedge_weights;
    
    for (const auto& net : nets_) {
        // 只保留连接 2 个以上实例的网络
        if (net.instances.size() >= 2) {
            hyperedges.push_back(net.instances);
            hyperedge_weights.push_back({net.weight});
        }
    }
    
    // Fixed vertices (none for now)
    std::vector<int> fixed_attr;
    
    // Community structure (for coarsening)
    std::vector<int> community_attr;
    
    // Placement attributes (empty for now)
    std::vector<std::vector<float>> placement_attr;
    
    logger.info("Building hypergraph with " + std::to_string(vertex_weights.size()) + 
                " vertices (instances) and " + std::to_string(hyperedges.size()) + " hyperedges (nets)");
    
    // Create hypergraph with proper constructor
    // 参数顺序: hyperedges, vertex_weights, hyperedge_weights
    auto hg = std::make_shared<Hypergraph>(
        vertex_dimensions,
        hyperedge_dimensions,
        placement_dimensions,
        hyperedges,           // 超边（网络连接）
        vertex_weights,       // 顶点权重（实例面积）
        hyperedge_weights,    // 超边权重
        fixed_attr,           // 固定顶点
        community_attr,       // 社区属性
        placement_attr,       // 布局属性
        &Logger::getInstance()
    );
    
    logger.info("Hypergraph built: " + std::to_string(hg->GetNumVertices()) + 
                " vertices, " + std::to_string(hg->GetNumHyperedges()) + " hyperedges");
    
    return hg;
}

// Forward declarations for timing path extraction helpers
std::vector<TimingPath> extractTimingPathsFromSTA(
    sta::Sta* sta, 
    const std::map<sta::Instance*, int>& inst_to_id,
    const std::map<sta::Net*, int>& net_to_id,
    int max_paths,
    Logger& logger);

std::map<sta::Instance*, int> buildInstanceToIdMap(
    sta::Network* network,
    const std::vector<Instance>& instances);

std::map<sta::Net*, int> buildNetToIdMap(
    sta::Network* network,
    const std::vector<Net>& nets);

std::vector<TimingPath> OpenStaAdapter::getCriticalPaths(int max_paths) const {
    auto& logger = Logger::getInstance();
    std::vector<TimingPath> paths;
    
    if (!sta_impl_ || !data_cached_) {
        logger.warning("OpenSTA not initialized or no data cached");
        return paths;
    }
    
    logger.info("Extracting critical timing paths (max " + std::to_string(max_paths) + ")...");
    
    // Build mapping from STA objects to our IDs
    auto inst_to_id = buildInstanceToIdMap(sta_impl_->network_, instances_);
    auto net_to_id = buildNetToIdMap(sta_impl_->network_, nets_);
    
    // Extract timing paths using the helper function
    paths = extractTimingPathsFromSTA(
        sta_impl_->sta_,
        inst_to_id,
        net_to_id,
        max_paths,
        logger
    );
    
    logger.info("Extracted " + std::to_string(paths.size()) + " timing paths");
    
    return paths;
}

std::vector<Instance> OpenStaAdapter::getInstances() const {
    return instances_;
}

std::vector<Net> OpenStaAdapter::getNets() const {
    return nets_;
}

std::vector<Pin> OpenStaAdapter::getPins() const {
    return pins_;
}

void OpenStaAdapter::clearCache() {
    data_cached_ = false;
    instances_.clear();
    nets_.clear();
    pins_.clear();
}

int OpenStaAdapter::getNumInstances() const {
    return instances_.size();
}

int OpenStaAdapter::getNumNets() const {
    return nets_.size();
}

int OpenStaAdapter::getNumPins() const {
    return pins_.size();
}

float OpenStaAdapter::getNetSlack(int net_id) const {
    // TODO: Implement actual slack extraction from timing analysis
    return 0.0f;
}

void OpenStaAdapter::buildDataCache() const {
    // This method is for lazy loading if needed
    // Currently we build data during readNetlist
}

} // namespace par
