library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

use IEEE.NUMERIC_STD.ALL;

entity tb is
end tb;

architecture Behavioral of tb is
component AXIS_AES_CTR_INTERFACE is
generic (
    ctrl_data_width: natural := 32;
    stream_data_width: natural := 32;
    
    C_AXI_CTRL_BASEADDR : UNSIGNED(31 downto 0) := x"05000000";

    C_KEY_SIZE: natural := 256
);
port (
    aclk: in STD_LOGIC;
    aresetn: in STD_LOGIC;
    
    axi_ctrl_araddr: in STD_LOGIC_VECTOR(31 downto 0);
    axi_ctrl_arready: out STD_LOGIC;
    axi_ctrl_arvalid: in STD_LOGIC;
    
    axi_ctrl_rdata: out STD_LOGIC_VECTOR(ctrl_data_width-1 downto 0);
    axi_ctrl_rresp: out STD_LOGIC_VECTOR(1 downto 0);
    axi_ctrl_rready: in STD_LOGIC;
    axi_ctrl_rvalid: out STD_LOGIC;
    
    axi_ctrl_awaddr: in STD_LOGIC_VECTOR(31 downto 0);
    axi_ctrl_awready: out STD_LOGIC;
    axi_ctrl_awvalid: in STD_LOGIC;
    
    axi_ctrl_wdata: in STD_LOGIC_VECTOR(ctrl_data_width-1 downto 0);
    axi_ctrl_wstrb: in STD_LOGIC_VECTOR(ctrl_data_width/8-1 downto 0);
    axi_ctrl_wready: out STD_LOGIC;
    axi_ctrl_wvalid: in STD_LOGIC;
    
    axi_ctrl_bresp: out STD_LOGIC_VECTOR(1 downto 0);
    axi_ctrl_bready: in STD_LOGIC;
    axi_ctrl_bvalid: out STD_LOGIC;
    
    axis_input_tready: out STD_LOGIC;
    axis_input_tvalid: in STD_LOGIC;
    axis_input_tdata: in STD_LOGIC_VECTOR(stream_data_width-1 downto 0);
    
    axis_output_tready: in STD_LOGIC;
    axis_output_tvalid: out STD_LOGIC;
    axis_output_tdata: out STD_LOGIC_VECTOR(stream_data_width-1 downto 0)
);
end component AXIS_AES_CTR_INTERFACE;

    constant ctrl_data_width: natural := 32;
    constant stream_data_width: natural := 16;
    
    constant ctrl_base_addr : UNSIGNED(31 downto 0) := x"40000000";

    constant key_size: natural := 256;

    signal aclk: STD_LOGIC;
    signal aresetn: STD_LOGIC;
 
    signal axi_ctrl_araddr: STD_LOGIC_VECTOR(31 downto 0);
    signal axi_ctrl_arready: STD_LOGIC;
    signal axi_ctrl_arvalid: STD_LOGIC;
 
    signal axi_ctrl_rdata: STD_LOGIC_VECTOR(ctrl_data_width-1 downto 0);
    signal axi_ctrl_rresp: STD_LOGIC_VECTOR(1 downto 0);
    signal axi_ctrl_rready: STD_LOGIC;
    signal axi_ctrl_rvalid: STD_LOGIC;
 
    signal axi_ctrl_awaddr: STD_LOGIC_VECTOR(31 downto 0);
    signal axi_ctrl_awready: STD_LOGIC;
    signal axi_ctrl_awvalid: STD_LOGIC;
 
    signal axi_ctrl_wdata: STD_LOGIC_VECTOR(ctrl_data_width-1 downto 0);
    signal axi_ctrl_wstrb: STD_LOGIC_VECTOR(ctrl_data_width/8-1 downto 0);
    signal axi_ctrl_wready: STD_LOGIC;
    signal axi_ctrl_wvalid: STD_LOGIC;
 
    signal axi_ctrl_bresp: STD_LOGIC_VECTOR(1 downto 0);
    signal axi_ctrl_bready: STD_LOGIC;
    signal axi_ctrl_bvalid: STD_LOGIC;
 
    signal axis_input_tready: STD_LOGIC;
    signal axis_input_tvalid: STD_LOGIC;
    signal axis_input_tdata: STD_LOGIC_VECTOR(stream_data_width-1 downto 0);
 
    signal axis_output_tready: STD_LOGIC;
    signal axis_output_tvalid: STD_LOGIC;
    signal axis_output_tdata: STD_LOGIC_VECTOR(stream_data_width-1 downto 0);

begin

