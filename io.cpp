#include <cstdio>
#include "types.h"

u8 hi_ram[0x80];

extern u8 LY;
extern u8 STAT;
extern u8 SCX;
extern u8 SCY;
extern u8 LCDC;

u8 io_read8(u16 addr)
{
	addr &= 0xFF;
	
	//if( addr != 0x44 ) printf("IO Read @0x%x\n", addr);
	
	if( addr > 0x7F && addr != 0xFF )
	{
		return hi_ram[addr&0x7f];
	}

	switch( addr )
	{
	case 0x40: return LCDC;
	case 0x41: return STAT;
	case 0x42: return SCY;
	case 0x43: return SCX;
	case 0x44: return LY;
	default: break;
	}
	return 0;
}


u8 serial_data, serial_ctrl;

void io_write8(u16 addr, u8 val)
{
	addr &= 0xFF;

	if( addr > 0x7F && addr != 0xFF )
	{
		hi_ram[addr&0x7f] = val;
		return;
	}

	//printf("IO write @$%x = $%x\n", addr, val);

	switch( addr )
	{
	case 0x01: serial_data = val; break;
	case 0x02: serial_ctrl = val; if( serial_ctrl == 0x81 ) { putchar(serial_data); fflush(nullptr); } break;

	case 0x40: LCDC = val; return;
	case 0x41: STAT = (STAT&7) | (val&~7); return;
	case 0x42: SCY = val; return;
	case 0x43: SCX = val; return;
	}	

	return;
}




