#include <algorithm>
#include "types.h"

u32 screen[160*144];
extern u8 OAM[0x100];
extern u8* VRAM;

u8 LCDC = 0;
u8 STAT = 3;
u8 LYC = 0xff;
u8 LY = 3;
u8 BGP = 0;
u8 OBP0 = 0;
u8 OBP1 = 0;
u8 SCX = 0;
u8 SCY = 0;

u32 dmg_palette[] = { 0xFFFFFF00, 0xC0C0C000, 0x60606000, 0x20202000 };

u8 sprites_online[10] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
u8 shifter_colors[16];
u8 shifter_src[16];
int current_dot = 0;
void gfx_find_sprites();

int end_mode_3 = 0;
int CurX = 0; // which actual LCD dot is next
int bgtileno = 0;
int bgtiled1, bgtiled2;

void gfx_dot()
{
	switch( STAT & 3 )
	{
	case 0: // HBLANK
		current_dot++;
		if( current_dot == 456 )
		{
			current_dot = 0;
			LY++;
			if( LY == 164 ) LY = 0;
			if( LY > 143 )
			{
				STAT = (STAT&~3) | 1;
			} else {
				STAT = (STAT&~3) | 2;
			}
			if( LY == LYC )
			{
				STAT |= 4;
			} else {
				STAT &= ~4;
			}
			//TODO: LYC and vblank interrupt		
		}
		return;
	case 2: // search OAM
		if( current_dot == 0 )
		{
			gfx_find_sprites();
		} else if( current_dot == 79 ) {
			STAT = (STAT&~3) | 3;
			CurX = 0;
		}
		current_dot++;
		return;
	case 1: // VBLANK
		current_dot++;
		if( current_dot == 456 )
		{
			current_dot = 0;
			LY++;
			if( LY > 163 ) 
			{
				LY = 0;
				STAT = (STAT&~3) | 2;	
			}
			if( LY == LYC )
			{
				STAT |= 4;
			} else {
				STAT &= ~4;
			}
			//TODO: LYC interrupt
		}
		return;
	case 3: // screen drawing
		if( CurX > 159 )
		{
			if( current_dot == end_mode_3 )
			{
				CurX = 0;
				STAT = (STAT&~3) | 0;
			}
			current_dot++;
			return;
		}
		break; // actual dot draw is the rest of function		
	}
	
	if( current_dot == 80 )
	{
		end_mode_3 = 80+168;
	}
	
	int scrollvalue = (SCY + (int)LY);
	
	int st = (32 * ((scrollvalue>>3)&0x1F)) + (((SCX>>3) + (CurX>>3)) & 0x1F);
	int taddr = (LCDC&4) ? 0x1C00 : 0x1800;
	taddr += st;
	int tileno = VRAM[taddr];
	
	int daddr = 0;// (LCDC&0x10) ? 0 : 0x800;
	if( !(LCDC & 0x10) )
	{
		if( tileno & 0x80 )
		{
			tileno &= 0x7f;
			daddr += 0x800;
		} else {
			daddr += 0x1000;
		}
	}
	daddr += tileno*16;

	scrollvalue &= 7;
	scrollvalue <<= 1;
	int d1 = VRAM[daddr + scrollvalue];
	int d2 = VRAM[daddr + scrollvalue + 1];
	
	int shft = 7 - ((SCX+CurX)&7);
	d1 >>= shft;
	d1 &= 1;
	d2 >>= shft;
	d2 &= 1;
	d1 = (d1<<1)|d2;
	screen[LY*160 + CurX] = dmg_palette[d1];

	CurX++;

	return;
}

void gfx_find_sprites()
{
	int sx = 0;
	int sprHeight = (LCDC&2) ? 16 : 8;
	
	for(int i = 0; i < 40 && sx < 10; ++i)
	{
		int sprY = OAM[i*4];
		sprY -= 16;
		if( sprY > 16 && LY - sprY < sprHeight )
		{
			sprites_online[sx++] = i;		
		}	
	}
	
	for(int i = sx; i < 10; ++i) sprites_online[i] = 0xff;
	
	for(int i = 0; i < sx-1; ++i)
	{
		for(int y = i; y < sx; ++y)
		{
			if( OAM[sprites_online[i]*4 + 1] > OAM[sprites_online[y]*4 + 1] )
			{
				u8 temp = sprites_online[i];
				sprites_online[i] = sprites_online[y];
				sprites_online[y] = temp;
			}	
		}
	}

	return;
}

void gfx_cycles(int c)
{
	for(int i = 0; i < c; ++i)
		gfx_dot();
	return;
}









