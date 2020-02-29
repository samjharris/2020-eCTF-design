create_generated_clock -name ja0 -source [get_pins system_i/i2s_output_1/U0/mclk_ODDR/C] -divide_by 1 [get_ports ja0]
set_clock_groups -asynchronous -group [get_clocks clk_fpga_0] -group [get_clocks clk_out1_system_clk_wiz_25M_0]
connect_debug_port u_ila_0/clk [get_nets [list u_ila_0_clk_out1]]
connect_debug_port u_ila_1/clk [get_nets [list u_ila_1_FCLK_CLK0]]
connect_debug_port dbg_hub/clk [get_nets u_ila_1_FCLK_CLK0]







connect_debug_port dbg_hub/clk [get_nets u_ila_0_FCLK_CLK0]


connect_debug_port u_ila_0/probe2 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[0]}]]
connect_debug_port u_ila_0/probe3 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[1]}]]
connect_debug_port u_ila_0/probe4 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[2]}]]
connect_debug_port u_ila_0/probe5 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[3]}]]
connect_debug_port u_ila_0/probe6 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[4]}]]
connect_debug_port u_ila_0/probe7 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[5]}]]
connect_debug_port u_ila_0/probe8 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[6]}]]
connect_debug_port u_ila_0/probe9 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[7]}]]
connect_debug_port u_ila_0/probe10 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[8]}]]
connect_debug_port u_ila_0/probe11 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[9]}]]
connect_debug_port u_ila_0/probe12 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[10]}]]
connect_debug_port u_ila_0/probe13 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[11]}]]
connect_debug_port u_ila_0/probe14 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[12]}]]
connect_debug_port u_ila_0/probe15 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[13]}]]
connect_debug_port u_ila_0/probe16 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[14]}]]
connect_debug_port u_ila_0/probe17 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[15]}]]
connect_debug_port u_ila_0/probe18 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[16]}]]
connect_debug_port u_ila_0/probe19 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[17]}]]
connect_debug_port u_ila_0/probe20 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[18]}]]
connect_debug_port u_ila_0/probe21 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[19]}]]
connect_debug_port u_ila_0/probe22 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[20]}]]
connect_debug_port u_ila_0/probe23 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[21]}]]
connect_debug_port u_ila_0/probe24 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[22]}]]
connect_debug_port u_ila_0/probe25 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[23]}]]
connect_debug_port u_ila_0/probe26 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[24]}]]
connect_debug_port u_ila_0/probe27 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[25]}]]
connect_debug_port u_ila_0/probe28 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[26]}]]
connect_debug_port u_ila_0/probe29 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[27]}]]
connect_debug_port u_ila_0/probe30 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[28]}]]
connect_debug_port u_ila_0/probe31 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[29]}]]
connect_debug_port u_ila_0/probe32 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[30]}]]
connect_debug_port u_ila_0/probe33 [get_nets [list {system_i/AXIS_AES_CTR_0/inst/axi_handshake_section.axi_ctrl_awaddr_internal_reg_n_0_[31]}]]






set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets clk]
