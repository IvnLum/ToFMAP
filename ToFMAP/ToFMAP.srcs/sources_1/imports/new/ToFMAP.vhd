library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.std_logic_unsigned.all;
use IEEE.std_logic_arith.all;


entity ToFMAP is
    Port (
        HEXDEC  : in STD_LOGIC;
        CLK     : in  STD_LOGIC;
        LED 	: out  STD_LOGIC_VECTOR (15 downto 0);
        RST     : in std_logic;
        RX      : in  STD_LOGIC;
        
        SDA     : inout std_logic;
        SCL     : inout std_logic;
        IRQ     : inout std_logic;
        SS      : out std_logic;
        
        disp_out: out STD_LOGIC_VECTOR(6 downto 0);
		disp_sel: out STD_LOGIC_VECTOR(3 downto 0);
		
        TX      : out  STD_LOGIC;
        PWM_o   : out STD_LOGIC;
        SOUT	: out  STD_LOGIC_VECTOR(3 downto 0);
        
        MOTOR_CTRL : out STD_LOGIC_VECTOR(3 downto 0);
        BUZZER   : out STD_LOGIC
    );
end ToFMAP;

architecture Structural of ToFMAP is


    
       --####################################################################--
      --######################################################################--
     --##############################             #############################--
    --##############################   COMPONENTS  #############################--
     --##############################             #############################--
      --######################################################################--
       --####################################################################--
       
    ----------------------------------------------------------------------------------
    ------------------------------ Display comp --------------------------------------
    ----------------------------------------------------------------------------------
    component DISPLAY is
    Port (
        clk : in STD_LOGIC;
        rst : in STD_LOGIC;
        D   : in std_logic_vector(3 downto 0);
        C   : in std_logic_vector(3 downto 0);
        B   : in std_logic_vector(3 downto 0);
        A   : in std_logic_vector(3 downto 0);
        disp_sel : out STD_LOGIC_VECTOR (3 downto 0);
        disp_out : out STD_LOGIC_VECTOR (6 downto 0)
        );
    end component;
    
    ----------------------------------------------------------------------------------
    --------------------------------- UART comp --------------------------------------
    ----------------------------------------------------------------------------------
    component UART is
    Generic (
        -- Baud rate 160 kBaud -> 100 MHz 100000000 / 160000 => 625
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
    end component;
    
    ----------------------------------------------------------------------------------
    ------------------------------- PWM (Servo) comp ---------------------------------
    ----------------------------------------------------------------------------------
    component PWM is
    Generic (
        -- Default for 10 ^ 8 / 50 Hz / 1000 (highest duty value)
        period : integer := 2000
    );
    Port (
        ena     : in std_logic;
        clk     : in std_logic;
        reset   : in std_logic;
        duty    : in std_logic_vector(9 downto 0);
        pwm_out : out std_logic
    );
    end component;
    
    ----------------------------------------------------------------------------------
    ------------------------------ I2C (pmodToF) comp --------------------------------
    ----------------------------------------------------------------------------------
    component pmodToF_i2c is
        Port (
            CLK     : in  STD_LOGIC;
            sda     : inout  STD_LOGIC;
            scl     : inout  STD_LOGIC;
            data    : out std_logic_vector(15 downto 0);
            IRQ     : inout std_logic;
            SS      : out std_logic;
            rx_event : out std_logic
        );
    end component;
    
    ----------------------------------------------------------------------------------
    --------------------------------- SINCOS LUT -------------------------------------
    ----------------------------------------------------------------------------------
    
    component sincos_async_lut is
        Port (
            reset           : in  std_logic;
            clk             : in  std_logic;
            clk_en          : in  std_logic;
            alpha           : in  std_logic_vector(8 downto 0);
            sin_data        : out std_logic_vector(31 downto 0);
            cos_data        : out std_logic_vector(31 downto 0)
        );
    end component;
    
    
    
       --####################################################################--
      --######################################################################--
     --##############################             #############################--
    --##############################    SIGNALS    #############################--
     --##############################             #############################--
      --######################################################################--
       --####################################################################--
    
    
    
    ----------------------------------------------------------------------------------
    ----------------------------- Display test signals -------------------------------
    ----------------------------------------------------------------------------------
    signal counter : std_logic_vector(15 downto 0) := (others => '0');
    signal decimal_v : std_logic_vector(15 downto 0) := (others => '0');
	signal natur : std_logic_vector(15 downto 0) := (others => '0');
	signal out_v : std_logic_vector(15 downto 0) := (others => '0');
	
    ----------------------------------------------------------------------------------
    ------------------------------ UART test signals ---------------------------------
    ----------------------------------------------------------------------------------
    signal loc_x    : std_logic_vector(31 downto 0) := (others => '0'); 
    signal loc_y    : std_logic_vector(31 downto 0) := (others => '0');
    signal dir_x    : std_logic_vector(31 downto 0) := (others => '0');
    signal dir_y    : std_logic_vector(31 downto 0) := (others => '0'); 
    signal inc      : std_logic_vector(31 downto 0) := (others => '0');
    signal azm      : std_logic_vector(31 downto 0) := (others => '0');
    signal dist     : std_logic_vector(31 downto 0) := (others => '0');
    signal batt     : std_logic_vector(7 downto 0)  := x"04";
    
    type byte_arr is array (0 to 33) of std_logic_vector(7 downto 0);
    
    constant byte_packet  : byte_arr :=
        (
        "01010101", -- Head
        
        loc_x(7 downto 0), loc_x(15 downto 8), loc_x(23 downto 16), loc_x(31 downto 24),
        loc_y(7 downto 0), loc_y(15 downto 8), loc_y(23 downto 16), loc_y(31 downto 24),
        dir_x(7 downto 0), dir_x(15 downto 8), dir_x(23 downto 16), dir_x(31 downto 24),
        dir_y(7 downto 0), dir_y(15 downto 8), dir_y(23 downto 16), dir_y(31 downto 24),
        inc(7 downto 0), inc(15 downto 8), inc(23 downto 16), inc(31 downto 24),
        azm(7 downto 0), azm(15 downto 8), azm(23 downto 16), azm(31 downto 24),
        dist(7 downto 0), dist(15 downto 8), dist(23 downto 16), dist(31 downto 24),
        batt(7 downto 0),
        
        x"00", x"00", x"00",
        
        "11111111" -- Tail
        );
    
    signal data_recv : std_logic_vector(7 downto 0);
    signal data_send : std_logic_vector(7 downto 0) := byte_packet(0);
    signal tx_event : std_logic := '0';
    signal rx_event : std_logic := '0';
    constant data_send_bytes : integer := 34;
    
    ----------------------------------------------------------------------------------
    ---------------------------- Servo test signals ----------------------------------
    ----------------------------------------------------------------------------------
    signal servo_clk : std_logic := '0';
    type duties_arr is array (0 to 1) of std_logic_vector(9 downto 0);
        
    constant duties : duties_arr := 
        (
        -- 0 / 180 = 0 => 50 + 0 * 50 = 50 / 1000 duty -> 50
        -- 180 / 180 = 1 => 50 + 1 * 50 = 100 / 1000 duty -> 100
         "0000110010",
         "0001100100"
        );
    constant duties_values : integer := 2;
    signal sel_duty : std_logic_vector(9 downto 0) := "0000110010";
    signal cur_duty : std_logic := '0';
    signal servo_upd: std_logic := '0';
    signal po : std_logic := '0';
    signal enable_div_sweep_time : std_logic := '1';
    
    ----------------------------------------------------------------------------------
    ----------------------------- Stepper test signals -------------------------------
    ----------------------------------------------------------------------------------
    type nibble_arr_stepper is array (0 to 7) of std_logic_vector(3 downto 0);
    constant nibble_stepper  : nibble_arr_stepper :=
        (
            "1001",
            "1000",
            "1100",
            "0100",
            "0110",
            "0010",
            "0011",
            "0001"
        );
    constant stepper_steps : integer := 8;  
    signal clk_div_stepper : std_logic := '0';
    
    ----------------------------------------------------------------------------------
    --------------------------------- I2C test signals -------------------------------
    ----------------------------------------------------------------------------------
    
    signal rx_event_i2c : std_logic := '0';  
    
    ----------------------------------------------------------------------------------
    --------------------------- SINCOS LUT test signals ------------------------------
    ----------------------------------------------------------------------------------
    
    signal alpha : std_logic_vector(8 downto 0) := (others => '0'); 
    signal sin_data : std_logic_vector(31 downto 0) := (others => '0'); 
    signal cos_data : std_logic_vector(31 downto 0) := (others => '0');
    
    ----------------------------------------------------------------------------------
    --------------------------- MOTOR CTRL test signals ------------------------------
    ----------------------------------------------------------------------------------
    
        
    signal motor_nibble : std_logic_vector(3 downto 0) := (others => '0'); 
    signal motor_duty_i : std_logic_vector(9 downto 0) :=  "1101100110"; -- 50
    signal motor_duty_ii : std_logic_vector(9 downto 0) := "1111100111"; -- 50
    signal motor_pwm_i : std_logic := '0';
    signal motor_pwm_ii : std_logic := '0';
    signal right_turn_rotate_offset : std_logic := '0';
    signal left_turn_rotate_offset : std_logic := '0';
    constant stepper_default_sleep_period : integer := 2_083_333 / 2;
    signal stepper_current_sleep_period : integer := 0;
    
begin
    
    
    
       --####################################################################--
      --######################################################################--
     --##############################             #############################--
    --##############################  CONNECTIONS  #############################--
     --##############################             #############################--
      --######################################################################--
       --####################################################################--
    
    
    LED <= RST & cur_duty & po & rx_event_i2c & "ZZZZZZ" & data_recv(5 downto 0);
    dist(15 downto 0) <= decimal_v(15 downto 0);
    PWM_o <= po;
    MOTOR_CTRL <= motor_nibble;
    ----------------------------------------------------------------------------------
    ---------------------------------- DISPLAY conn ----------------------------------
    ----------------------------------------------------------------------------------
    DISPLAY_conn: DISPLAY PORT MAP (
	       clk => clk,
	       rst => rst,
	       D => out_v(15 downto 12),
	       C => out_v(11 downto 8),
	       B => out_v(7 downto 4),
	       A => out_v(3 downto 0),
	       disp_sel => disp_sel,
	       disp_out => disp_out
	);
	
    ----------------------------------------------------------------------------------
    ------------------------------------ UART conn -----------------------------------
    ----------------------------------------------------------------------------------
    UART_conn:
    UART port map (
        clk => CLK,
        data_in => data_send,
        tx_send => '1',
        tx_reset => RST,
        rx => RX,
        tx => TX,
        data_recv => data_recv,
        tx_event => tx_event,
        rx_event => rx_event
    );
    
    ----------------------------------------------------------------------------------
    ------------------------------- PWM (Servo) conn ---------------------------------
    ----------------------------------------------------------------------------------
    PWM_conn:
    PWM generic map (
        period => 2000
    ) port map (
        ena => enable_div_sweep_time,
        clk => CLK,
        reset => servo_clk,
        duty => sel_duty,
        pwm_out => po
    );
    
    PWM_conn_motor_i:
    PWM generic map (
        period => 1000
    ) port map (
        ena => '1',
        clk => CLK,
        reset => '0',
        duty => motor_duty_i,
        pwm_out => motor_pwm_i
    );
    
    PWM_conn_motor_ii:
    PWM generic map (
        period => 1000
    ) port map (
        ena => '1',
        clk => CLK,
        reset => '0',
        duty => motor_duty_ii,
        pwm_out => motor_pwm_ii
    );
    
    
    ----------------------------------------------------------------------------------
    ------------------------------- I2C (pmodToF) conn -------------------------------
    ----------------------------------------------------------------------------------
    pmodToF_i2c_conn:
    pmodToF_i2c port map (
        CLK => CLK,
        sda => sda,
        scl => scl,
        data => counter,
        IRQ  => IRQ,
        SS   => SS,
        rx_event   => rx_event_i2c
    );
    
    ----------------------------------------------------------------------------------
    --------------------------------- SINCOS LUT conn --------------------------------
    ----------------------------------------------------------------------------------
    sincos_lut_conn:
    sincos_async_lut port map (
        reset => '0',
        clk   => CLK,
        clk_en=> '1',
        alpha => alpha,
        sin_data => sin_data,
        cos_data => cos_data
    );    

	
	decimal_v <= std_logic_vector( to_unsigned( (conv_integer(counter) * 33310) / 65536, 16));
	natur(3 downto 0) <= std_logic_vector( to_unsigned(conv_integer(decimal_v) mod 10, 4) );
	natur(7 downto 4) <= std_logic_vector( to_unsigned((conv_integer(decimal_v) / 10) mod 10, 4) );
	natur(11 downto 8) <= std_logic_vector( to_unsigned((conv_integer(decimal_v) / 100) mod 10, 4) );
	natur(15 downto 12) <= std_logic_vector( to_unsigned((conv_integer(decimal_v) / 1000) mod 10, 4) );
 
	out_v <= natur when HEXDEC = '1' else counter;
	
	
	
	
       --####################################################################--
      --######################################################################--
     --##############################             #############################--
    --##############################   PROCESSES   #############################--
     --##############################             #############################--
      --######################################################################--
       --####################################################################--

	----------------------------------------------------------------------------
    -------------------------- UART control process ----------------------------
    ----------------------------------------------------------------------------
    UART_tx_next_byte:
    process(tx_event, RST)
        variable data_send_current_byte : integer := 0;
    begin
        if RST = '1' then
            data_send <= byte_packet(0);
            data_send_current_byte := 0;
        elsif rising_edge(tx_event) then
            data_send <= byte_packet(data_send_current_byte);
            if data_send_current_byte < (data_send_bytes - 1) then
                data_send_current_byte := data_send_current_byte + 1;
            else
                data_send_current_byte := 0;
            end if;
        end if;
    end process;
    
    ----------------------------------------------------------------------------------
    ----------------------- PWM (Servo) control processes ----------------------------
    ----------------------------------------------------------------------------------
    Servo_clk_div:
    process(CLK)
        variable servo_current_clk : integer := 0;
        
        variable enable_sweep_current_period : integer := 0;
        constant target_sweep_total_period : integer := 2_000_000;
        variable target_sweep_current_sleep : integer := 0;
        constant target_sweep_total_sleep : integer := 1;
        
        variable servo_current_udegrees : integer := 45000000;
        constant delta_udegrees : integer := 1374;
        constant upd_await_cycles : integer := 1000;
        variable current_upd_await_cycles : integer := 0;
    begin
        inc <= std_logic_vector( to_signed(servo_current_udegrees, 32) );
        if rising_edge(CLK) then
            
            if current_upd_await_cycles < upd_await_cycles then
                current_upd_await_cycles := current_upd_await_cycles + 1;
            else
                if cur_duty = '1' and (servo_current_udegrees < 135000000) then
                    servo_current_udegrees := servo_current_udegrees + delta_udegrees;
                elsif cur_duty = '0' and (servo_current_udegrees > 45000000) then
                    servo_current_udegrees := servo_current_udegrees - delta_udegrees;
                end if;
                current_upd_await_cycles := 0;
            end if;
            
            if enable_sweep_current_period < target_sweep_total_period then
                enable_sweep_current_period := enable_sweep_current_period + 1;
            else
                if target_sweep_current_sleep < target_sweep_total_sleep then
                    target_sweep_current_sleep := target_sweep_current_sleep + 1;
                    enable_div_sweep_time <= '0';
                else
                    enable_div_sweep_time <= '1';
                    target_sweep_current_sleep := 0;
                end if;
                enable_sweep_current_period := 0;
            end if;
                    
            if servo_current_clk < 65_500_000 then
                servo_current_clk := servo_current_clk + 1;
                servo_clk <= '0';
            else
                servo_clk <= '1';
                servo_current_clk := 0;
            end if;
        end if;
    end process;
     
    Servo_control:
    process(servo_clk)
        variable current_duty : integer := 0;
    begin
        if rising_edge(servo_clk) then
            sel_duty <= duties(current_duty);
            if current_duty < (duties_values - 1) then
                current_duty := current_duty + 1;
                cur_duty <= '0'; 
            else
                cur_duty <= '1'; 
                current_duty := 0;
            end if;
        end if;
    end process;
    
    -------------------------------------------------------------------------------
    ------------------------- Stepper control processes ---------------------------
    -------------------------------------------------------------------------------
    stepper_pulse_clk_process : process(CLK)
        variable clk_d : integer := 0;
        
        constant manufacturer_udegress_per_step : integer := 7_500_000;
        constant max_udegrees : integer := 360_000_000;
        variable stepper_current_udegrees : integer := 0;
    begin
        if rising_edge(CLK) then
            if clk_d < stepper_current_sleep_period then
                clk_d := clk_d + 1;
                clk_div_stepper <= '0';
            else
                clk_d := 0;
                clk_div_stepper <= '1';
            end if;
        end if;
    end process;
    
    stepper_update_azimuth_process : process(CLK)
        constant clk_c : integer := 2_083_333 / 2;-- 60 rpm => ( 1 rev => (48/2) micropasos/seg (0,020833/2) seg/paso)
        variable clk_d : integer := 0;
        
        constant manufacturer_udegress_per_step : integer := 7_500_000;
        constant max_udegrees : integer := 360_000_000;
        variable stepper_current_udegrees : integer := 0;
    begin
        azm <= std_logic_vector( to_signed(stepper_current_udegrees, 32) );

        if rising_edge(CLK) then
            if clk_d < clk_c then
                clk_d := clk_d + 1;
            else
                clk_d := 0;
                
                if (stepper_current_udegrees + manufacturer_udegress_per_step/2) < max_udegrees then
                    stepper_current_udegrees := stepper_current_udegrees + manufacturer_udegress_per_step / 2;
                else
                    stepper_current_udegrees := (stepper_current_udegrees + manufacturer_udegress_per_step/2) - max_udegrees;
                end if;
            end if;
        end if;
    end process;
       
    stepper_upd_process : process(clk_div_stepper)
        variable cont : integer := 0;
    begin
        if rising_edge(clk_div_stepper) then
            SOUT <= nibble_stepper(cont);
            if cont < stepper_steps-1 then
                cont := cont + 1;
            else
                cont := 0;
            end if;
        end if;
    end process;
    
    ----------------------------------------------------------------------------------
    ------------------ Vehicle location/rotation control process  --------------------
    ----------------------------------------------------------------------------------
    vehicle_location_process : process(clk)
        constant clk_c : integer := 1_000_000;
        variable clk_d : integer := 0;
        
        variable loc_x_v : integer := 750_000;
        variable loc_y_v : integer := 750_000;
        variable azimuth : integer := 0;
        
        constant delta_mdegrees : integer := 378 * 10;
        constant delta_m_milimeters_div_100 : integer := 400 / 100; 
        constant max_mdegrees : integer := 360_000;
        
        variable loc_x_signed : std_logic_vector(31 downto 0) := (others => '0');
        variable loc_y_signed : std_logic_vector(31 downto 0) := (others => '0');
        variable azimuth_signed : std_logic_vector(31 downto 0) := (others => '0');
    begin
        loc_x <= loc_x_signed;
        loc_y <= loc_y_signed;
        dir_x <= azimuth_signed;
        alpha <= std_logic_vector(to_signed(azimuth / 1000, 9));
        
        if rising_edge(CLK) then
            if clk_d < clk_c then
                clk_d := clk_d + 1;
            else
                clk_d := 0;
                loc_x_signed := std_logic_vector( to_signed(loc_x_v / 10, 32) );
                loc_y_signed := std_logic_vector( to_signed(loc_y_v / 10, 32) );
                azimuth_signed := std_logic_vector( to_signed(azimuth, 32) );
                
                if data_recv(0) = '1' then
                    if data_recv(5) = '1' then
                            loc_y_v := loc_y_v - (delta_m_milimeters_div_100 * conv_integer(cos_data));
                            loc_x_v := loc_x_v + (delta_m_milimeters_div_100 * conv_integer(sin_data));
                    elsif data_recv(4) = '1' then
                            loc_y_v := loc_y_v + (delta_m_milimeters_div_100 * conv_integer(cos_data));
                            loc_x_v := loc_x_v - (delta_m_milimeters_div_100 * conv_integer(sin_data));
                    end if;
                else
                    if data_recv(3) = '1' then
                        if (azimuth - delta_mdegrees) >= 0 then
                            azimuth := azimuth - delta_mdegrees;
                        else
                            azimuth := max_mdegrees - (azimuth - delta_mdegrees);
                        end if;
            
                    elsif data_recv(2) = '1' then
                        if (azimuth + delta_mdegrees) < max_mdegrees then
                            azimuth := azimuth + delta_mdegrees;
                        else
                            azimuth := (azimuth + delta_mdegrees) - max_mdegrees;
                        end if;
                    end if;
                end if;
            end if;
        end if;
    end process; 
    
    vehicle_motor_right_ctrl : process(clk, motor_pwm_i)
    begin
        if rising_edge(clk) then
            if motor_pwm_i = '1' then
                if data_recv(0) = '1' then
                    if data_recv(5) = '1' then
                        motor_nibble(1 downto 0) <= "10";
                    elsif data_recv(4) = '1' then
                        motor_nibble(1 downto 0) <= "01";
                    else
                        motor_nibble(1 downto 0) <= (others => '0');
                    end if;
                else
                    if data_recv(3) = '1' then
                        motor_nibble(1 downto 0) <= "10";
                        stepper_current_sleep_period <= stepper_default_sleep_period / 2;
                    elsif data_recv(2) = '1' then
                        stepper_current_sleep_period <= stepper_default_sleep_period - 992_063;
                        motor_nibble(1 downto 0) <= "01";
                    else
                        stepper_current_sleep_period <= stepper_default_sleep_period;
                        motor_nibble(1 downto 0) <= (others => '0');
                    end if;
                end if;                   
            else
                stepper_current_sleep_period <= stepper_default_sleep_period;
                motor_nibble(1 downto 0) <= (others => '0');
            end if;
        end if;
    end process;
    
    vehicle_motor_left_ctrl : process(clk, motor_pwm_ii)
    begin
        if rising_edge(clk) then
            if motor_pwm_ii = '1' then
                if data_recv(0) = '1' then
                    if data_recv(5) = '1' then
                        motor_nibble(3 downto 2) <= "10";
                    elsif data_recv(4) = '1' then
                        motor_nibble(3 downto 2) <= "01";
                    else
                        motor_nibble(3 downto 2) <= (others => '0');
                    end if;
                else
                    if data_recv(2) = '1' then
                        motor_nibble(3 downto 2) <= "10";
                    elsif data_recv(3) = '1' then
                        motor_nibble(3 downto 2) <= "01";
                    else
                        motor_nibble(3 downto 2) <= (others => '0');
                    end if;
                end if;                   
            else
                motor_nibble(3 downto 2) <= (others => '0');
            end if;
        end if;
    end process;
    
    buzzer_process : process(CLK)
    -- 1kHz Buzzer
        constant clk_c : integer := 25_000 * 2;
        constant clk_c2 : integer := 25_000 * 4;
        variable clk_d : integer := 0;
        variable BUZ : std_logic := '0';
    begin
        if rising_edge(CLK) then
            if data_recv(1) = '1' then
                BUZZER  <= BUZ; 
            else
                BUZZER <= '0';
            end if;
            
            if clk_d < clk_c then
                clk_d := clk_d + 1;
                BUZ := '1';
            elsif clk_d < clk_c2 then
                clk_d := clk_d + 1;
                BUZ := '0';
            else 
                clk_d := 0;
            end if;
        end if;     
    end process;
    
end Structural;
