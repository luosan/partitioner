# Test ariane design with OpenSTA
puts "==============================================="
puts "Testing ariane design with OpenSTA"
puts "==============================================="

# Step 1: Read Liberty library
puts "\nStep 1: Reading Liberty library..."
read_liberty /home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/Nangate45/Nangate45_typ.lib

# Step 2: Read Verilog file
puts "\nStep 2: Reading Verilog file..."
read_verilog /home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/ariane.v

# Step 3: Link the design
puts "\nStep 3: Linking design with top module 'ariane'..."
link_design ariane

# Step 4: Check what we have
puts "\nStep 4: Checking design statistics..."
puts "Top instance: [current_instance]"

# Get some basic statistics
set top_inst [current_instance]
puts "Top instance name: $top_inst"

# Count instances (this is TCL script, so we can use TCL commands)
set inst_count [llength [get_cells -hierarchical *]]
puts "Number of instances: $inst_count"

# Count nets
set net_count [llength [get_nets -hierarchical *]]
puts "Number of nets: $net_count"

# Count ports
set port_count [llength [get_ports *]]
puts "Number of ports: $port_count"

puts "\n==============================================="
puts "Test completed successfully!"
puts "==============================================="
