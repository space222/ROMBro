#include <cstdio>
#include "types.h"

extern u8* ROMfile;
u8 BIOS[0x100];
bool BIOS_On = true;

int gb_mapper = 0;
int gb_rom_banks = 0;
int gb_ram_type = 0;

u8 WRAM_b0[0x1000];
u8 WRAM_b1_builtin[0x1000];
u8* WRAM_b1 = &WRAM_b1_builtin[0];

u8 EXRAM[128*1024]; // cart ram
int exram_bank_offset = 0;

u8 VRAM_builtin[0x4000];
u8* VRAM = &VRAM_builtin[0];

u8 OAM[0x100];

u8* ROM_lo;
u8* ROM_hi;

u8 io_read8(u16);
void io_write8(u16,u8);
void mbc_write8(u16,u8);

u8 mem_read8(u16 addr)
{
	if( addr >= 0xFF00 ) return io_read8(addr);

	if( addr >= 0xFEA0 ) return 0; //unusable range
	
	if( addr >= 0xFE00 ) return OAM[addr&0xff];
	
	if( addr >= 0xC000 )
	{
		if( addr & 0x1000 ) return WRAM_b1[addr&0xfff];		
		return WRAM_b0[addr&0xfff];	
	}
	
	if( addr >= 0xA000 )
	{
		if( gb_ram_type == NO_RAM ) return 0;
		if( gb_ram_type == MBC2_RAM )
		{
			return EXRAM[addr&0x1ff] & 0xf;		
		}
		if( gb_ram_type == KB2 )
		{
			return EXRAM[addr&0x3ff];
		}
		
		return EXRAM[exram_bank_offset + (addr&0x1fff)];
	}
	
	if( addr >= 0x8000 )
	{
		return VRAM[addr&0x1fff];	
	}
	
	if( addr >= 0x4000 )
	{
		return ROM_hi[addr&0x3fff];	
	}
	
	if( addr < 0x100 && BIOS_On )
	{
		return BIOS[addr];
	}
	
	return ROM_lo[addr&0x3fff];
}

void mem_write8(u16 addr, u8 val)
{
	if( addr >= 0xFF00 )
	{
		io_write8(addr, val);
		return;
	}

	if( addr >= 0xFEA0 ) return; // unusable range

	if( addr >= 0xFE00 )
	{
		OAM[addr&0xff] = val;
		return;
	}
	
	if( addr >= 0xC000 )
	{
		if( addr & 0x1000 ) WRAM_b1[addr&0xfff] = val;
		else  		    WRAM_b0[addr&0xfff] = val;
		return;
	}
	
	if( addr >= 0xA000 )
	{
		if( gb_ram_type == NO_RAM ) return;
		if( gb_ram_type == MBC2_RAM )
		{
			EXRAM[addr&0x1ff] = val & 0xf;
		} else if( gb_ram_type == KB2 ) {
			EXRAM[addr&0x7ff] = val;
		} else {
			EXRAM[exram_bank_offset + (addr&0x1fff)] = val;
		}
	}
	
	if( addr >= 0x8000 )
	{
		VRAM[addr&0x1fff] = val;
		return;
	}
	
	mbc_write8(addr, val);

	return;
}

bool isColorGB = false;

void gb_mapping()
{
	if( ROMfile[0x143] & 0x80 )
	{
		isColorGB = true;
		printf("File is a GBC ROM.\n");
	} else {
		printf("Running DMG Mode.\n");
	}
	
	printf("ROM type: %i\n", ROMfile[0x147]);
	
	switch( ROMfile[0x147] )
	{
	case 8:
	case 9:
	case 0: gb_mapper = NO_MBC; break;
	case 1:
	case 2:
	case 3: gb_mapper = MBC1; break;
	case 5:
	case 6: gb_mapper = MBC2;  break;
	case 0xF:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13: gb_mapper = MBC3; break;
	case 0x19: 
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E: gb_mapper = MBC5; break;
	case 0x20: gb_mapper = MBC6; break;
	case 0x22: gb_mapper = MBC7; break;
	}
	
	switch( ROMfile[0x148] )
	{
	case 0: gb_rom_banks = 1; break;
	case 1: gb_rom_banks = 4; break;
	case 2: gb_rom_banks = 8; break;	
	case 3: gb_rom_banks = 16; break;	
	case 4: gb_rom_banks = 32; break;
	case 5: gb_rom_banks = 64; break;
	case 6: gb_rom_banks = 128; break;
	case 7: gb_rom_banks = 256; break;
	case 8: gb_rom_banks = 512; break;
	case 0x52: gb_rom_banks = 72; break;
	case 0x53: gb_rom_banks = 80; break;
	case 0x54: gb_rom_banks = 96; break;	
	}
	
	ROM_lo = &ROMfile[0];
	ROM_hi = &ROMfile[0x4000];
	
	printf("RAM type: %i\n", ROMfile[0x149]);
	switch( ROMfile[0x149] )
	{
	case 0: if( gb_mapper == MBC2 ) gb_ram_type = MBC2_RAM; else gb_ram_type = NO_RAM; break;
	case 1: gb_ram_type = KB2; break;
	case 2: gb_ram_type = BANKS1; break;
	case 3: gb_ram_type = BANKS4; break;
	case 4: gb_ram_type = BANKS16; break;
	case 5: gb_ram_type = BANKS8; break;	
	}
	
	return;
}

