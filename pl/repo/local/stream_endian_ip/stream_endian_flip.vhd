----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 04/02/2020 05:51:19 AM
-- Design Name: 
-- Module Name: stream_endian_flip - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity stream_endian_flip is
    Generic ( data_width : integer := 32 );
    Port ( axis_aclk: in STD_LOGIC;
           axis_aresetn: in STD_LOGIC;
           axis_input_tvalid : in STD_LOGIC;
           axis_input_tready : out STD_LOGIC;
           axis_input_tdata : in STD_LOGIC_VECTOR (data_width-1 downto 0);
           axis_input_tdest : in STD_LOGIC_VECTOR (7 downto 0);
           axis_input_tlast : in STD_LOGIC;
           axis_output_tvalid : out STD_LOGIC;
           axis_output_tready : in STD_LOGIC;
           axis_output_tdata : out STD_LOGIC_VECTOR (data_width-1 downto 0);
           axis_output_tdest : out STD_LOGIC_VECTOR (7 downto 0);
           axis_output_tlast : out STD_LOGIC);
end stream_endian_flip;

architecture Behavioral of stream_endian_flip is
begin
    assert (data_width = 32 or data_width = 16) report "Data width must be 16 or 32" severity failure;
    axis_output_tvalid <= axis_input_tvalid;
    axis_input_tready <= axis_output_tready;
    axis_output_tdest<= axis_input_tdest;
    axis_output_tlast <= axis_input_tlast;
    
    width_larger_swap: if (data_width = 32) generate
    axis_output_tdata(31 downto 24) <= axis_input_tdata (23 downto 16);
    axis_output_tdata(23 downto 16) <= axis_input_tdata (31 downto 24);
    end generate;
    axis_output_tdata(15 downto 8) <= axis_input_tdata (7 downto 0);
    axis_output_tdata(7 downto 0) <= axis_input_tdata (15 downto 8);
end Behavioral;
