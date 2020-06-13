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
			bank_num |= val << 4;
		}
	
		ROM_hi = &ROMfile[bank_num*8*1024];		
	}



	return;
}


