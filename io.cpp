#include <cstdio>
#include "types.h"

u8 hi_ram[0x80];

u8 io_read8(u16 addr)
{
	addr &= 0xFF;
	
	if( addr > 0x7F && addr != 0xFF )
	{
		return hi_ram[addr&0x7f];
	}


	//printf("IO read @$%x\n", addr);

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
	
	}	

	return;
}




