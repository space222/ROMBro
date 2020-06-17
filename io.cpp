#include <cstdio>
#include <SDL.h>
#include "types.h"

u8 hi_ram[0x80];
extern u16 PC;

extern bool BIOS_On;
extern u8 LY;
extern u8 LYC;
extern u8 STAT;
extern u8 SCX;
extern u8 SCY;
extern u8 LCDC;
extern u8 BGP;
extern u8* keys;

u8 JPAD = 0xc0;

u8 io_read8(u16 addr)
{
	addr &= 0xFF;
	
	if( addr == 0 ) printf("IO Read @0x%x\n", addr);
	
	if( addr > 0x7F && addr != 0xFF )
	{
		return hi_ram[addr&0x7f];
	}

	switch( addr )
	{
	case 0x00: 
		if( JPAD & 0x10 )
		{
			u8 K = (keys[SDL_SCANCODE_S]<<3)|(keys[SDL_SCANCODE_A]<<2) 
				| (keys[SDL_SCANCODE_X]<<1) | keys[SDL_SCANCODE_Z];
			K ^= 0xF;
			return JPAD|K;
		} else if( JPAD & 0x20 ) {
			u8 K = (keys[SDL_SCANCODE_DOWN]<<3)|(keys[SDL_SCANCODE_UP]<<2)
				| (keys[SDL_SCANCODE_LEFT]<<1)|keys[SDL_SCANCODE_RIGHT];
			K ^= 0xF;
			return JPAD|K;
		}
		return 0xC0;
	case 0x40: return LCDC;
	case 0x41: return STAT;
	case 0x42: return SCY;
	case 0x43: return SCX;
	case 0x44: return LY;
	case 0x45: return LYC;
	
	case 0x47: return BGP;
	//case 0x50: return 0xFE | BIOS_On;
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
	case 0x00: JPAD = val&0x30; break;
	case 0x01: serial_data = val; break;
	case 0x02: serial_ctrl = val; if( serial_ctrl == 0x81 ) { putchar(serial_data); fflush(nullptr); } break;

	case 0x40: LCDC = val; return;
	case 0x41: STAT = (STAT&7) | (val&~7); return;
	case 0x42: SCY = val; return;
	case 0x43: SCX = val; return;
	
	case 0x45: LYC = val; return;
	
	case 0x47: BGP = val; return;
	case 0x50:
		if( BIOS_On && (val&1) )
		{
			printf("BIOS Disabled at PC=%x\n", PC);
			BIOS_On = false;
		}
		return;
	}	

	return;
}




