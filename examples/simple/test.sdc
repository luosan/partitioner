# Simple SDC constraints for test
create_clock -name clk -period 10 [get_ports clk]
set_input_delay -clock clk 2 [get_ports in_data*]
set_output_delay -clock clk 2 [get_ports out_data*]
