// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:Berger
/******************************************************************************

Fidelity Chess Challenger 7 (BCC)
------------------------
model CC7 is an older version
RE information from netlist by Berger

Zilog Z80A, 3.579MHz from XTAL
Z80 IRQ/NMI unused, no timer IC.
This is a cost-reduced design from CC10, no special I/O chips.

Backgammon Challenger (BKC) is the same PCB, with the speaker connection going
to the display panel instead.

Memory map:
-----------
0000-0FFF: 4K 2332 ROM CN19103N BCC-REVB.
2000-2FFF: ROM/RAM bus conflict!
3000-3FFF: 256 bytes RAM (2111 SRAM x2)
4000-FFFF: Z80 A14/A15 not connected

Port map (Write):
---------
D0-D3: digit select and keypad mux
D4: LOSE led
D5: CHECK led
A0-A2: NE591 A0-A2
D7: NE591 D (_C not used)
NE591 Q0-Q6: digit segments A-G
NE591 Q7: buzzer

Port map (Read):
---------
D0-D3: keypad row

******************************************************************************/

#include "emu.h"
#include "includes/fidelbase.h"

#include "cpu/z80/z80.h"
#include "sound/volt_reg.h"
#include "speaker.h"

// internal artwork
#include "fidel_bcc.lh" // clickable
#include "fidel_bkc.lh" // clickable


namespace {

class bcc_state : public fidelbase_state
{
public:
	bcc_state(const machine_config &mconfig, device_type type, const char *tag) :
		fidelbase_state(mconfig, type, tag)
	{ }

	// machine drivers
	void bcc(machine_config &config);
	void bkc(machine_config &config);

private:
	// address maps
	void main_map(address_map &map);
	void main_io(address_map &map);

	// I/O handlers
	DECLARE_READ8_MEMBER(input_r);
	DECLARE_WRITE8_MEMBER(control_w);
};



/******************************************************************************
    Devices, I/O
******************************************************************************/

// TTL

WRITE8_MEMBER(bcc_state::control_w)
{
	// a0-a2,d7: digit segment data via NE591
	u8 mask = 1 << (offset & 7);
	m_7seg_data = (m_7seg_data & ~mask) | ((data & 0x80) ? mask : 0);

	// BCC: NE591 Q7 is speaker out
	if (m_dac != nullptr)
		m_dac->write(BIT(m_7seg_data, 7));

	// d0-d3: led select, input mux
	// d4,d5: upper leds(direct)
	set_display_segmask(0xf, 0x7f);
	display_matrix(8, 6, m_7seg_data, data & 0x3f);
	m_inp_mux = data & 0xf;
}

READ8_MEMBER(bcc_state::input_r)
{
	// d0-d3: multiplexed inputs
	return read_inputs(4);
}



/******************************************************************************
    Address Maps
******************************************************************************/

void bcc_state::main_map(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0x3fff);
	map(0x0000, 0x0fff).rom();
	map(0x3000, 0x30ff).mirror(0x0f00).ram();
}

void bcc_state::main_io(address_map &map)
{
	map.global_mask(0x07);
	map(0x00, 0x07).rw(FUNC(bcc_state::input_r), FUNC(bcc_state::control_w));
}



/******************************************************************************
    Input Ports
******************************************************************************/

static INPUT_PORTS_START( bcc )
	PORT_START("IN.0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("EN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("PV") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("D4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("H8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CODE(KEYCODE_H)

	PORT_START("IN.1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("PB") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("C3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CODE(KEYCODE_C)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("G7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CODE(KEYCODE_G)

	PORT_START("IN.2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CB") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("DM") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("B2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("F6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CODE(KEYCODE_F)

	PORT_START("IN.3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("RE") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("LV") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("A1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CODE(KEYCODE_A)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("E5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CODE(KEYCODE_E)
INPUT_PORTS_END

static INPUT_PORTS_START( bkc )
	PORT_START("IN.0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("EN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD)

	PORT_START("IN.1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("CL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD)

	PORT_START("IN.2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("GM") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD)

	PORT_START("IN.3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("RE") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("PB") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("PV") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD)
INPUT_PORTS_END



/******************************************************************************
    Machine Drivers
******************************************************************************/

void bcc_state::bkc(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, 3.579545_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &bcc_state::main_map);
	m_maincpu->set_addrmap(AS_IO, &bcc_state::main_io);

	TIMER(config, "display_decay").configure_periodic(FUNC(fidelbase_state::display_decay_tick), attotime::from_msec(1));
	config.set_default_layout(layout_fidel_bkc);
}

void bcc_state::bcc(machine_config &config)
{
	bkc(config);
	config.set_default_layout(layout_fidel_bcc);

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();
	DAC_1BIT(config, m_dac, 0).add_route(ALL_OUTPUTS, "speaker", 0.25);
	voltage_regulator_device &vref(VOLTAGE_REGULATOR(config, "vref", 0));
	vref.add_route(0, "dac", 1.0, DAC_VREF_POS_INPUT);
}



/******************************************************************************
    ROM Definitions
******************************************************************************/

ROM_START( cc7 ) // model BCC
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cn19103n_bcc-revb", 0x0000, 0x1000, CRC(a397d471) SHA1(9b12bc442fccee40f4d8500c792bc9d886c5e1a5) ) // 2332
ROM_END


ROM_START( backgamc ) // model BKC, PCB label P-380A-5
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cn19255n_101-32012", 0x0000, 0x1000, CRC(0a8a19b7) SHA1(d6f0dd44b33c9b79570cf0ceac02a036ec91ba57) ) // 2332
ROM_END

} // anonymous namespace



/******************************************************************************
    Drivers
******************************************************************************/

//    YEAR  NAME      PARENT CMP MACHINE  INPUT  STATE      INIT        COMPANY, FULLNAME, FLAGS
CONS( 1979, cc7,      0,      0, bcc,     bcc,   bcc_state, empty_init, "Fidelity Electronics", "Chess Challenger 7 (model BCC, rev. B)", MACHINE_SUPPORTS_SAVE | MACHINE_CLICKABLE_ARTWORK )

CONS( 1979, backgamc, 0,      0, bkc,     bkc,   bcc_state, empty_init, "Fidelity Electronics", "Backgammon Challenger", MACHINE_SUPPORTS_SAVE | MACHINE_CLICKABLE_ARTWORK | MACHINE_NO_SOUND_HW )