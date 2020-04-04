create_generated_clock -name ja0 -source [get_pins system_i/i2s_output_1/U0/mclk_ODDR/C] -divide_by 1 [get_ports ja0]
set_clock_groups -asynchronous -group [get_clocks clk_fpga_0] -group [get_clocks clk_out1_system_clk_wiz_25M_0]

set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets clk]
