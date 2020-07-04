#include <cstdio>
#include <SDL.h>
#include "types.h"

u8 mem_read8(u16);
void mem_write8(u16, u8);

u8 hi_ram[0x80];
extern u16 PC;

extern SDL_Joystick* controller0;

extern bool isColorGB;
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
extern u32 gbc_bg_palette[0x20];
extern u32 gbc_spr_palette[0x20];
extern u8* WRAM_b1;
extern u8 WRAM_b1_builtin[0x7000];
extern u8* VRAM;
extern u8 VRAM_builtin[0x4000];

u16 gbc_hdma_src = 0;
u16 gbc_hdma_dest = 0;
u8 gbc_hdma_ctrl = 0;
bool gbc_hdma_active = false;

u8 gbc_bg_pal[0x40];
u8 gbc_spr_pal[0x40];

u8 gbc_bgpal_index = 0;
u8 gbc_sprpal_index = 0;

u8 IF = 0;
u8 IE = 0;
u8 DIV = 0;
u8 TCOUNT = 0;
u8 TMA = 0;
u8 TCTRL = 0;
u8 JPAD = 0xc0;

u8 WRAMBank = 0;
u8 VRAMBank = 0;

u8 gbc_speed = 0;

void snd_write8(u16, u8);
u8 snd_read8(u16);

