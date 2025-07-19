library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.std_logic_unsigned.all;

entity PWM is
    Generic (
        -- -> 10 ^ 8 / 50 Hz / 1000 (highest duty value) / 2 (1' 0 1' 0)
        period : integer := 1000 
    );
    Port (
        ena     : in std_logic;
        clk     : in std_logic;
        reset   : in std_logic;
        duty    : in std_logic_vector(9 downto 0);
        pwm_out : out std_logic
    );  
end PWM;

architecture RTL of PWM is
    signal PWM_clk : std_logic := '0';
begin
    PWM_clk_divider : process(clk, reset)
    variable current_clk : integer := 0; 
    begin
        if reset = '1' then
            PWM_clk <= '0';
            current_clk := 0;
        elsif rising_edge(clk) then
            if current_clk < period then
                PWM_clk <= '0';
                current_clk := current_clk + 1;
            else
                current_clk := 0;
                PWM_clk <= '1';
            end if;
        end if;
    end process;
    
    
    PWM_process : process(PWM_clk, ena, reset)
    variable iter : integer := 0;
    begin
        if reset = '1' or ena = '0' then
            iter := 0;
            pwm_out <= '0';
        elsif rising_edge(PWM_clk) then
            if iter < to_integer(unsigned(duty)) then
                pwm_out <= '1';
                iter := iter + 1;
            elsif iter = period then
                iter := 0;
            else
                iter := iter + 1;
                pwm_out <= '0'; 
            end if;
        end if;
    end process;
    --clk <= not clk after 10ns;
end RTL;