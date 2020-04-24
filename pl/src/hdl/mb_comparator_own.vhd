----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 02/10/2020 05:49:32 PM
-- Design Name: 
-- Module Name: mb_comparator_own - Behavioral
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

entity mb_comparator_own is
    Port ( lockstep_m : in STD_LOGIC_VECTOR (0 to 4095);
           lockstep_s : in STD_LOGIC_VECTOR (0 to 4095);
           clk: in STD_LOGIC;
           reset_signal: in STD_LOGIC;
           mb_error : out STD_LOGIC;
           mb_error_core : out STD_LOGIC;
           mb_error_axi_ip : out STD_LOGIC;
           mb_error_axi_dp : out STD_LOGIC;
           mb_error_axis : out STD_LOGIC;
           mb_error_axi_ic : out STD_LOGIC;
           mb_error_axi_dc : out STD_LOGIC;
           mb_error_trace : out STD_LOGIC;
           mb_error_reserved : out STD_LOGIC);
end mb_comparator_own;

architecture Behavioral of mb_comparator_own is
    signal error_state: STD_LOGIC;

    signal mb_error_internal : STD_LOGIC := '0';
    signal mb_error_core_internal : STD_LOGIC := '0';
    signal mb_error_axi_ip_internal : STD_LOGIC := '0';
    signal mb_error_axi_dp_internal : STD_LOGIC := '0';
    signal mb_error_axis_internal : STD_LOGIC := '0';
    signal mb_error_axi_ic_internal : STD_LOGIC := '0';
    signal mb_error_axi_dc_internal : STD_LOGIC := '0';
    signal mb_error_trace_internal : STD_LOGIC := '0';
    signal mb_error_reserved_internal : STD_LOGIC := '0';
begin
    error_state_proc: process(clk) is
    begin
        if rising_edge(clk) then
            if error_state = '0' then
                if mb_error_internal = '1' then
                    error_state <= '1';
                end if;
            else
                if reset_signal = '1' then
                    error_state <= '0';
                end if;
            end if;
        end if;
    end process;

    error_compare: process(clk) is
    begin
        if rising_edge(clk) then
            if lockstep_m (0 to 4095) /= lockstep_s(0 to 4095) then
                mb_error_internal <= '1';
            elsif error_state = '0' then
                mb_error_internal <= '0';
            end if;
            
            if lockstep_m (0 to 202) /= lockstep_s(0 to 202) then
                mb_error_core_internal <= '1';
            elsif error_state = '0' then
                mb_error_core_internal <= '0';
            end if;
                        
            if lockstep_m (207 to 464) /= lockstep_s(207 to 464) then
                mb_error_axi_ip_internal <= '1';
            elsif error_state = '0' then
                mb_error_axi_ip_internal <= '0';
            end if;
            
            if lockstep_m (465 to 722) /= lockstep_s(465 to 722) then
                mb_error_axi_dp_internal <= '1';
            elsif error_state = '0' then
                mb_error_axi_dp_internal <= '0';
            end if;
            
            if lockstep_m (723 to 1282) /= lockstep_s(723 to 1282) then
                mb_error_axis_internal <= '1';
            elsif error_state = '0' then
                mb_error_axis_internal <= '0';
            end if;
            
            if lockstep_m (1283 to 2081) /= lockstep_s(1283 to 2081) then
                mb_error_axi_ic_internal <= '1';
            elsif error_state = '0' then
                mb_error_axi_ic_internal <= '0';
            end if;
            
            if lockstep_m (2082 to 2880) /= lockstep_s(2082 to 2880) then
                mb_error_axi_dc_internal <= '1';
            elsif error_state = '0' then
                mb_error_axi_dc_internal <= '0';
            end if;
            
            if lockstep_m (2881 to 3229) /= lockstep_s(2881 to 3229) then
                mb_error_trace_internal <= '1';
            elsif error_state = '0' then
                mb_error_trace_internal <= '0';
            end if;
            
            if lockstep_m (3230 to 4095) /= lockstep_s(3230 to 4095) then
                mb_error_reserved_internal <= '1';
            elsif error_state = '0' then
                mb_error_reserved_internal <= '0';
            end if;
        end if;
    end process;
    
    mb_error <= mb_error_internal;
    mb_error_core <= mb_error_core_internal;
    mb_error_axi_ip <= mb_error_axi_ip_internal;
    mb_error_axi_dp <= mb_error_axi_dp_internal;
    mb_error_axis <= mb_error_axis_internal;
    mb_error_axi_ic <= mb_error_axi_ic_internal;
    mb_error_axi_dc <= mb_error_axi_dc_internal;
    mb_error_trace <= mb_error_trace_internal;
    mb_error_reserved <= mb_error_reserved_internal;
end Behavioral;
