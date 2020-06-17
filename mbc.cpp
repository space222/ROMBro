#include <cstdio>
#include <cstdlib>
#include "types.h"

extern int gb_mapper;

int bank_num = 0;

extern u8* ROMfile;
extern u8* ROM_hi;

void mbc_write8(u16 addr, u8 val)
{
	if( gb_mapper == MBC1 )
	{
		if( addr >= 0x2000 && addr < 0x4000 )
		{
			if( val == 0 ) val = 1;
			bank_num &= ~0x1F;
			bank_num |= val & 0x1F;
		} else if( addr >= 0x4000 && addr < 0x6000 ) {
			val &= 3;
			bank_num &= ~0x60;
			bank_num |= val << 5;
		}
	
		ROM_hi = &ROMfile[bank_num*16*1024];
		return;		
	}

	if( gb_mapper == MBC5 )
	{
		if( addr >= 0x2000 && addr < 0x3000 )
		{
			bank_num &= ~0xFF;
			bank_num |= val;
		} else if( addr >= 0x3000 && addr < 0x4000 ) {
			val &= 1;
			bank_num &= ~0x100;
			bank_num |= val << 8;
		}
	
		ROM_hi = &ROMfile[bank_num*16*1024];		
	}

	return;
}


