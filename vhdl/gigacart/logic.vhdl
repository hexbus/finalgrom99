-- FinalGROM Gigacart development top level.
-- SPDX-License-Identifier: GPL-3.0-or-later
-- Copyright (c) 2017 Ralph Benzinger. Modified for this investigation.
-- GROM write/increment timing follows JJB's supplied JB_basic.vhdl.
-- Compatibility candidate: ordinary ROM/GROM plus cooperative paging.
-- Experiment: native single-bank 4 KiB RAM. GRAM/dump remain unsupported.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity logic is
  -- Paging uses the existing mailbox ABI; native read-only cartridges
  -- retain address banking and all five GROM banks.
  generic (ENABLE_MAILBOX : boolean := true);
  port(ROMS,WE,GS,DBIN,GC,GR : in std_logic;
    BUS_DX : inout std_logic_vector(7 downto 0);
    BUS_AX : in std_logic_vector(12 downto 0);
    RAM_DX : inout std_logic_vector(7 downto 0);
    RAM_AX : out std_logic_vector(19 downto 0);
    RAM_OE,RAM_WE : out std_logic;
    CCMD : in std_logic_vector(2 downto 0);
    CCLK : in std_logic;
    CDX : inout std_logic_vector(7 downto 0);
    CX : inout std_logic);
end entity;

architecture rtl of logic is
  signal offline,previous_offline : std_logic := '1';
  signal giga : std_logic := '0';
  signal ram_mode : std_logic := '0';
  signal native_ram_write : std_logic;
  signal load_addr : unsigned(19 downto 0) := (others=>'0');
  signal fill_phase : std_logic_vector(2 downto 0) := "001";
  signal grom_addr : unsigned(15 downto 0);
  signal address_clock,address_enable : std_logic;
  signal native_address_clock : std_logic := '0';
  signal address_next : unsigned(19 downto 0);
  signal grom_mask : std_logic_vector(4 downto 0) := (others=>'0');
  signal slot_clock,slot_enable : std_logic;
  signal slot_data : std_logic_vector(6 downto 0);
  signal grom_sel : std_logic := '0';
  signal grit : std_logic := '1';
  signal grrd,grwa,grwi : std_logic;
  -- In menu mode the same latch carries the existing reload sender's byte
  -- and clock. This avoids keeping a second, idle bank latch in this CPLD.
  signal logical_bank : unsigned(13 downto 0) := (others=>'0');
  signal slot : std_logic_vector(6 downto 0) := (others=>'0');
  signal valid : std_logic := '1';
  signal scene : std_logic_vector(5 downto 0) := (others=>'0');
  signal fault : std_logic := '0';
  signal grant,data_drive : std_logic := '0';
  -- First stage of the CCLK-domain idle sampler. Only grant samples this
  -- register; it must never directly drive SRAM pins or the write strobe.
  signal idle_sample : std_logic := '0';
  signal accepted : std_logic := '0';
  signal busy,lookup,fill_mode,fill_write,rom_read : std_logic;
  signal ti_addr : std_logic_vector(19 downto 0);
  signal cooperative : std_logic;
  signal request_epoch,ack_epoch : std_logic := '0';
  signal mailbox_session : std_logic := '0';
  signal select_armed : std_logic := '0';
  signal mailbox,mailbox_read,mailbox_select : std_logic;
