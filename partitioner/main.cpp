// SPDX-License-Identifier: BSD-3-Clause
// TritonPart Standalone Tool
// Supports triton_part_design and evaluate_part_design_solution

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

#include "TritonPartCore.h"
#include "adapter/OpenStaAdapter.h"
#include "src/Hypergraph.h"
#include "src/Evaluator.h"
#include "utils/Logger.h"

using namespace par;

enum class Mode {
    PARTITION,   // triton_part_design
    EVALUATE,    // evaluate_part_design_solution
    HELP
};

struct Options {
    Mode mode = Mode::PARTITION;
    
    // Input files
    std::string verilog_file;
    std::string top_module;
    std::vector<std::string> liberty_files;
    std::string sdc_file;
    
    // Partition parameters
    int num_parts = 2;
    float balance_constraint = 2.0;
    int seed = 0;
    int top_n = 100000;
    bool timing_aware = false;
    float extra_delay = 1e-9;
    bool guardband = false;
    
    // Output files
    std::string solution_file = "partition.part";
    std::string hypergraph_file;           // For evaluate mode
    std::string hypergraph_int_weight_file; // For evaluate mode (hMETIS format)
    
    // Debug
    bool debug = false;
};

void printUsage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " <command> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  partition    Run triton_part_design (default)" << std::endl;
    std::cout << "  evaluate     Run evaluate_part_design_solution" << std::endl;
    std::cout << std::endl;
    std::cout << "Common Options:" << std::endl;
    std::cout << "  -v <verilog>      Verilog netlist file (required)" << std::endl;
    std::cout << "  -m <module>       Top module name (required)" << std::endl;
    std::cout << "  -l <liberty>      Liberty library file (can specify multiple)" << std::endl;
    std::cout << "  -s <sdc>          SDC constraints file" << std::endl;
    std::cout << "  -n <num_parts>    Number of partitions (default: 2)" << std::endl;
    std::cout << "  -b <balance>      Balance constraint (default: 2.0)" << std::endl;
    std::cout << "  --seed <seed>     Random seed (default: 0)" << std::endl;
    std::cout << "  -t                Enable timing-aware partitioning" << std::endl;
    std::cout << "  --top_n <n>       Top N timing paths (default: 100000)" << std::endl;
    std::cout << "  --extra_delay <d> Extra delay for cuts (default: 1e-9)" << std::endl;
    std::cout << "  --guardband       Enable timing guardband" << std::endl;
    std::cout << "  -d                Enable debug logging" << std::endl;
    std::cout << "  -h, --help        Print this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Partition Mode Options:" << std::endl;
    std::cout << "  -o <output>       Solution output file (default: partition.part)" << std::endl;
    std::cout << std::endl;
    std::cout << "Evaluate Mode Options:" << std::endl;
    std::cout << "  --solution <file>     Solution file to evaluate (required)" << std::endl;
    std::cout << "  --hypergraph <file>   Output hypergraph file (weighted)" << std::endl;
    std::cout << "  --hypergraph_int <f>  Output hypergraph file (integer weights, hMETIS format)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Partition a design" << std::endl;
    std::cout << "  " << prog_name << " partition -v design.v -m top -l lib.lib -s design.sdc -n 4 -t -o result.part" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Evaluate a partition solution and generate hypergraph" << std::endl;
    std::cout << "  " << prog_name << " evaluate -v design.v -m top -l lib.lib -s design.sdc -n 4 -t \\" << std::endl;
    std::cout << "      --solution result.part --hypergraph design.hgr.wt --hypergraph_int design.hgr.int" << std::endl;
}

