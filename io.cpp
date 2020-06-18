#include <cstdio>
#include <SDL.h>
#include "types.h"

u8 mem_read8(u16);

u8 hi_ram[0x80];
extern u16 PC;

extern bool BIOS_On;
extern u8 LY;
extern u8 LYC;
extern u8 STAT;
extern u8 SCX;
extern u8 SCY;
extern u8 WY;
extern u8 WX;
extern u8 LCDC;
extern u8 BGP;
extern u8 OBP0;
extern u8 OBP1;
extern u8* keys;
extern u8 OAM[0x100];

u8 IF = 0;
u8 IE = 0;
u8 DIV = 0;
u8 TCOUNT = 0;
u8 TMA = 0;
u8 TCTRL = 0;
u8 JPAD = 0xc0;

u8 io_read8(u16 addr)
{
	addr &= 0xFF;
	
	//printf("IO Read @0x%x\n", addr);
	
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
		
	case 0x04: return DIV;
	case 0x05: return TCOUNT;
	case 0x06: return TMA;
	case 0x07: return TCTRL;
		
	case 0x0F: return IF;
	case 0x40: return LCDC;
	case 0x41: return STAT;
	case 0x42: return SCY;
	case 0x43: return SCX;
	case 0x44: return LY;
	case 0x45: return LYC;
	case 0x46: return 0; // read DMA value?
	case 0x47: return BGP;
	case 0x48: return OBP0;
	case 0x49: return OBP1;
	case 0x4A: return WY;
	case 0x4B: return WX;
	
	//case 0x50: return 0xFE | BIOS_On;
	
	case 0xFF: return IE;
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

	case 0x04: DIV = 0; break;
	case 0x05: TCOUNT = val; break;
	case 0x06: TMA = val; break;
	case 0x07: TCTRL = val&7; break;

	case 0x0F: IF = val&0x1F; break;

	case 0x40: LCDC = val; return;
	case 0x41: STAT = (STAT&7) | (val&~7); /*printf("STAT set to %x\n", val);*/ return;
	case 0x42: SCY = val; return;
	case 0x43: SCX = val; return;
	
	case 0x45: LYC = val; return;
	case 0x46: {
		u16 base = val << 8;
		for(int i = 0; i < 0xA0; ++i) OAM[i] = mem_read8(base++);	
		} return;
	case 0x47: BGP = val; return;
	case 0x48: OBP0 = val; return;
	case 0x49: OBP1 = val; return;
	case 0x4A: WY = val; return;
	case 0x4B: WX = val; return;
	
	case 0x50:
		if( BIOS_On && (val&1) )
		{
			printf("BIOS Disabled at PC=%x\n", PC);
			BIOS_On = false;
		}
		return;
		
	case 0xFF: IE = val&0x1F; /*printf("IE set to %x\n", IE);*/ break;
	}	

	return;
}