begin
  grom_addr<=load_addr(15 downto 0);
  address_clock<=not grwi when (offline='0' and giga='0') or native_address_clock='1' else CCLK;
  cooperative <= giga when ENABLE_MAILBOX else '0';
  -- Native RAM uses the existing SRAM upper half of bank zero. The AVR
  -- rejects RAM-mode images larger than one 8 KiB CPU window.
  native_ram_write <= '1' when offline='0' and giga='0' and ram_mode='1' and
    ROMS='0' and DBIN='0' and BUS_AX(12)='1' else '0';
  -- >7FF8/9 request low, >7FFA/B request high, >7FFC/D status/select,
  -- >7FFE/F commit. Only even-byte writes have effects. No raw bank writes
  -- are permitted in cooperative mode; all execution leases use this ABI.
  mailbox <= '1' when ENABLE_MAILBOX and cooperative='1' and giga='1' and
    offline='0' and select_armed='0' and BUS_AX(12 downto 3)="1111111111" else '0';
  -- The TMS9900 reads even a MOV destination before writing it. After ARM,
  -- serve status at any native select address until its even-byte write;
  -- that discarded read must not depend on the released old ROM slot.
  mailbox_read <= '1' when ENABLE_MAILBOX and cooperative='1' and offline='0' and
    ROMS='0' and DBIN='1' and (select_armed='1' or
    (mailbox='1' and mailbox_session='1')) else '0';
  mailbox_select <= '1' when mailbox='1' and BUS_AX(2 downto 1)="10" else '0';
  busy <= '1' when ROMS='0' or GS='0' else '0';
  lookup <= '1' when offline='0' and giga='1' and ROMS='0' and WE='0' and
    (cooperative='0' or select_armed='1') else '0';
  fill_mode <= '1' when offline='0' and giga='1' and CCMD="011" else '0';
  fill_write <= '1' when fill_mode='1' and fill_phase(1)='1' and CCLK='0' and grant='1' else '0';
  rom_read <= '1' when offline='0' and ROMS='0' and DBIN='1' and mailbox_read='0' else '0';

  ti_addr <= "111111" & BUS_DX(1 downto 0) & BUS_AX(12 downto 1) when lookup='1' else
    "1111" & std_logic_vector(grom_addr) when grrd='0' else
    slot & BUS_AX when giga='1' else std_logic_vector(logical_bank(6 downto 0) and unsigned(slot)) & BUS_AX;
  RAM_AX <= std_logic_vector(load_addr) when offline='1' or grant='1' else ti_addr;
  BUS_DX <= fault & "0000" & ack_epoch & request_epoch & not(request_epoch xor ack_epoch)
      when mailbox_read='1' else
    RAM_DX when rom_read='1' and grant='0' and (giga='0' or (valid='1' and fault='0')) else
    RAM_DX when offline='0' and grant='0' and grrd='0' and GR='1' and grom_sel='1' else (others=>'Z');
  RAM_DX <= CDX when (offline='1' and CCMD="111") or data_drive='1' else
    BUS_DX when native_ram_write='1' else (others=>'Z');
  -- A granted transfer completes without asynchronously switching SRAM pins.
  -- Three uninterrupted pulses, nominal 125 ns half-periods. Sample idle
  -- on the first falling edge, then grant on the rising edge. Phase 100
  -- releases the data driver after write end. Grant resets only after that
  -- registered driver control is low; the final rising edge advances the
  -- address. Pin ordering still requires fitted/board qualification.
  -- Candidate timing must be checked against docs/ti-bus-timing.md.
  RAM_OE <= '1' when (offline='1' and CCMD="111") or grant='1' or native_ram_write='1' else '0';
  RAM_WE <= CX when (offline='1' and CCMD="111") and CCLK='0' else '0' when fill_write='1' or (native_ram_write='1' and WE='0') else '1';

  CDX <= std_logic_vector(logical_bank(7 downto 0)) when offline='0' and giga='0' else
    "101010" & ack_epoch & request_epoch when offline='0' and giga='1' and
      cooperative='1' and CCMD="000" and CCLK='0' else
    x"A7" when offline='0' and giga='1' and CCMD="000" and CCLK='0' else
    std_logic_vector(logical_bank(7 downto 0)) when offline='0' and giga='1' and CCMD="000" else
    fault & valid & scene when offline='0' and giga='1' and CCMD="001" and CCLK='0' else
    "00" & std_logic_vector(logical_bank(13 downto 8)) when offline='0' and giga='1' and CCMD="001" else
    (others=>'Z');
  CX <= logical_bank(8) when offline='0' and giga='0' else
    accepted when offline='0' and giga='1' and CCMD="011" else
    fault when offline='0' and giga='1' else 'Z';

  -- Mutually exclusive roles: legacy bank mask / mailbox physical slot.
  -- Mode changes occur with both clocks parked high by the transfer protocol.
  slot_clock<=CCLK when offline='1' else WE;
  slot_enable<='1' when (offline='1' and CCMD="100") or
    (offline='0' and giga='1' and ROMS='0' and
     ((cooperative='1' and select_armed='1' and BUS_AX(0)='0') or
      (not ENABLE_MAILBOX and fault='0'))) else '0';
  slot_data<=CDX(6 downto 0) when offline='1' and giga='0' and CDX(7)='0' else
    "0000000" when offline='1' else RAM_DX(6 downto 0);
  slot_capture: process(slot_clock)
  begin
    if rising_edge(slot_clock) then
      if slot_enable='1' then slot<=slot_data;end if;
    end if;
  end process;

  bank_capture: process(WE,offline,CCMD)
    variable b : unsigned(13 downto 0);
  begin
    if (offline='1' and CCMD="100") then
      logical_bank<=(others=>'0');valid<='1';scene<=(others=>'0');
    elsif rising_edge(WE) then
      if offline='0' and ROMS='0' then
        if giga='0' then
          if ram_mode='0' or BUS_AX(12)='0' then
            logical_bank <= "00000" & unsigned(BUS_AX(12) & BUS_AX(8 downto 1));
          end if;
        elsif cooperative='1' then
          if mailbox='1' and BUS_AX(0)='0' and request_epoch=ack_epoch then
            case BUS_AX(2 downto 1) is
              when "00" => logical_bank(7 downto 0)<=unsigned(BUS_DX);
              when "01" => logical_bank(13 downto 8)<=unsigned(BUS_DX(5 downto 0));
              when others => null;
            end case;
          elsif select_armed='1' and BUS_AX(0)='0' then
            -- The odd byte of a TI word write never updates the ROM latch.
            -- Helper issues the original address/data selection after ACK.
            logical_bank<=unsigned(BUS_DX(1 downto 0) & BUS_AX(12 downto 1));
            
            if RAM_DX(7)='1' and unsigned(RAM_DX(6 downto 0))<125 then valid<='1';else valid<='0';end if;
          end if;
        elsif not ENABLE_MAILBOX and fault='0' then
          b:=unsigned(BUS_DX(1 downto 0) & BUS_AX(12 downto 1));
          logical_bank<=b;
          if RAM_DX(7)='1' and unsigned(RAM_DX(6 downto 0))<125 then valid<='1';else valid<='0';end if;
          -- The opt-in demand-only prototype replaces the sampled scene
          -- observer with explicit requests. Do not allocate both observers.
          if not ENABLE_MAILBOX and b>=8 and b<=47 then scene<=std_logic_vector(b(5 downto 0));end if;
        end if;
      end if;
    end if;
  end process;
  miss_capture: process(offline,rom_read)
  begin
    if offline='1' then fault<='0';
    elsif rising_edge(rom_read) then
      if giga='1' and valid='0' then fault<='1';end if;
    end if;
  end process;

  idle_sampler: process(fill_mode,CCLK)
  begin
    if fill_mode='0' then idle_sample<='0';
    elsif falling_edge(CCLK) then idle_sample<=not busy;end if;
  end process;
  -- The read overlay exists only between staging and SELECT. After SELECT
  -- every byte of the 8 KiB payload, including >7FF8..>7FFF, is visible.
  mailbox_transaction: process(WE,offline)
  begin
    if offline='1' then mailbox_session<='0';select_armed<='0';
    elsif rising_edge(WE) then
      if ROMS='0' and BUS_AX(0)='0' and select_armed='1' then select_armed<='0';end if;
      if mailbox='1' and ROMS='0' and BUS_AX(0)='0' and request_epoch=ack_epoch then
        if BUS_AX(2 downto 1)="00" then mailbox_session<='1';
        elsif BUS_AX(2 downto 1)="10" then mailbox_session<='0';select_armed<='1';end if;
      end if;
    end if;
  end process;
  -- Single outstanding request. COMMIT explicitly releases the prior lease:
  -- the caller must already be wholly in safe RAM, interrupts disabled, with
  -- no dependency on that old bank. Request data stays stable until ACK.
  mailbox_request: process(WE,offline)
  begin
    if offline='1' then request_epoch<='0';
    elsif rising_edge(WE) then
      if mailbox='1' and mailbox_session='1' and ROMS='0' and BUS_AX(2 downto 0)="110" and request_epoch=ack_epoch then
        request_epoch<=not request_epoch;
      end if;
    end if;
  end process;
  -- In cooperative mode only, a command-001 rising clock is ACK. The AVR
  -- must reserve and publish the requested bank before issuing this pulse.
  -- Query uses command 000 (low: epochs; high: bank low), then 001 HIGH
  -- (bank high). Do NOT use legacy giga_wire_status in cooperative mode.
  mailbox_ack: process(CCLK,offline)
  begin
    if offline='1' then ack_epoch<='0';
    elsif rising_edge(CCLK) then
      if ENABLE_MAILBOX and cooperative='1' and giga='1' and CCMD="001" then ack_epoch<=request_epoch;end if;
    end if;
  end process;
  write_grant: process(fill_mode,CCLK,fill_phase,data_drive)
  begin
    if fill_mode='0' or (fill_phase(2)='1' and data_drive='0') then grant<='0';
    elsif rising_edge(CCLK) then
      if fill_phase(0)='1' then grant<=idle_sample;end if;
    end if;
  end process;
  drive_control: process(fill_mode,CCLK,fill_phase)
  begin
    -- The registered phase transition follows the direct write-clock edge.
    -- Fitted minimum/maximum pin timing must establish nonnegative data hold.
    if fill_mode='0' or fill_phase(2)='1' then data_drive<='0';
    elsif falling_edge(CCLK) then
      if fill_phase(1)='1' then data_drive<=grant;else data_drive<='0';end if;
    end if;
  end process;
  fill_ack: process(fill_mode,CCLK)
  begin
    if fill_mode='0' then accepted<='0';
    elsif rising_edge(CCLK) then
      if fill_phase(0)='1' then accepted<='0';
      elsif fill_phase(1)='1' then accepted<=grant;end if;
    end if;
  end process;
  fill_sequence: process(fill_mode,CCLK)
  begin
    if fill_mode='0' then fill_phase<="001";
    elsif rising_edge(CCLK) then
      fill_phase<=fill_phase(1 downto 0) & fill_phase(2);
    end if;
  end process;

  -- Mutually exclusive native GROM and AVR SRAM-address lifetimes.
  -- Decode data/enable before the selected clock, avoiding a delta-cycle
  -- race when a GROM address write updates its low byte.
  address_decode: process(load_addr,offline,giga,BUS_AX,BUS_DX,CCMD,CDX,fill_mode,fill_phase,accepted)
  begin
    address_next<=load_addr;address_enable<='0';
    if offline='0' and giga='0' then
      address_enable<='1';
      if BUS_AX(1)='1' then address_next(15 downto 0)<=load_addr(7 downto 0) & unsigned(BUS_DX);
      else address_next(12 downto 0)<=load_addr(12 downto 0)+1;end if;
    elsif offline='1' and CCMD="100" then
      address_enable<='1';
      if CDX(7)='0' then
        if giga='1' then address_next<=unsigned(CDX(6 downto 0)) & to_unsigned(0,13);
        else address_next<=(others=>'0');end if;
      else address_next<=to_unsigned(16#F6000#,20);end if;
    elsif offline='0' and giga='1' and CCMD="010" then
      address_enable<='1';address_next<=load_addr(11 downto 0) & unsigned(CDX);
    elsif (offline='1' and CCMD="111") or
      (fill_mode='1' and fill_phase(2)='1' and accepted='1') then
      address_enable<='1';address_next(12 downto 0)<=load_addr(12 downto 0)+1;
    end if;
  end process;
  address_control: process(address_clock,offline,CCMD)
  begin
    -- CONFIG starts a new offline address lifetime. Clear the shared
    -- counter while its clock source changes, before any SRAM loading.
    if offline='1' and CCMD="101" then load_addr<=(others=>'0');
    elsif rising_edge(address_clock) then
      if address_enable='1' then load_addr<=address_next;end if;
    end if;
  end process;
  mask_control: process(CCLK)
  begin
    if rising_edge(CCLK) then
      if offline='1' and CCMD="100" and CDX(7)='1' then grom_mask<=CDX(4 downto 0);end if;
    end if;
  end process;

  -- JJB/QI timing: GR is an input, never used here to stall a ROM access.
  grrd<='0' when GS='0' and DBIN='1' and WE='1' and BUS_AX(1)='0' else '1';
  grwa<='0' when GS='0' and DBIN='0' and WE='0' and BUS_AX(1)='1' else '1';
  grwi<='0' when grwa='0' or grit='0' else '1';
  increment_trigger: process(BUS_AX(1),GR)
  begin
    if BUS_AX(1)='1' then grit<='1';
    elsif falling_edge(GR) then if DBIN='0' then grit<='0';end if;end if;
  end process;
  grom_control: process(grwi,offline,CCMD,CDX)
  begin
    if (offline='1' and CCMD="100") and CDX(7 downto 6)="10" then
      grom_sel<='0';
    elsif falling_edge(grwi) then
      if BUS_AX(1)='1' then

        if grom_addr(7 downto 5)="011" then grom_sel<=grom_mask(0);
        elsif grom_addr(7 downto 5)="100" then grom_sel<=grom_mask(1);
        elsif grom_addr(7 downto 5)="101" then grom_sel<=grom_mask(2);
        elsif grom_addr(7 downto 5)="110" then grom_sel<=grom_mask(3);
        elsif grom_addr(7 downto 5)="111" then grom_sel<=grom_mask(4);
        else grom_sel<='0';end if;
      else
        -- Address data reads increment the shared counter above.
        null;
      end if;
    end if;
  end process;
  -- Keep the GROM clock selected until CONFIG has asserted the shared
  -- counter reset. Otherwise changing to a parked-high CCLK creates an
  -- artificial rising edge while address data is changing.
  address_lifetime: process(offline,CCMD)
  begin
    if offline='1' and CCMD="101" then native_address_clock<='0';
    elsif falling_edge(offline) then
      native_address_clock<=not giga;
    end if;
  end process;
  mode_filter: process(GC)
  begin
    if rising_edge(GC) then
      previous_offline<=CCMD(2);
      if previous_offline=CCMD(2) then offline<=CCMD(2);end if;
      if offline='1' and CCMD="101" then giga<=CDX(2);ram_mode<=CDX(0);end if;
    end if;
  end process;
end architecture;