Options parseArgs(int argc, char* argv[]) {
    Options opts;
    
    if (argc < 2) {
        opts.mode = Mode::HELP;
        return opts;
    }
    
    // Parse command (first argument)
    std::string cmd = argv[1];
    if (cmd == "partition") {
        opts.mode = Mode::PARTITION;
    } else if (cmd == "evaluate") {
        opts.mode = Mode::EVALUATE;
    } else if (cmd == "-h" || cmd == "--help" || cmd == "help") {
        opts.mode = Mode::HELP;
        return opts;
    } else if (cmd[0] == '-') {
        // No command specified, default to partition, reparse from arg 1
        opts.mode = Mode::PARTITION;
        // Process this argument below
    } else {
        std::cerr << "Unknown command: " << cmd << std::endl;
        opts.mode = Mode::HELP;
        return opts;
    }
    
    // Determine start index for options
    int start_idx = (cmd[0] == '-') ? 1 : 2;
    
    for (int i = start_idx; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-v" && i + 1 < argc) {
            opts.verilog_file = argv[++i];
        } else if (arg == "-m" && i + 1 < argc) {
            opts.top_module = argv[++i];
        } else if (arg == "-l" && i + 1 < argc) {
            opts.liberty_files.push_back(argv[++i]);
        } else if (arg == "-s" && i + 1 < argc) {
            opts.sdc_file = argv[++i];
        } else if (arg == "-n" && i + 1 < argc) {
            opts.num_parts = std::atoi(argv[++i]);
        } else if (arg == "-b" && i + 1 < argc) {
            opts.balance_constraint = std::atof(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            opts.seed = std::atoi(argv[++i]);
        } else if (arg == "-t") {
            opts.timing_aware = true;
        } else if (arg == "--top_n" && i + 1 < argc) {
            opts.top_n = std::atoi(argv[++i]);
        } else if (arg == "--extra_delay" && i + 1 < argc) {
            opts.extra_delay = std::atof(argv[++i]);
        } else if (arg == "--guardband") {
            opts.guardband = true;
        } else if (arg == "-o" && i + 1 < argc) {
            opts.solution_file = argv[++i];
        } else if (arg == "--solution" && i + 1 < argc) {
            opts.solution_file = argv[++i];
        } else if (arg == "--hypergraph" && i + 1 < argc) {
            opts.hypergraph_file = argv[++i];
        } else if (arg == "--hypergraph_int" && i + 1 < argc) {
            opts.hypergraph_int_weight_file = argv[++i];
        } else if (arg == "-d") {
            opts.debug = true;
        } else if (arg == "-h" || arg == "--help") {
            opts.mode = Mode::HELP;
            return opts;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
        }
    }
    
    return opts;
}

