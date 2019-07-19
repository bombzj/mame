// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic, AJR
/***************************************************************************

    PolyMorphic Systems Video Terminal Interface

    Original implementation by Miodrag Milanovic
    Converted to S-100 bus device by AJR

    This video and keyboard interface board was a primary component of
    PolyMorphic's System 88, but was also sold for use in other S-100
    systems.

    Any generic keyboard with a parallel ASCII interface can be used.
    The actual keyboard provided by PolyMorphic Systems is almost entirely
    based on TTL/LSTTL components.

    The video timing circuit has no fixed dot clock, which is instead
    generated by a VCO connected to a user-adjustable potentiometer. The
    blanking and sync frequencies, on the other hand, are divisions of
    either pin 49 of the S-100 bus or an optionally installable 2 MHz XTAL.

****************************************************************************/

#include "emu.h"
#include "polyvti.h"

#include "machine/i8212.h"
#include "machine/keyboard.h"
#include "emupal.h"
#include "screen.h"

class poly_vti_device : public device_t, public device_s100_card_interface
{
	static const u8 s_mcm6571a_shift[];

public:
	// construction/destruction
	poly_vti_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_add_mconfig(machine_config &config) override;
	virtual const tiny_rom_entry *device_rom_region() const override;

	// device_s100_card_interface overrides
	virtual u8 s100_smemr_r(offs_t offset) override;
	virtual void s100_mwrt_w(offs_t offset, u8 data) override;
	virtual u8 s100_sinp_r(offs_t offset) override;

private:
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void kbd_put(u8 data);
	DECLARE_WRITE_LINE_MEMBER(kbd_int_w);

	// object finders
	required_device<i8212_device> m_kbdlatch;
	required_region_ptr<u8> m_FNT;
	required_ioport m_address;

	// internal state
	std::unique_ptr<u8[]> m_video_ram;
};

DEFINE_DEVICE_TYPE_PRIVATE(S100_POLY_VTI, device_s100_card_interface, poly_vti_device, "polyvti", "PolyMorphic Systems Video Terminal Interface")

poly_vti_device::poly_vti_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, S100_POLY_VTI, tag, owner, clock)
	, device_s100_card_interface(mconfig, *this)
	, m_kbdlatch(*this, "kbdlatch")
	, m_FNT(*this, "chargen")
	, m_address(*this, "ADDRESS")
{
}

void poly_vti_device::device_start()
{
	m_video_ram = make_unique_clear<u8[]>(0x400);
	save_pointer(NAME(m_video_ram), 0x400);
}

const u8 poly_vti_device::s_mcm6571a_shift[] =
{
	0,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,
	1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,
	1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0
};

u32 poly_vti_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	for (int y = 0; y < 16; y++)
	{
		u16 addr = y*64;
		int xpos = 0;
		for (int x = 0; x < 64; x++)
		{
			u8 code = m_video_ram[addr + x];
			if ((code & 0x80)==0)
			{
				for (int j = 0; j < 15; j++)
				{
					u8 l = j/5;
					for (int b = 0; b < 10; b++)
					{
						u8 r = b/5;
						if (l==0 && r==0)
							bitmap.pix16(y*15+j, xpos+b ) = BIT(code,5) ? 0 : 1;

						if (l==0 && r==1)
							bitmap.pix16(y*15+j, xpos+b ) = BIT(code,2) ? 0 : 1;

						if (l==1 && r==0)
							bitmap.pix16(y*15+j, xpos+b ) = BIT(code,4) ? 0 : 1;

						if (l==1 && r==1)
							bitmap.pix16(y*15+j, xpos+b ) = BIT(code,1) ? 0 : 1;

						if (l==2 && r==0)
							bitmap.pix16(y*15+j, xpos+b ) = BIT(code,3) ? 0 : 1;

						if (l==2 && r==1)
							bitmap.pix16(y*15+j, xpos+b ) = BIT(code,0) ? 0 : 1;
					}
				}
			}
			else
			{
				for (int j = 0; j < 15; j++)
				{
					code &= 0x7f;
					int l = 0;
					if (s_mcm6571a_shift[code]==0)
					{
						if (j < 9)
							l = m_FNT[code*16 + j];
					}
					else
					{
						if ((j > 2) && (j < 12))
							l = m_FNT[code*16 + j - 3];
					}

					for (int b = 0; b < 7; b++)
						bitmap.pix16(y*15+j, xpos+b ) =  (l >> (6-b)) & 1;

					bitmap.pix16(y*15+j, xpos+7 ) =  0;
					bitmap.pix16(y*15+j, xpos+8 ) =  0;
					bitmap.pix16(y*15+j, xpos+9 ) =  0;
				}
			}
			xpos += 10;
		}
	}
	return 0;
}

