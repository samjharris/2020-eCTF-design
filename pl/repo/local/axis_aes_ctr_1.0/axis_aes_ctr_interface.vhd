library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

-- Width of addresses is assumed to be 32, change if necessary
entity AXIS_AES_CTR_INTERFACE is
generic (
    ctrl_data_width: natural := 32;
    stream_data_width: natural := 32;
    
    C_AXI_CTRL_BASEADDR : UNSIGNED(31 downto 0) := x"05000000";

    C_KEY_SIZE: natural := 256;
    
    C_W_ADDRALIGN_STRBFLIP: boolean := false
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
    axis_input_tlast: in STD_LOGIC;
    
    axis_output_tready: in STD_LOGIC;
    axis_output_tvalid: out STD_LOGIC;
    axis_output_tdata: out STD_LOGIC_VECTOR(stream_data_width-1 downto 0);
    axis_output_tdest: out STD_LOGIC_VECTOR(7 downto 0);
    axis_output_tlast: out STD_LOGIC
);

end AXIS_AES_CTR_INTERFACE;

architecture Behavioral of AXIS_AES_CTR_INTERFACE is
    -- location why
    constant ctr_size: natural := 128;
    constant ctr_incr_size: natural := 64;
    
    type BYTE_ARRAY is array (natural range <>) of STD_LOGIC_VECTOR(7 downto 0);

    constant axi_reg_size: natural := 32+16+4+4;
    -- Key and control is never readable so manually trim this out
    signal axi_register: BYTE_ARRAY(32 to axi_reg_size-1-4) := (others => x"00");
    signal axi_register_shadow_ext: BYTE_ARRAY(0 to axi_reg_size-1) := (others => x"00");
    
    type AXI_HANDSHAKE_TYPE is (IDLE, PROCESSING, DONE);
    --signal axi_ctrl_ar_state: AXI_HANDSHAKE_TYPE := IDLE;
    signal axi_ctrl_r_state: AXI_HANDSHAKE_TYPE := IDLE;
    --signal axi_ctrl_aw_state: AXI_HANDSHAKE_TYPE := IDLE;
    signal axi_ctrl_w_state: AXI_HANDSHAKE_TYPE := IDLE;
    --signal axi_ctrl_b_state: AXI_HANDSHAKE_TYPE := IDLE;
    signal axi_ctrl_b_avail: STD_LOGIC := '0';
    signal axi_ctrl_bvalid_internal: STD_LOGIC := '0';
    
    signal axi_ctrl_araddr_internal: UNSIGNED(7 downto 0);
    signal axi_ctrl_awaddr_internal: UNSIGNED(7 downto 0);
    signal axi_ctrl_awaddr_trimmed: STD_LOGIC_VECTOR(31 downto 0);
    signal axi_ctrl_wstrb_internal: STD_LOGIC_VECTOR(ctrl_data_width/8-1 downto 0);
    
    signal axi_ctrl_araddr_offset: INTEGER range 0 to ctrl_data_width/8-1;
    signal axi_ctrl_awaddr_offset: INTEGER range 0 to ctrl_data_width/8-1;
    
    -- Bit return is whether given address is readable
    pure function IsAddrReadable(addr: UNSIGNED(7 downto 0)) 
        return STD_LOGIC is
    begin
        if addr >= x"20" and addr <= x"30" then
            return '1';
        else
            return '0';
        end if;
    end IsAddrReadable;
    
    -- Vector return is strobe mask indicating which bytes are writable
    pure function IsAddrWritable(addr: UNSIGNED(7 downto 0))
        return STD_LOGIC_VECTOR is
        constant wmask: STD_LOGIC_VECTOR(0 to (16#ff#+ctrl_data_width/8-1)) := 
                (0 to 16#2f# => '1', 16#34# to 16#37# => '1', others => '0');
        variable return_value: STD_LOGIC_VECTOR(3 downto 0);
        variable index: integer;
    begin
        index := to_integer(addr);
        for i in 0 to ctrl_data_width/8-1 loop
            return_value(ctrl_data_width/8-1-i) := wmask(index+i);
        end loop;
        return return_value;
        -- Old code without strobing
        --if addr <= (x"30"-x"4") or addr = x"34" then
        --    return '1';
        --else
        --    return '0';
        --end if;
    end IsAddrWritable;

    signal axi_ctrl_araddr_readable: STD_LOGIC := '0';
    signal axi_ctrl_awaddr_writable: STD_LOGIC_VECTOR(ctrl_data_width/8-1 downto 0) := (others => '0');
    
    -- These bits are toggled from the appropriate places
    -- R/W transaction can happen when bit pairs are equal
    signal axi_araddr_track_ar: STD_LOGIC := '0';
    signal axi_araddr_track_r: STD_LOGIC := '1';
    signal axi_awaddr_track_aw: STD_LOGIC := '0';
    signal axi_awaddr_track_w: STD_LOGIC := '1';
    
    constant aes_core_encdec: STD_LOGIC := '1';
    signal aes_core_init: STD_LOGIC := '0';
    signal aes_core_read_next: STD_LOGIC := '0';
    signal aes_core_ready: STD_LOGIC;
    
    signal aes_core_key: STD_LOGIC_VECTOR(255 downto 0);
    signal aes_core_keylen: STD_LOGIC;
    
    signal aes_counter: STD_LOGIC_VECTOR(ctr_size-1 downto 0);
    signal aes_counter_new: STD_LOGIC_VECTOR(ctr_size-1 downto 0);
    signal aes_ctr_updated: STD_LOGIC := '0';

    signal aes_core_result: STD_LOGIC_VECTOR(127 downto 0);
    signal aes_core_result_valid: STD_LOGIC;
    signal aes_core_result_valid_ctr: natural := 0;
    
    type AES_INIT_CYCLE is (START, AWAIT_CORE_FREE, SEND_INIT_PULSE, AWAIT_CORE_READY, DONE);
    signal aes_key_init: AES_INIT_CYCLE := START;
    signal aes_key_init_prev: AES_INIT_CYCLE := START;
    signal aes_ctr_init: AES_INIT_CYCLE := START;
    
    signal aes_ctr_block_section: natural := (ctr_size-1)/stream_data_width;
    type AXI_STREAM_STATE_TYPE is (AWAIT_AES_CORE, IDLE, TX_RESULT, AWAIT_AES_START);
    signal axi_stream_state: AXI_STREAM_STATE_TYPE := AWAIT_AES_CORE;
    
    component aes_core is 
    port (
        clk: in STD_LOGIC;
        reset_n: in STD_LOGIC;

        encdec: in STD_LOGIC;
        init: in STD_LOGIC;
        read_next: in STD_LOGIC;
        ready: out STD_LOGIC;
    
        key: in STD_LOGIC_VECTOR (255 downto 0);
        keylen: in STD_LOGIC;
        
        block_data: in STD_LOGIC_VECTOR (127 downto 0);
        result: out STD_LOGIC_VECTOR (127 downto 0);
        result_valid: out STD_LOGIC
    );
    end component aes_core;
begin
    aes_core_inst: component aes_core port map(
        clk => aclk,
        reset_n => aresetn,
        
        encdec => aes_core_encdec,
        init => aes_core_init,
        read_next => aes_core_read_next,
        ready => aes_core_ready,
        
        key => aes_core_key,
        keylen => aes_core_keylen,
        
        block_data => aes_counter,
        result => aes_core_result,
        result_valid => aes_core_result_valid
    );

    aes_core_input_mappings: block is
    begin
        assert (C_KEY_SIZE = 128 or C_KEY_SIZE = 256) report "Only 128-bit or 256-bit keys are supported" severity failure;
        aes_core_keylen <= '1' when C_KEY_SIZE = 256 else '0';

        -- Could write all of these as "generate"s but sometimes nested for is more legible
        axi_to_aes_key: process(axi_register_shadow_ext) is
        begin
            for i in 0 to (C_KEY_SIZE-1)/8 loop
                for j in 0 to 7 loop
                    aes_core_key((C_KEY_SIZE-8*i)-(8-j)) <= axi_register_shadow_ext (i) (j);
                end loop;
            end loop;
        end process;
        
        -- This one is a generate because nested for loops confused the simulator
        aes_ctr_to_axi: for i in 0 to (ctr_size-1) generate
        begin
            axi_register(16#20#+(i/8)) (8-1-(i rem 8)) <= aes_counter(ctr_size-1-i);
        end generate;
        --aes_ctr_to_axi: process(aes_counter) is
        --begin
        --    for i in 0 to 127/8 loop
        --        for j in 0 to 7 loop
        --            axi_register(i+16#20#)(j) <= aes_counter((128-8*i)-(8-j));
        --        end loop;
        --    end loop;
        --end process;
    end block aes_core_input_mappings;

    aes_core_init_fsm: block is
    begin
        aes_init_key_fsm: process(aclk) is
            variable ctr_ctrl_reg_updated: STD_LOGIC := '0';
        begin
            if rising_edge(aclk) then
                if aresetn = '0' then
                    aes_key_init <= START;
                    aes_key_init_prev <= START;
                else
                    -- Is the init_key bit actually written?
                    ctr_ctrl_reg_updated := '0';
                    for i in 0 to 3 loop
                        if axi_ctrl_awaddr_internal=(x"34"+i) and axi_ctrl_wstrb_internal(i) = '1' then
                            ctr_ctrl_reg_updated := '1';
                        end if;
                    end loop;
                    aes_key_init_prev <= aes_key_init;
                    case aes_key_init is
                    when START =>
                        if axi_register_shadow_ext(16#34#+3) (0) = '1' then
                            aes_key_init <= AWAIT_CORE_FREE;
                        end if;
                    when AWAIT_CORE_FREE =>
                        if aes_core_ready = '1' then
                            aes_key_init <= SEND_INIT_PULSE;
                        end if;
                    when SEND_INIT_PULSE =>
                        aes_key_init <= AWAIT_CORE_READY;
                    when AWAIT_CORE_READY =>
                        if aes_core_ready = '1' then
                            aes_key_init <= DONE;
                        end if;
                    when DONE =>
                        -- Don't loop back to start because we want to immediately wait for AES core to be free before re-initializing
                        if axi_ctrl_w_state = DONE and ctr_ctrl_reg_updated = '1' and axi_register_shadow_ext(16#34#+3) (0) = '1' then
                            aes_key_init <= AWAIT_CORE_FREE;
                        end if;
                    end case;
                end if;
            end if;
        end process;
        aes_init_key_outputs: process(aes_key_init) is
        begin
            case aes_key_init is
            when SEND_INIT_PULSE =>
                aes_core_init <= '1';
                axi_register(16#30#+3)(1 downto 0) <= "00";
            when DONE =>
                aes_core_init <= '0';
                axi_register(16#30#+3)(1 downto 0) <= "11";
            when others =>
                aes_core_init <= '0';
                axi_register(16#30#+3)(1 downto 0) <= "00";
            end case;
        end process;
        --aes_core_init <= '1' when aes_key_init = SEND_INIT_PULSE else '0';
        --axi_register(16#30#+3)(0) <= '1' when aes_key_init = DONE else '0';
        --axi_register(16#30#+3)(1) <= '1' when aes_key_init = DONE else '0';

        -- Mux writes to the counter that is used as input and read from
        -- Update from AXI write if that occurs
        -- Update when incremented if that occurs
        aes_ctr_write_mux: process(aclk) is
            variable ctr_ctrl_reg_updated : STD_LOGIC := '0';
        begin
            if rising_edge(aclk) then
                aes_ctr_updated <= '0';
                ctr_ctrl_reg_updated := '0';
                for i in 0 to 2 loop
                    if axi_ctrl_awaddr_internal=(x"34"+i) and axi_ctrl_wstrb_internal(i+1) = '1' then
                        ctr_ctrl_reg_updated := '1';
                    end if;
                end loop;
                if axi_register_shadow_ext(16#34#+2) (0) = '1' and axi_ctrl_w_state = DONE
                        and ctr_ctrl_reg_updated = '1' and 
                        (aes_ctr_init = START or aes_ctr_init = DONE) then
                    for i in 0 to (ctr_size-1)/8 loop
                        for j in 0 to 7 loop
                            aes_counter((ctr_size-8*i)-(8-j)) <= axi_register_shadow_ext(i+16#20#)(j);
                        end loop;
                    end loop;
                    aes_ctr_updated <= '1';
                elsif axi_stream_state = AWAIT_AES_START then --and axi_stream_state_prev = TX_RESULT is guaranteed
                    aes_ctr_updated <= '1';
                    aes_counter <= aes_counter_new;
                end if;
            end if;
        end process;

        aes_init_ctr_fsm: process(aclk) is
        begin
            if rising_edge(aclk) then
                if aresetn = '0' then
                    aes_ctr_init <= START;
                else
                    case aes_ctr_init is
                    when START =>
                        if (aes_ctr_updated = '1' or aes_key_init_prev = AWAIT_CORE_READY) and aes_key_init = DONE then
                            aes_ctr_init <= AWAIT_CORE_FREE;
                        end if;
                    when AWAIT_CORE_FREE =>
                        if aes_core_ready = '1' then
                            aes_ctr_init <= SEND_INIT_PULSE;
                        end if;
                    when SEND_INIT_PULSE =>
                        aes_ctr_init <= AWAIT_CORE_READY;
                    when AWAIT_CORE_READY =>
                        if aes_core_result_valid = '1' then
                            aes_ctr_init <= DONE;
                        end if;
                    when DONE =>
                        if aes_key_init /= DONE then --do not need to check for key_init DONE again
                            aes_ctr_init <= START;
                        elsif aes_ctr_updated = '1' then
                            aes_ctr_init <= AWAIT_CORE_FREE;
                        end if;
                    end case;
                end if;
            end if;
        end process;
        aes_init_ctr_outputs: process(aes_ctr_init) is
        begin
            case aes_ctr_init is
            when SEND_INIT_PULSE =>
                aes_core_read_next <= '1';
                axi_register(16#30#+2)(0) <= '0';
            when DONE =>
                aes_core_read_next <= '0';
                axi_register(16#30#+2)(0) <= '1';
            when others =>
                aes_core_read_next <= '0';
                axi_register(16#30#+2)(0) <= '0';
            end case;
        end process;
        --aes_core_read_next <= '1' when aes_ctr_init = SEND_INIT_PULSE else '0';
        --axi_register(16#30#+2)(0) <= '1' when aes_ctr_init = DONE else '0';
    end block aes_core_init_fsm;
    
    axi_handshake_section: block
    begin
    
    write_endian_hacks: block
    begin
    strbflip_mapping: process(axi_ctrl_wstrb) is
    begin
        if C_W_ADDRALIGN_STRBFLIP then
            for i in 0 to ctrl_data_width/8-1 loop
                axi_ctrl_wstrb_internal(i) <= axi_ctrl_wstrb((ctrl_data_width/8-1)-i);
            end loop;
        else
            axi_ctrl_wstrb_internal <= axi_ctrl_wstrb;
        end if;
    end process;
    
    addrtrim_mapping: process(axi_ctrl_awaddr) is
    begin
        if C_W_ADDRALIGN_STRBFLIP then
            axi_ctrl_awaddr_trimmed(31 downto 2) <= axi_ctrl_awaddr(31 downto 2);
            axi_ctrl_awaddr_trimmed (1 downto 0) <= (others => '0');
        else
            axi_ctrl_awaddr_trimmed <= axi_ctrl_awaddr;
        end if;
    end process;
    end block;
    
    -- Have a singular signal to check for read/write sections
    -- Wrap this in a process to make the code cleaner
    update_addr_rw_status: process(axi_ctrl_araddr_internal, axi_ctrl_awaddr_internal) is
    begin
        axi_ctrl_araddr_readable <= IsAddrReadable(axi_ctrl_araddr_internal);
        axi_ctrl_awaddr_writable <= IsAddrWritable(axi_ctrl_awaddr_internal);
    end process;
    
    ar_process: process(aclk, aresetn) is
        variable addr_temp: UNSIGNED(31 downto 0);
    begin   
        if rising_edge(aclk) then
            if aresetn = '0' then
                -- axi_ctrl_ar_state <= IDLE;
                axi_ctrl_arready <= '0';
                axi_araddr_track_ar <= '0';
            else
                if axi_ctrl_r_state /= PROCESSING then
                    axi_ctrl_arready <= '1';
                else
                    axi_ctrl_arready <= '0';
                end if;
                if axi_ctrl_arvalid = '1' and axi_ctrl_r_state /= PROCESSING then 
                    if (axi_araddr_track_ar /= axi_araddr_track_r) then
                        axi_araddr_track_ar <= not axi_araddr_track_ar;
                    end if;
                    addr_temp := (unsigned(axi_ctrl_araddr) - C_AXI_CTRL_BASEADDR);
                    axi_ctrl_araddr_internal <= addr_temp(7 downto 0);
                end if;
            end if;
        end if;
    end process;
    
    r_process: process(aclk) is
        variable data_slice_helper: INTEGER;
    begin
        if rising_edge(aclk) then
            if aresetn = '0' then
                axi_ctrl_r_state <= IDLE;
                axi_araddr_track_r <= '1';
            else
                case axi_ctrl_r_state is
                when IDLE =>
                    axi_ctrl_araddr_offset <= 0;
                    if axi_ctrl_rready = '1' and axi_araddr_track_ar = axi_araddr_track_r then
                        if axi_ctrl_araddr_readable = '1' then
                            axi_ctrl_r_state <= PROCESSING;
                        else
                            axi_ctrl_r_state <= DONE;
                            axi_ctrl_rresp <= "10"; -- SLVERR
                        end if;
                    end if;
                when PROCESSING =>
                    data_slice_helper := 8*(3-axi_ctrl_araddr_offset);
                    axi_ctrl_rdata(data_slice_helper+7 downto data_slice_helper) <= 
                        axi_register(to_integer(axi_ctrl_araddr_internal)+axi_ctrl_araddr_offset);
                    axi_ctrl_araddr_offset <= axi_ctrl_araddr_offset + 1;
                    if axi_ctrl_araddr_offset = ctrl_data_width/8-1 then
                        axi_ctrl_araddr_offset <= 0;
                        axi_ctrl_r_state <= DONE;
                        axi_ctrl_rresp <= "00"; -- OK
                    end if;
                when DONE =>
                    axi_araddr_track_r <= not axi_araddr_track_r;
                    axi_ctrl_r_state <= IDLE;
                end case;
            end if;
        end if;
    end process;
    axi_ctrl_rvalid <= '1' when axi_ctrl_r_state = DONE else '0';
    
    aw_process: process(aclk) is
        variable addr_temp: UNSIGNED(31 downto 0);
    begin
        if rising_edge(aclk) then
            if aresetn = '0' then
                -- axi_ctrl_ar_state <= IDLE;
                axi_ctrl_awready <= '0';
                axi_awaddr_track_aw <= '0';
            else
                if axi_ctrl_w_state /= PROCESSING then
                    axi_ctrl_awready <= '1';
                else
                    axi_ctrl_awready <= '0';
                end if;
                if axi_ctrl_awvalid = '1' and axi_ctrl_w_state /= PROCESSING then
                    if (axi_awaddr_track_aw /= axi_awaddr_track_w) then
                        axi_awaddr_track_aw <= not axi_awaddr_track_aw;
                    end if;
                    addr_temp := unsigned(axi_ctrl_awaddr_trimmed) - C_AXI_CTRL_BASEADDR;
                    axi_ctrl_awaddr_internal <= addr_temp(7 downto 0);
                end if;
            end if;
        end if;
    end process;
    
    w_process: process(aclk) is
        variable data_slice_helper: INTEGER;
    begin
        if rising_edge(aclk) then
            if aresetn = '0' then
                axi_ctrl_w_state <= IDLE;
            else
                case axi_ctrl_w_state is
                when IDLE =>
                    axi_ctrl_b_avail <= '0';
                    axi_ctrl_awaddr_offset <= 0;
                    if axi_ctrl_wvalid = '1' and axi_awaddr_track_aw = axi_awaddr_track_w then
                        if unsigned(axi_ctrl_wstrb_internal and not axi_ctrl_awaddr_writable) = 0 then
                            axi_ctrl_w_state <= PROCESSING;
                        else
                            axi_ctrl_w_state <= DONE;
                            axi_ctrl_b_avail <= '1';
                            axi_ctrl_bresp <= "10";
                        end if;
                    end if;
                when PROCESSING =>
                    data_slice_helper := 8*(ctrl_data_width/8-1-axi_ctrl_awaddr_offset);
                    if axi_ctrl_wstrb_internal(ctrl_data_width/8-1-axi_ctrl_awaddr_offset) = '1' then
                        axi_register_shadow_ext(to_integer(axi_ctrl_awaddr_internal)+axi_ctrl_awaddr_offset) 
                            <= axi_ctrl_wdata(data_slice_helper+7 downto data_slice_helper);
                    end if;
                    axi_ctrl_awaddr_offset <= axi_ctrl_awaddr_offset + 1;
                    if axi_ctrl_awaddr_offset = ctrl_data_width/8-1 then
                        axi_ctrl_awaddr_offset <= 0;
                        axi_ctrl_w_state <= DONE;
                        axi_ctrl_b_avail <= '1';
                        axi_ctrl_bresp <= "00";
                    end if;
                when DONE =>
                    axi_awaddr_track_w <= not axi_awaddr_track_w;
                    axi_ctrl_w_state <= IDLE;
                end case;
            end if;
        end if;
    end process;
    axi_ctrl_wready <= '1' when axi_ctrl_w_state=DONE else '0';
    
    b_process: process(aclk) is
    begin
        if rising_edge(aclk) then
            if aresetn = '0' then
                axi_ctrl_bvalid_internal <= '0';
            elsif axi_ctrl_bvalid_internal = '0' and axi_ctrl_b_avail = '1' then
                axi_ctrl_bvalid_internal <= '1';
            elsif axi_ctrl_bvalid_internal = '1' and axi_ctrl_bready = '1' then
                axi_ctrl_bvalid_internal <= '0';
            end if;
        end if;
    end process;
    
    axi_ctrl_bvalid <= axi_ctrl_bvalid_internal;
    
    end block axi_handshake_section;

    axi_stream_encryption: block is
    begin
    axi_stream_process_fsm: process(aclk) is
    begin
        if rising_edge(aclk) then
            if aresetn = '0' then
                axi_stream_state <= AWAIT_AES_CORE;
                aes_ctr_block_section <= (ctr_size-1)/stream_data_width;
                aes_core_result_valid_ctr <= 0;
                axis_output_tlast <= '0';
            else
                case axi_stream_state is
                when AWAIT_AES_CORE =>
                    axis_output_tlast <= '0';
                    -- Wait for AES core to be ready for at least 4 clock cycles
                    if aes_core_ready = '1' and aes_core_result_valid = '1' then
                        if aes_core_result_valid_ctr = 3 then
                            axi_stream_state <= IDLE;
                            aes_core_result_valid_ctr <= 0;
                        else
                            aes_core_result_valid_ctr <= aes_core_result_valid_ctr + 1;
                        end if;
                    else
                        aes_core_result_valid_ctr <= 0;
                    end if;
                when IDLE =>
                    axis_output_tlast <= '0';
                    axis_output_tdata <= axis_input_tdata xor 
                        aes_core_result(aes_ctr_block_section*stream_data_width+stream_data_width-1 downto aes_ctr_block_section*stream_data_width);
                    if not (aes_key_init = DONE) then
                        axi_stream_state <= AWAIT_AES_CORE;
                        aes_ctr_block_section <= (ctr_size-1)/stream_data_width;
                    elsif axis_input_tvalid = '1' then
                        aes_counter_new <= aes_counter; 
                        axi_stream_state <= TX_RESULT;
                        if aes_ctr_block_section = 0 or axis_input_tlast = '1' then
                            axis_output_tlast <= '1';
                        else
                            axis_output_tlast <= '0';
                        end if;
                    end if;
                when TX_RESULT =>
                    if axis_output_tready = '1' then
                        axis_output_tlast <= '0';
                        if aes_ctr_block_section = 0 or not (aes_key_init = DONE and aes_core_result_valid = '1') then
                            aes_ctr_block_section <= (ctr_size-1)/stream_data_width;
                            aes_counter_new(ctr_incr_size-1 downto 0)
                                    <= std_logic_vector(unsigned(aes_counter(ctr_incr_size-1 downto 0)) + 1);
                            axi_stream_state <= AWAIT_AES_START;
                        else
                            aes_ctr_block_section <= aes_ctr_block_section - 1;
                            axi_stream_state <= IDLE;
                        end if;
                    end if;
                when AWAIT_AES_START =>
                    axis_output_tlast <= '0';
                    if aes_core_result_valid = '0' then
                        axi_stream_state <= AWAIT_AES_CORE;
                    end if;
                end case;
            end if;
        end if;
    end process;
    
    axi_stream_process_handshake_outputs: process(axi_stream_state) is
    begin
        case axi_stream_state is
        when IDLE =>
            axis_input_tready <= '1';
            axis_output_tvalid <= '0';
        when TX_RESULT =>
            axis_input_tready <= '0';
            axis_output_tvalid <= '1';
        when others =>
            axis_input_tready <= '0';
            axis_output_tvalid <= '0';
        end case;
    end process;
    
    axi_stream_tdest_mapping: block is
    begin
    axi_register(16#30#+1) <= axi_register_shadow_ext(16#34#+1);
    axis_output_tdest <= axi_register_shadow_ext(16#34#+1);
    end block axi_stream_tdest_mapping;
    
    end block axi_stream_encryption;
end Behavioral;
