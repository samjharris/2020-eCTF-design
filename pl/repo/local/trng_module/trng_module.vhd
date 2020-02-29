----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 02/25/2020 04:06:05 AM
-- Design Name: 
-- Module Name: trng_module - Behavioral
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

entity trng_module is
    Port ( clk : in STD_LOGIC;
           randout : out STD_LOGIC_VECTOR (31 downto 0)
           );
end trng_module;

architecture Behavioral of trng_module is
    signal sr: STD_LOGIC_VECTOR(31 downto 0);
    signal random_out: STD_LOGIC_VECTOR(7 downto 0);
    signal sr_push: STD_LOGIC;
    
    component whirlygig is
        Port (
            clk : in STD_LOGIC;
            pSerialOut : out std_logic_vector(7 downto 0);
            dataready : out STD_LOGIC
        );
    end component whirlygig;
begin
    whirlygig_rng: whirlygig
    port map (
        clk => clk,
        pSerialOut => random_out,
        dataready => sr_push
    );
    process(clk) is
    begin
        if rising_edge(clk) then
            if sr_push = '1' then
                sr(31 downto 8) <= sr(23 downto 0);
                sr(7 downto 0) <= random_out;
            end if;
        end if;
    end process;
    randout <= sr;
end Behavioral;
