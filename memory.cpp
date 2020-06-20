#include <cstdio>
#include <string>
#include "types.h"

extern u8* ROMfile;
u8 BIOS[0x100];
bool BIOS_On = true;

extern u8 STAT;
extern u8 LCDC;

int gb_mapper = 0;
int gb_rom_banks = 0;
int gb_ram_type = 0;

u8 WRAM_b0[0x1000];
u8 WRAM_b1_builtin[0x7000];
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

bool rtc_regs_active = false;

u8 mem_read8(u16 addr)
{
	if( addr >= 0xFF00 ) return io_read8(addr & 0xFF);

	if( addr >= 0xFEA0 ) return 0; //unusable range
	
	if( addr >= 0xFE00 ) 
	{
		if( 1 /*(STAT & 3) < 2 || !(LCDC&0x80)*/ ) return OAM[addr&0xff];
		//return 0xff;
	}
	
	if( addr >= 0xC000 )
	{
		if( addr & 0x1000 ) return WRAM_b1[addr&0xfff];		
		return WRAM_b0[addr&0xfff];	
	}
	
	if( addr >= 0xA000 )
	{
		if( rtc_regs_active ) return 0;
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
		//if( ((STAT & 3) == 3) && (LCDC & 0x80) ) return 0xff;
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
		io_write8(addr & 0xFF, val);
		return;
	}

	if( addr >= 0xFEA0 ) return; // unusable range

	if( addr >= 0xFE00 )
	{
		if( 1 /* ((STAT & 3) > 1) || !(LCDC&0x80) */ ) OAM[addr&0xff] = val;
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
		if( rtc_regs_active ) return;
		if( gb_ram_type == NO_RAM ) return;
		if( gb_ram_type == MBC2_RAM )
		{
			EXRAM[addr&0x1ff] = val & 0xf;
		} else if( gb_ram_type == KB2 ) {
			EXRAM[addr&0x7ff] = val;
		} else {
			EXRAM[exram_bank_offset + (addr&0x1fff)] = val;
		}
		return;
	}
	
	if( addr >= 0x8000 )
	{
		//if( (STAT & 3) == 3 && (LCDC & 0x80) ) return;
		VRAM[addr&0x1fff] = val;
		return;
	}
	
	mbc_write8(addr, val);

	return;
}

bool hasRTC = false;
bool isColorGB = false;
extern bool BIOS_On;
extern bool emubios;
extern std::string romname;

std::string savfile;

void gb_mapping()
{
	if( ROMfile[0x143] & 0x80 )
	{
		isColorGB = true;
		emubios = BIOS_On = false;
		printf("File is a GBC ROM.\n");
	} else {
		printf("Running DMG Mode.\n");
	}
	
	printf("ROM type: 0x%x\n", ROMfile[0x147]);
	
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
	case 0xF:  hasRTC = true;
	case 0x10: hasRTC = true;
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
	
	printf("RAM type: 0x%x\n", ROMfile[0x149]);
	switch( ROMfile[0x149] )
	{
	case 0: if( gb_mapper == MBC2 ) gb_ram_type = MBC2_RAM; else gb_ram_type = NO_RAM; break;
	case 1: gb_ram_type = KB2; break;
	case 2: gb_ram_type = BANKS1; break;
	case 3: gb_ram_type = BANKS4; break;
	case 4: gb_ram_type = BANKS16; break;
	case 5: gb_ram_type = BANKS8; break;	
	}
	
	savfile = romname;
	auto pos = romname.rfind('.');
	if( pos != std::string::npos )
	{
		savfile = savfile.substr(0, pos);
	}
	savfile += ".sav";

	FILE* fp = fopen(savfile.c_str(), "rb");
	if( ! fp ) return;
	int unu = 0;
	
	switch( gb_ram_type )
	{
	case MBC2_RAM: unu = fread(EXRAM, 1, 512, fp); break;
	case KB2: unu = fread(EXRAM, 1, 2*1024, fp); break;
	case BANKS1: unu = fread(EXRAM, 1, 8*1024, fp); break;
	case BANKS4: unu = fread(EXRAM, 1, 4*8*1024, fp); break;
	case BANKS8: unu = fread(EXRAM, 1, 8*8*1024, fp); break;
	case BANKS16:unu= fread(EXRAM, 1, 16*8*1024, fp); break;
	}
	
	fclose(fp);	
	return;
}

void mem_save()
{
	if( gb_ram_type == NO_RAM ) return;
	
	FILE* fp = fopen(savfile.c_str(), "wb");
	int unu = 0;
	
	switch( gb_ram_type )
	{
	case MBC2_RAM: unu = fwrite(EXRAM, 1, 512, fp); break;
	case KB2: unu = fwrite(EXRAM, 1, 2*1024, fp); break;
	case BANKS1: unu = fwrite(EXRAM, 1, 8*1024, fp); break;
	case BANKS4: unu = fwrite(EXRAM, 1, 4*8*1024, fp); break;
	case BANKS8: unu = fwrite(EXRAM, 1, 8*8*1024, fp); break;
	case BANKS16:unu= fwrite(EXRAM, 1, 16*8*1024, fp); break;
	}

	fclose(fp);
	return;
}