DUT: component AXIS_AES_CTR_INTERFACE generic map (
    ctrl_data_width => ctrl_data_width,
    stream_data_width => stream_data_width,

    C_AXI_CTRL_BASEADDR => ctrl_base_addr,

    C_KEY_SIZE => key_size
)
port map (
    aclk => aclk,
    aresetn => aresetn,

    axi_ctrl_araddr => axi_ctrl_araddr,
    axi_ctrl_arready => axi_ctrl_arready,
    axi_ctrl_arvalid => axi_ctrl_arvalid,
    
    axi_ctrl_rdata => axi_ctrl_rdata,
    axi_ctrl_rresp => axi_ctrl_rresp,
    axi_ctrl_rready => axi_ctrl_rready,
    axi_ctrl_rvalid => axi_ctrl_rvalid,

    axi_ctrl_awaddr => axi_ctrl_awaddr,
    axi_ctrl_awready => axi_ctrl_awready,
    axi_ctrl_awvalid => axi_ctrl_awvalid,
    
    axi_ctrl_wdata => axi_ctrl_wdata,
    axi_ctrl_wstrb => axi_ctrl_wstrb,
    axi_ctrl_wready => axi_ctrl_wready,
    axi_ctrl_wvalid => axi_ctrl_wvalid,
    
    axi_ctrl_bresp => axi_ctrl_bresp,
    axi_ctrl_bready => axi_ctrl_bready,
    axi_ctrl_bvalid => axi_ctrl_bvalid,
    
    axis_input_tready => axis_input_tready,
    axis_input_tvalid => axis_input_tvalid,
    axis_input_tdata => axis_input_tdata,
    
    axis_output_tready => axis_output_tready,
    axis_output_tvalid => axis_output_tvalid,
    axis_output_tdata => axis_output_tdata
);

clk_reset_generation: block is
begin

clk_gen: process is
begin
aclk <= '0';
wait for 5 ns;
aclk <= '1';
wait for 5 ns;
end process;

rst_gen: process is
begin
aresetn <= '0';
wait for 100 ns;
aresetn <= '1';
wait;
end process;

end block clk_reset_generation;

axi_ctrl: block is
begin
axi_ctrl_write: block is
begin

write_key_ctr: process is
begin
axi_ctrl_awvalid <= '0';
axi_ctrl_wvalid <= '0';
axi_ctrl_wstrb <= x"f";
wait until aresetn = '1';
for i in 0 to 7 loop
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000000"+to_unsigned(4*i,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"0F1E2D3C";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
end loop;

for i in 0 to 3 loop
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000020"+to_unsigned(4*i,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"12345678";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
end loop;

-- First one should fail
for i in 0 to 1 loop
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000030"+to_unsigned(4*i,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"00000101";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
end loop;

wait for 4000 ns;
-- Overwrite part of key
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000000"+to_unsigned(0,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"BABABABA";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
-- And reinit
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000030"+to_unsigned(4,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"00000101";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
    
wait for 1000 ns;
    -- Overwrite part of counter
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000020"+to_unsigned(3,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"ABABABAB";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
    -- And reinit
    wait until rising_edge(aclk);
    axi_ctrl_awaddr <= std_logic_vector(x"40000030"+to_unsigned(4,32));
    axi_ctrl_awvalid <= '1';
    if axi_ctrl_awready /= '1' then
        wait until axi_ctrl_awready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_awvalid <= '0';
    
    wait until rising_edge(aclk);
    axi_ctrl_wdata <= x"00000101";
    axi_ctrl_wvalid <= '1';
    if axi_ctrl_wready /= '1' then
        wait until axi_ctrl_wready = '1';
    end if;
    wait until rising_edge(aclk);
    axi_ctrl_wvalid <= '0';
wait;
end process;

b_write: process is
begin
wait until axi_ctrl_bvalid = '1';
axi_ctrl_bready <= '1';
wait until rising_edge(aclk);
axi_ctrl_bready <= '0';
end process;

end block axi_ctrl_write;

axi_ctrl_read: block is
begin

status_get: process is
begin
axi_ctrl_araddr <= x"40000030";
axi_ctrl_arvalid <= '1';
if axi_ctrl_arready /= '1' then
    wait until axi_ctrl_arready = '1';
end if;
wait until rising_edge(aclk);
axi_ctrl_arvalid <= '0';

wait for 20 ns;

axi_ctrl_rready <= '0';
wait until rising_edge(aclk);
axi_ctrl_rready <= '1';
if axi_ctrl_rvalid /= '1' then
    wait until axi_ctrl_rvalid = '1';
end if;
wait until rising_edge(aclk);
axi_ctrl_rready <= '0';
wait for 100 ns;
end process;

end block axi_ctrl_read;
end block axi_ctrl;

axi_stream_test: block is
begin
write_input_data: process is
    variable actual_input_data: UNSIGNED(stream_data_width-1 downto 0) := (others => '0');
begin
axis_input_tvalid <= '1';
axis_input_tdata <= std_logic_vector(actual_input_data);
wait until axis_input_tready = '1';
wait until rising_edge(aclk);
axis_input_tvalid <= '0';
--actual_input_data := actual_input_data + x"20000000";
wait until rising_edge(aclk);
end process;

read_output_process: process is
begin
axis_output_tready <= '0';
wait for 2500 ns;
axis_output_tready <= '1';
wait;
end process;
end block;

end Behavioral;