u8 poly_vti_device::s100_smemr_r(offs_t offset)
{
	if ((offset & 0xfc00) >> 10 == m_address->read())
		return m_video_ram[offset & 0x3ff];

	return 0xff;
}

void poly_vti_device::s100_mwrt_w(offs_t offset, u8 data)
{
	if ((offset & 0xfc00) >> 10 == m_address->read())
		m_video_ram[offset & 0x3ff] = data;
}

u8 poly_vti_device::s100_sinp_r(offs_t offset)
{
	if ((offset & 0xfc00) >> 10 == m_address->read())
		return m_kbdlatch->read(machine().dummy_space(), 0);

	return 0xff;
}

void poly_vti_device::kbd_put(u8 data)
{
	if (data)
	{
		if (data==8)
			data=127;  // fix backspace
		m_kbdlatch->strobe(machine().dummy_space(), 0, data);
	}
}

WRITE_LINE_MEMBER(poly_vti_device::kbd_int_w)
{
	// TODO: jumper selectable
	m_bus->vi2_w(state);
}

static INPUT_PORTS_START(polyvti)
	PORT_START("ADDRESS")
	PORT_DIPNAME(0x3f, 0x3e, "Address Range") PORT_DIPLOCATION("SW:2,3,4,5,6,7")
	PORT_DIPSETTING(0x00, "0000-03FF")
	PORT_DIPSETTING(0x01, "0400-07FF")
	PORT_DIPSETTING(0x02, "0800-0BFF")
	PORT_DIPSETTING(0x03, "0C00-0FFF")
	PORT_DIPSETTING(0x04, "1000-13FF")
	PORT_DIPSETTING(0x05, "1400-17FF")
	PORT_DIPSETTING(0x06, "1800-1BFF")
	PORT_DIPSETTING(0x07, "1C00-1FFF")
	PORT_DIPSETTING(0x08, "2000-23FF")
	PORT_DIPSETTING(0x09, "2400-27FF")
	PORT_DIPSETTING(0x0a, "2800-2BFF")
	PORT_DIPSETTING(0x0b, "2C00-2FFF")
	PORT_DIPSETTING(0x0c, "3000-33FF")
	PORT_DIPSETTING(0x0d, "3400-37FF")
	PORT_DIPSETTING(0x0e, "3800-3BFF")
	PORT_DIPSETTING(0x0f, "3C00-3FFF")
	PORT_DIPSETTING(0x10, "4000-43FF")
	PORT_DIPSETTING(0x11, "4400-47FF")
	PORT_DIPSETTING(0x12, "4800-4BFF")
	PORT_DIPSETTING(0x13, "4C00-4FFF")
	PORT_DIPSETTING(0x14, "5000-53FF")
	PORT_DIPSETTING(0x15, "5400-57FF")
	PORT_DIPSETTING(0x16, "5800-5BFF")
	PORT_DIPSETTING(0x17, "5C00-5FFF")
	PORT_DIPSETTING(0x18, "6000-63FF")
	PORT_DIPSETTING(0x19, "6400-67FF")
	PORT_DIPSETTING(0x1a, "6800-6BFF")
	PORT_DIPSETTING(0x1b, "6C00-6FFF")
	PORT_DIPSETTING(0x1c, "7000-73FF")
	PORT_DIPSETTING(0x1d, "7400-77FF")
	PORT_DIPSETTING(0x1e, "7800-7BFF")
	PORT_DIPSETTING(0x1f, "7C00-7FFF")
	PORT_DIPSETTING(0x20, "8000-83FF")
	PORT_DIPSETTING(0x21, "8400-87FF")
	PORT_DIPSETTING(0x22, "8800-8BFF")
	PORT_DIPSETTING(0x23, "8C00-8FFF")
	PORT_DIPSETTING(0x24, "9000-93FF")
	PORT_DIPSETTING(0x25, "9400-97FF")
	PORT_DIPSETTING(0x26, "9800-9BFF")
	PORT_DIPSETTING(0x27, "9C00-9FFF")
	PORT_DIPSETTING(0x28, "A000-A3FF")
	PORT_DIPSETTING(0x29, "A400-A7FF")
	PORT_DIPSETTING(0x2a, "A800-ABFF")
	PORT_DIPSETTING(0x2b, "AC00-AFFF")
	PORT_DIPSETTING(0x2c, "B000-B3FF")
	PORT_DIPSETTING(0x2d, "B400-B7FF")
	PORT_DIPSETTING(0x2e, "B800-BBFF")
	PORT_DIPSETTING(0x2f, "BC00-BFFF")
	PORT_DIPSETTING(0x30, "C000-C3FF")
	PORT_DIPSETTING(0x31, "C400-C7FF")
	PORT_DIPSETTING(0x32, "C800-CBFF")
	PORT_DIPSETTING(0x33, "CC00-CFFF")
	PORT_DIPSETTING(0x34, "D000-D3FF")
	PORT_DIPSETTING(0x35, "D400-D7FF")
	PORT_DIPSETTING(0x36, "D800-DBFF")
	PORT_DIPSETTING(0x37, "DC00-DFFF")
	PORT_DIPSETTING(0x38, "E000-E3FF")
	PORT_DIPSETTING(0x39, "E400-E7FF")
	PORT_DIPSETTING(0x3a, "E800-EBFF")
	PORT_DIPSETTING(0x3b, "EC00-EFFF")
	PORT_DIPSETTING(0x3c, "F000-F3FF")
	PORT_DIPSETTING(0x3d, "F400-F7FF")
	PORT_DIPSETTING(0x3e, "F800-FBFF")
	PORT_DIPSETTING(0x3f, "FC00-FFFF")

	PORT_START("UNUSED")
	PORT_DIPNAME(1, 1, DEF_STR(Unused)) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING(1, DEF_STR(Off))
	PORT_DIPSETTING(0, DEF_STR(On))
