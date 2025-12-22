// Test to understand OpenSTA behavior - improved version

#include "sta/Sta.hh"
#include "sta/Network.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Corner.hh"
#include "sta/StaMain.hh"
#include "sta/PortDirection.hh"
#include <tcl.h>
#include <iostream>
#include <string>

// Swig uses C linkage for init functions.
extern "C" {
extern int Sta_Init(Tcl_Interp *interp);
}

// External TCL init array from OpenSTA
namespace sta {
extern const char *tcl_inits[];
}

// Simple TCL evaluation helper
int evalTclCmd(Tcl_Interp* interp, const std::string& cmd) {
    std::cout << "Executing TCL: " << cmd << "\n";
    int result = Tcl_Eval(interp, cmd.c_str());
    if (result != TCL_OK) {
        std::cerr << "TCL Error: " << Tcl_GetStringResult(interp) << "\n";
    }
    return result;
}

int main() {
    std::cout << "=== OpenSTA Test - Improved Version ===\n\n";
    
    // Step 1: Initialize TCL
    std::cout << "Step 1: Initializing TCL...\n";
    Tcl_Interp* tcl_interp = Tcl_CreateInterp();
    if (!tcl_interp) {
        std::cerr << "Failed to create TCL interpreter\n";
        return 1;
    }
    
    if (Tcl_Init(tcl_interp) != TCL_OK) {
        std::cerr << "Failed to init TCL\n";
        return 1;
    }
    std::cout << "TCL initialized successfully\n\n";
    
    // Step 2: Initialize STA core
    std::cout << "Step 2: Initializing STA core...\n";
    sta::initSta();
    sta::Sta* sta = new sta::Sta;
    sta::Sta::setSta(sta);
    sta->makeComponents();
    sta->setTclInterp(tcl_interp);
    
    // Step 3: Initialize Swig TCL commands - CRITICAL for avoiding segfault!
    std::cout << "Step 3: Initializing Swig TCL commands...\n";
    if (Sta_Init(tcl_interp) != TCL_OK) {
        std::cerr << "Failed to initialize Sta TCL commands\n";
        return 1;
    }
    std::cout << "Swig TCL commands registered successfully!\n";
    
    // Step 4: Initialize TCL scripts
    std::cout << "Step 4: Loading OpenSTA TCL scripts...\n";
    sta::evalTclInit(tcl_interp, sta::tcl_inits);
    Tcl_Eval(tcl_interp, "init_sta_cmds");
    std::cout << "TCL scripts loaded\n\n";
    
    // Try TCL commands approach first
    std::cout << "Step 4: Testing TCL command approach...\n";
    std::cout << "---------------------------------------\n";
    
    sta::Network* network = sta->network();
    sta::Corner* corner = sta->cmdCorner();
    
    std::cout << "STA initialized with TCL support\n\n";
    
    // Test with C++ API directly
    std::string lib_file = "/home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/Nangate45/Nangate45_typ.lib";
    std::string verilog_file = "/home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/ariane.v";
    
    std::cout << "Testing C++ API with improved initialization\n";
    std::cout << "--------------------------------------------\n";
    
    // Step A: Read Liberty
    std::cout << "\nStep A: Reading Liberty file...\n";
    sta::LibertyLibrary* lib = sta->readLiberty(
        lib_file.c_str(),
        corner,
        sta::MinMaxAll::all(),
        true
    );
    
    if (lib) {
        std::cout << "SUCCESS: Liberty loaded: " << lib->name() << "\n";
    } else {
        std::cerr << "FAILED: Could not load Liberty\n";
    }
    
    // Step B: Read Verilog
    std::cout << "\nStep B: Reading Verilog file...\n";
    bool verilog_ok = sta->readVerilog(verilog_file.c_str());
    if (verilog_ok) {
        std::cout << "SUCCESS: Verilog read OK\n";
    } else {
        std::cerr << "FAILED: Verilog read failed\n";
    }
    
    // Check if we have any cells loaded
    if (lib) {
        std::cout << "Liberty library " << lib->name() << " is loaded\n";
        // Note: cell iterator API might be different
    }
    
    // Step B2: Check what cells/modules are available
    std::cout << "\nStep B2: Checking available modules...\n";
    
    // Try to find cells in the library
    if (lib) {
        // LibertyLibrary is-a Library, no cast needed
        sta::Cell* ariane_cell = network->findCell(lib, "ariane");
        if (ariane_cell) {
            std::cout << "Found 'ariane' cell in Liberty: " << network->name(ariane_cell) << "\n";
        } else {
            std::cout << "Cell 'ariane' not found in Liberty library\n";
        }
    }
    
    // The issue is that 'ariane' is a Verilog module, not a Liberty cell
    std::cout << "Note: 'ariane' is a Verilog module, not a Liberty cell\n";
    
    // Step C: Check network state before linking
    std::cout << "\nStep C: Checking network state before linking...\n";
    sta::Instance* top_before = network->topInstance();
    if (top_before) {
        std::cout << "Top instance exists before link: " << network->name(top_before) << "\n";
    } else {
        std::cout << "No top instance before link (expected)\n";
    }
    
    // Step D: Try different linking approaches
    std::cout << "\nStep D: Attempting to link design...\n";
    std::cout << "WARNING: This is where it usually crashes\n";
    std::cout << "Trying with makeBlackBoxes=false first...\n";
    
    try {
        // Try with makeBlackBoxes=false
        bool link_ok = sta->linkDesign("ariane", false);
        if (link_ok) {
            std::cout << "SUCCESS: Link OK with makeBlackBoxes=false\n";
        } else {
            std::cerr << "FAILED: Link returned false\n";
            
            // Try with makeBlackBoxes=true
            std::cout << "Trying with makeBlackBoxes=true...\n";
            link_ok = sta->linkDesign("ariane", true);
            if (link_ok) {
                std::cout << "SUCCESS: Link OK with makeBlackBoxes=true\n";
            } else {
                std::cerr << "FAILED: Link still failed\n";
            }
        }
        
        // Check result
        sta::Instance* top_after = network->topInstance();
        if (top_after) {
            std::cout << "\nSUCCESS: Top instance after link: " << network->name(top_after) << "\n";
            
            // Count children
            int child_count = 0;
            sta::InstanceChildIterator* child_iter = network->childIterator(top_after);
            while (child_iter->hasNext()) {
                child_iter->next();
                child_count++;
            }
            delete child_iter;
            std::cout << "Total number of child instances: " << child_count << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION during linkDesign\n";
    }
    
    std::cout << "\n=== Test Complete ===\n";
    
    delete sta;
    Tcl_DeleteInterp(tcl_interp);
    
    return 0;
}