int runPartition(const Options& opts) {
    auto& logger = Logger::getInstance();
    
    logger.info("========================================");
    logger.info("TritonPart Design Partitioning");
    logger.info("========================================");
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Create adapter and core
        auto adapter = std::make_shared<OpenStaAdapter>();
        TritonPartCore core;
        core.setAdapter(adapter);
        
        // Configure parameters
        core.setNumPartitions(opts.num_parts);
        core.setBalance(opts.balance_constraint);
        core.setTimingAware(opts.timing_aware);
        
        logger.info("Configuration:");
        logger.info("  Partitions: " + std::to_string(opts.num_parts));
        logger.info("  Balance constraint: " + std::to_string(opts.balance_constraint));
        logger.info("  Timing-aware: " + std::string(opts.timing_aware ? "yes" : "no"));
        if (opts.timing_aware) {
            logger.info("  Top N paths: " + std::to_string(opts.top_n));
            logger.info("  Extra delay: " + std::to_string(opts.extra_delay));
            logger.info("  Guardband: " + std::string(opts.guardband ? "yes" : "no"));
        }
        
        // Read input files
        for (const auto& lib_file : opts.liberty_files) {
            logger.info("Reading Liberty: " + lib_file);
            if (!core.readLiberty(lib_file)) {
                logger.error("Failed to read Liberty file: " + lib_file);
                return 1;
            }
        }
        
        logger.info("Reading Verilog: " + opts.verilog_file);
        if (!core.readNetlist(opts.verilog_file, opts.top_module)) {
            logger.error("Failed to read Verilog file");
            return 1;
        }
        
        if (!opts.sdc_file.empty()) {
            logger.info("Reading SDC: " + opts.sdc_file);
            if (!core.readSDC(opts.sdc_file)) {
                logger.error("Failed to read SDC file");
                return 1;
            }
        }
        
        // Build hypergraph
        logger.info("Building hypergraph...");
        if (!core.buildHypergraph()) {
            logger.error("Failed to build hypergraph");
            return 1;
        }
        
        // Extract timing paths
        if (opts.timing_aware) {
            logger.info("Extracting timing paths...");
            if (!core.extractTimingPaths()) {
                logger.warning("Failed to extract timing paths");
            }
        }
        
        // Perform partitioning
        logger.info("Starting partitioning...");
        if (!core.partition()) {
            logger.error("Partitioning failed");
            return 1;
        }
        
        // Report metrics
        core.reportPartitionMetrics();
        
        // Write output
        if (!core.writePartitionResult(opts.solution_file)) {
            logger.error("Failed to write output file");
            return 1;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        logger.info("========================================");
        logger.info("Partitioning completed successfully!");
        logger.info("Total runtime: " + std::to_string(duration.count()) + " ms");
        logger.info("Output written to: " + opts.solution_file);
        logger.info("========================================");
        
    } catch (const std::exception& e) {
        logger.error("Exception: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}

int runEvaluate(const Options& opts) {
    auto& logger = Logger::getInstance();
    
    logger.info("========================================");
    logger.info("TritonPart Solution Evaluation");
    logger.info("========================================");
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Create adapter
        auto adapter = std::make_shared<OpenStaAdapter>();
        
        // Configure and read input
        logger.info("Configuration:");
        logger.info("  Partitions: " + std::to_string(opts.num_parts));
        logger.info("  Balance constraint: " + std::to_string(opts.balance_constraint));
        logger.info("  Timing-aware: " + std::string(opts.timing_aware ? "yes" : "no"));
        logger.info("  Solution file: " + opts.solution_file);
        
        // Read input files
        for (const auto& lib_file : opts.liberty_files) {
            logger.info("Reading Liberty: " + lib_file);
            if (!adapter->readLiberty(lib_file)) {
                logger.error("Failed to read Liberty file: " + lib_file);
                return 1;
            }
        }
        
        logger.info("Reading Verilog: " + opts.verilog_file);
        if (!adapter->readNetlist(opts.verilog_file, opts.top_module)) {
            logger.error("Failed to read Verilog file");
            return 1;
        }
        
        if (!opts.sdc_file.empty()) {
            logger.info("Reading SDC: " + opts.sdc_file);
            if (!adapter->readSDC(opts.sdc_file)) {
                logger.error("Failed to read SDC file");
                return 1;
            }
        }
        
        // Run timing analysis if timing-aware
        if (opts.timing_aware) {
            logger.info("Running timing analysis...");
            adapter->runTimingAnalysis();
        }
        
        // Build hypergraph
        logger.info("Building hypergraph...");
        auto hypergraph = adapter->buildHypergraph();
        if (!hypergraph) {
            logger.error("Failed to build hypergraph");
            return 1;
        }
        
        logger.info("Hypergraph: " + std::to_string(hypergraph->GetNumVertices()) + 
                   " vertices, " + std::to_string(hypergraph->GetNumHyperedges()) + " hyperedges");
        
        // Create evaluator
        std::vector<float> e_wt_factors = {1.0f};
        std::vector<float> v_wt_factors = {1.0f};
        std::vector<float> placement_wt_factors = {};
        float net_timing_factor = opts.timing_aware ? 1.0f : 0.0f;
        float path_timing_factor = opts.timing_aware ? 1.0f : 0.0f;
        float path_snaking_factor = opts.timing_aware ? 1.0f : 0.0f;
        float timing_exp_factor = 2.0f;
        
        auto evaluator = std::make_shared<GoldenEvaluator>(
            opts.num_parts,
            e_wt_factors,
            v_wt_factors,
            placement_wt_factors,
            net_timing_factor,
            path_timing_factor,
            path_snaking_factor,
            timing_exp_factor,
            opts.extra_delay,
            hypergraph,  // timing graph
            &logger
        );
        
        // Initialize timing if enabled
        if (opts.timing_aware) {
            evaluator->InitializeTiming(hypergraph);
        }
        
        // Write hypergraph files if requested
        if (!opts.hypergraph_file.empty()) {
            logger.info("Writing weighted hypergraph: " + opts.hypergraph_file);
            evaluator->WriteWeightedHypergraph(hypergraph, opts.hypergraph_file);
        }
        
        if (!opts.hypergraph_int_weight_file.empty()) {
            logger.info("Writing integer weight hypergraph: " + opts.hypergraph_int_weight_file);
            evaluator->WriteIntWeightHypergraph(hypergraph, opts.hypergraph_int_weight_file);
        }
        
        // Read and evaluate solution file
        if (!opts.solution_file.empty()) {
            logger.info("Reading solution file: " + opts.solution_file);
            
            std::vector<int> solution;
            std::ifstream sol_file(opts.solution_file);
            if (!sol_file.is_open()) {
                logger.error("Cannot open solution file: " + opts.solution_file);
                return 1;
            }
            
            std::string line;
            while (std::getline(sol_file, line)) {
                // Skip empty lines and comment lines
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                
                // Parse line: could be "vertex_id partition_id" or just "partition_id"
                std::istringstream iss(line);
                int first_val, second_val;
                if (iss >> first_val) {
                    if (iss >> second_val) {
                        // Format: vertex_id partition_id
                        solution.push_back(second_val);
                    } else {
                        // Format: partition_id only
                        solution.push_back(first_val);
                    }
                }
            }
            sol_file.close();
            
            logger.info("Solution loaded: " + std::to_string(solution.size()) + " vertices");
            
            if (static_cast<int>(solution.size()) != hypergraph->GetNumVertices()) {
                logger.warning("Solution size (" + std::to_string(solution.size()) + 
                             ") != hypergraph vertices (" + std::to_string(hypergraph->GetNumVertices()) + ")");
            }
            
            // Evaluate partition
            std::vector<float> base_balance(opts.num_parts, 1.0f / opts.num_parts);
            std::vector<std::vector<int>> group_attr;  // empty groups
            
            evaluator->ConstraintAndCutEvaluator(hypergraph, solution, 
                                                  opts.balance_constraint, base_balance,
                                                  group_attr, true);
            
            // Display timing path cuts if timing-aware
            if (opts.timing_aware) {
                logger.report("Display Timing Path Cuts Statistics");
                auto path_stats = evaluator->GetTimingCuts(hypergraph, solution);
                evaluator->PrintPathStats(path_stats);
            }
            
            // Print cutsize matrix between partitions
            evaluator->PrintCutsizeMatrix(hypergraph, solution);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        logger.info("========================================");
        logger.info("Evaluation completed successfully!");
        logger.info("Total runtime: " + std::to_string(duration.count()) + " ms");
        logger.info("========================================");
        
    } catch (const std::exception& e) {
        logger.error("Exception: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    Options opts = parseArgs(argc, argv);
    
    // Print banner
    std::cout << "================================================" << std::endl;
    std::cout << "       TritonPart Standalone Tool v1.0          " << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;
    
    if (opts.mode == Mode::HELP) {
        printUsage(argv[0]);
        return 0;
    }
    
    // Initialize logger
    auto& logger = Logger::getInstance();
    logger.setLevel(opts.debug ? LogLevel::DEBUG : LogLevel::INFO);
    
    // Validate common required options
    if (opts.verilog_file.empty()) {
        logger.error("Verilog file is required (-v)");
        printUsage(argv[0]);
        return 1;
    }
    
    if (opts.top_module.empty()) {
        logger.error("Top module name is required (-m)");
        printUsage(argv[0]);
        return 1;
    }
    
    if (opts.liberty_files.empty()) {
        logger.error("At least one Liberty file is required (-l)");
        printUsage(argv[0]);
        return 1;
    }
    
    // Run appropriate mode
    switch (opts.mode) {
        case Mode::PARTITION:
            return runPartition(opts);
        case Mode::EVALUATE:
            return runEvaluate(opts);
        default:
            printUsage(argv[0]);
            return 1;
    }
}
