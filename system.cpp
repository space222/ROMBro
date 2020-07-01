#include <cstdio>
#include <cstdlib>
#include "types.h"

void gfx_cycles(int);
void snd_cycles(int);

void push16(u16);

extern bool IME;
extern bool Halted;
extern u16 PC;
extern u8 IE;
extern u8 IF;

extern u8 DIV;
extern u8 TCTRL;
extern u8 TCOUNT;
extern u8 TMA;

int tcycle_counts[] = { 1024, 16, 64, 256 };

int cycles_until_div = 0;
int timer_cycle_count = 0;

int total_cycles = 0;

void system_cycles(int c)
{
	total_cycles += c;
	gfx_cycles(c);
	snd_cycles(c);
	
	cycles_until_div += c;
	if( cycles_until_div >= 256 )
	{
		cycles_until_div -= 256;
		DIV++;
	}
	
	if( TCTRL & 4 )
	{
		timer_cycle_count += c;
		if( timer_cycle_count >= tcycle_counts[TCTRL&3] )
		{
			timer_cycle_count -= tcycle_counts[TCTRL&3];
			TCOUNT++;
			if( TCOUNT == 0 )
			{
				TCOUNT = TMA;
				IF |= 4;
			}
		}
	}
	
	return;
}

