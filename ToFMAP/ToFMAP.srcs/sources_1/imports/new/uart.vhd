library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
USE IEEE.NUMERIC_STD.ALL;

entity UART is
    Generic (
        -- Baud rate 160000 -> 100 MHz 100000000 / 1600 => 625
        BAUD_CYCLES : integer := 625
    );
    Port (
        clk         : in std_logic;
        data_in     : in std_logic_vector(7 downto 0);
        tx_send     : in std_logic;
        tx_reset    : in std_logic;
        rx          : in std_logic;
                
        tx          : out std_logic;
        data_recv   : out std_logic_vector(7 downto 0);
        
        tx_event    : out std_logic;
        rx_event    : out std_logic
    );
end UART;

architecture RTL of UART is
    signal div_clk          : std_logic := '0';   
    signal tx_buffer_data   : std_logic_vector(9 downto 0) := (others=>'0');
    signal rx_buffer_data : std_logic_vector(7 downto 0) := (others=>'0');
    signal rx_out_data : std_logic_vector(7 downto 0) := (others=>'0');
    
begin
    tx_buffer_data <= '1' & data_in & '0';
    data_recv <= rx_out_data;
   
    baud_clk_divider        : process(clk)
    variable baud_current : integer range 0 to BAUD_CYCLES := 0;
    begin
        if rising_edge(clk) then
            if baud_current < (BAUD_CYCLES-1) then
                baud_current := baud_current + 1;
                div_clk <= '0'; 
            else
                div_clk <= '1';
                baud_current := 0;
            end if;
        end if;
    end process;

    tx_process : process(div_clk, tx_reset)
    variable tx_current_bit : integer range 0 to 9 := 0;
    begin
        tx_event <= '0';
        if tx_reset = '1' then                    -- [TXRESET] --
            tx <= '1';
            tx_current_bit  := 0;
        elsif rising_edge(div_clk) then           -- [TXIDLE] --
            if tx_send = '0' then
                tx <= '1';
            else                                  -- [TXSEND] --
                tx <= tx_buffer_data(tx_current_bit);
                if tx_current_bit < 9 then
                    tx_current_bit := tx_current_bit + 1;
                else                              -- [TXSTOP] --
                    tx_event <= '1';
                    tx_current_bit := 0;
                end if;
            end if;
        end if;
    end process;
    
    rx_process: process(div_clk)
    variable rx_current_bit : integer range 0 to 9 := 0;
    begin
        if rising_edge(div_clk) then
            rx_event <= '0';
            if rx_current_bit = 9 then             -- [RXSTOP] --
                rx_current_bit := 0;
                if rx = '1' then
                    rx_event <= '1';
                    rx_out_data <= rx_buffer_data;
                end if;
            elsif rx_current_bit > 0 then          -- [RXRECV] --
                rx_buffer_data(rx_current_bit-1) <= rx;
                rx_current_bit := rx_current_bit + 1;
            elsif rx = '0' then                    -- [RXSTART] --
                rx_current_bit := 1;
            end if;                                -- [RXIDLE] -- ( rx = '1' )
        end if;
    end process;
        
end RTL;