u8 io_read8(u16 addr)
{
	addr &= 0xFF;
	
	//printf("IO Read @0x%x\n", addr);
	
	if( addr > 0x7F && addr != 0xFF )
	{
		return hi_ram[addr&0x7f];
	}
	
	if( addr >= 0x10 && addr < 0x40 )
	{
		return snd_read8(addr);
	}

	switch( addr )
	{
	case 0x00: 
		if( JPAD & 0x10 )
		{
			u8 K = 0;
			if( !controller0 )
			{
				K = (keys[SDL_SCANCODE_S]<<3)|(keys[SDL_SCANCODE_A]<<2) 
					| (keys[SDL_SCANCODE_X]<<1) | keys[SDL_SCANCODE_Z];	
			} else {
				K = (SDL_JoystickGetButton(controller0, 0)<<3)|(SDL_JoystickGetButton(controller0, 1)<<2)
					|(SDL_JoystickGetButton(controller0, 2)<<1)|SDL_JoystickGetButton(controller0, 3);
			}
			K ^= 0xF;
			return JPAD|K;
		} else if( JPAD & 0x20 ) {
			u8 K = 0;
			if( !controller0 )
			{
				K = (keys[SDL_SCANCODE_DOWN]<<3)|(keys[SDL_SCANCODE_UP]<<2)
					| (keys[SDL_SCANCODE_LEFT]<<1)|keys[SDL_SCANCODE_RIGHT];
			} else {
				K = (SDL_JoystickGetButton(controller0, 4)<<3)|(SDL_JoystickGetButton(controller0, 5)<<2)
					|(SDL_JoystickGetButton(controller0, 6)<<1)|SDL_JoystickGetButton(controller0, 7);
			}
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
	
	if( isColorGB )
	{
		switch( addr )
		{
		case 0x4D:
			printf("Program read gbc speed status.\n");
			return gbc_speed<<7;
		case 0x4F: return VRAMBank;
		
		case 0x51: return gbc_hdma_src>>8;
		case 0x52: return gbc_hdma_src;
		case 0x53: return gbc_hdma_dest>>8;
		case 0x54: return gbc_hdma_dest;
		case 0x55: return gbc_hdma_ctrl;
		
		case 0x68: return gbc_bgpal_index;
		case 0x69: return gbc_bg_pal[gbc_bgpal_index&0x1f];
		case 0x6A: return gbc_sprpal_index;
		case 0x6B: return gbc_spr_pal[gbc_sprpal_index&0x1f];
		
		case 0x70: return WRAMBank;
		}
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
	
	if( addr >= 0x10 && addr < 0x40 )
	{
		snd_write8(addr, val);
		return;
	}

	printf("IO Write $%x = $%x\n", addr, val);

	switch( addr )
	{
	case 0x00: JPAD = val&0x30; return;
	case 0x01: serial_data = val; return;
	case 0x02: serial_ctrl = val; if( serial_ctrl == 0x81 ) { putchar(serial_data); fflush(nullptr); } return;

	case 0x04: DIV = 0; return;
	case 0x05: TCOUNT = val; return;
	case 0x06: TMA = val; return;
	case 0x07: TCTRL = val&7; return;

	case 0x0F: IF = val&0x1F; return;

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

	case 0xFF: IE = val&0x1F; return;
	default: break;
	}
	
	if( isColorGB )
	{
		switch( addr )
		{
		case 0x4D:
			printf("Program attempted gbc speed switch.\n");	
			if( val & 1 ) gbc_speed ^= 1;
			return;
		case 0x4F: 
			val &= 1;
			VRAMBank = 0xFE | val;
			VRAM = &VRAM_builtin[0x2000 * val];
			return;
			
		case 0x51: gbc_hdma_src &= 0xff; gbc_hdma_src |= val << 8; break;
		case 0x52: gbc_hdma_src &=~0xff; gbc_hdma_src |= val & 0xf0; break;
		case 0x53: gbc_hdma_dest&= 0xff; gbc_hdma_dest|= (val&0x1f) << 8; break;
		case 0x54: gbc_hdma_dest&=~0xff; gbc_hdma_dest|= val & 0xf0; break;
		case 0x55:
			if( gbc_hdma_active && !(val & 0x80) )
			{
				gbc_hdma_active = false;
				gbc_hdma_ctrl &= 0x7f;
				return;
			}
			gbc_hdma_ctrl = val;
			if( !(val & 0x80) && gbc_hdma_src )
			{
				//printf("DMA! from %x to %x, %i bytes!\n", gbc_hdma_src, gbc_hdma_dest, (val + 1) * 0x10);
				int num = val + 1;
				num *= 0x10;
				for(int i = 0; i < num; ++i)
				{
					mem_write8(0x8000 + gbc_hdma_dest++, mem_read8(gbc_hdma_src++));				
				}
				gbc_hdma_ctrl = 0xff;
			} else {
				gbc_hdma_active = true;
			}
			break;
			
		case 0x68: gbc_bgpal_index = val; return;
		case 0x69:{
			gbc_bg_pal[gbc_bgpal_index&0x3F] = val;
			u32 pi = (gbc_bgpal_index&0x3F)>>1;
			u16 r1 = gbc_bg_pal[pi<<1] | (gbc_bg_pal[(pi<<1) + 1]<<8);
			gbc_bg_palette[pi] = ((r1&0x1F)<<27)|(((r1>>5)&0x1F)<<19)|(((r1>>10)&0x1F)<<11);
			if( gbc_bgpal_index & 0x80 )
			{
				gbc_bgpal_index++;
				gbc_bgpal_index &= 0xBF;
			}
			}return;		
		case 0x6A: gbc_sprpal_index = val; return;
		case 0x6B:{
			gbc_spr_pal[gbc_sprpal_index&0x3F] = val;
			u32 pi = (gbc_sprpal_index&0x3F)>>1;
			u16 r1 = gbc_spr_pal[pi<<1] | (gbc_spr_pal[(pi<<1) + 1]<<8);
			gbc_spr_palette[pi] = ((r1&0x1F)<<27)|(((r1>>5)&0x1F)<<19)|(((r1>>10)&0x1F)<<11);
			if( gbc_sprpal_index & 0x80 )
			{
				gbc_sprpal_index++;
				gbc_sprpal_index &= 0xBF;
			}
			}return;
			
		case 0x70:
			val &= 7;
			WRAMBank = val;
			if( val == 0 ) val = 1;
			val--;
			WRAM_b1 = &WRAM_b1_builtin[val*4096];
			return;
		}	
	}

	return;
}




