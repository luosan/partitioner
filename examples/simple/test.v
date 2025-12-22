// Simple test netlist for TritonPart standalone
module top (
    input clk,
    input rst,
    input [7:0] in_data,
    output reg [7:0] out_data
);

// Registers
reg [7:0] reg1, reg2, reg3, reg4;
reg [7:0] pipe1, pipe2;

// Combinational logic
wire [7:0] sum1, sum2;
wire [7:0] mult1, mult2;

// First stage
assign sum1 = reg1 + reg2;
assign mult1 = reg1 * 2;

// Second stage
assign sum2 = reg3 + reg4;
assign mult2 = reg3 * 3;

// Pipeline registers
always @(posedge clk) begin
    if (rst) begin
        reg1 <= 8'b0;
        reg2 <= 8'b0;
        reg3 <= 8'b0;
        reg4 <= 8'b0;
        pipe1 <= 8'b0;
        pipe2 <= 8'b0;
        out_data <= 8'b0;
    end else begin
        // Input registers
        reg1 <= in_data;
        reg2 <= reg1;
        reg3 <= reg2;
        reg4 <= reg3;
        
        // Pipeline
        pipe1 <= sum1 + mult1;
        pipe2 <= sum2 + mult2;
        
        // Output
        out_data <= pipe1 + pipe2;
    end
end

endmodule
