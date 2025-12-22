# Test timing path extraction with OpenSTA
puts "==============================================="
puts "Testing timing path extraction"
puts "==============================================="

# Step 1: Read Liberty library
puts "\n1. Reading Liberty library..."
read_liberty /home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/Nangate45/Nangate45_typ.lib

# Step 2: Read Verilog file
puts "\n2. Reading Verilog file..."
read_verilog /home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/ariane.v

# Step 3: Link the design
puts "\n3. Linking design with top module 'ariane'..."
link_design ariane

# Step 4: Read SDC constraints
puts "\n4. Reading SDC constraints..."
source /home/lzx/work/OpenROAD-flow-scripts/tools/OpenROAD/src/par/examples/timing-aware-partitioning/ariane.sdc

# Step 5: Report timing paths
puts "\n5. Reporting timing paths..."
report_checks -path_delay max -through * -max_paths 10
puts ""
report_checks -path_delay min -through * -max_paths 10

# Step 6: Report statistics
puts "\n6. Design statistics:"
puts "Number of instances: [llength [get_cells -hierarchical *]]"
puts "Number of nets: [llength [get_nets -hierarchical *]]"
puts "Number of ports: [llength [get_ports *]]"

# Step 7: Report worst timing paths
puts "\n7. Worst timing paths:"
report_checks -path_delay max -slack_max 0.0 -max_paths 100 > worst_paths.txt
puts "Worst paths written to worst_paths.txt"

puts "\n==============================================="
puts "Test completed successfully!"
puts "==============================================="