INPUT_PORTS_END

ioport_constructor poly_vti_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(polyvti);
}

/* F4 Character Displayer */
static const gfx_layout vti_charlayout =
{
	8, 16,                  /* text = 7 x 9 */
	128,                    /* 128 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16                    /* every char takes 16 bytes */
};

static GFXDECODE_START(gfx_vti)
	GFXDECODE_ENTRY("chargen", 0x0000, vti_charlayout, 0, 1)
GFXDECODE_END

void poly_vti_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500)); /* not accurate */
	screen.set_size(64*10, 16*15);
	screen.set_visarea(0, 64*10-1, 0, 16*15-1);
	screen.set_screen_update(FUNC(poly_vti_device::screen_update));
	screen.set_palette("palette");

	GFXDECODE(config, "gfxdecode", "palette", gfx_vti);
	PALETTE(config, "palette", palette_device::MONOCHROME);

	generic_keyboard_device &keyboard(GENERIC_KEYBOARD(config, "keyboard", 0));
	keyboard.set_keyboard_callback(FUNC(poly_vti_device::kbd_put));

	I8212(config, m_kbdlatch);
	m_kbdlatch->md_rd_callback().set_constant(0);
	m_kbdlatch->int_wr_callback().set(FUNC(poly_vti_device::kbd_int_w));
}

ROM_START(polyvti)
	ROM_REGION(0x800, "chargen", 0)
	ROM_LOAD("6571.bin", 0x0000, 0x0800, CRC(5a25144b) SHA1(7b9fee0c8ef2605b85d12b6d9fe8feb82418c63a))
ROM_END

const tiny_rom_entry *poly_vti_device::device_rom_region() const
{
	return ROM_NAME(polyvti);
}
