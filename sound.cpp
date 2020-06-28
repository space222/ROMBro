#include "types.h"

int duty_pos[4] = {0};

int c1_sweep = 0;

int c1_env = 0;
int c2_env = 0;

int timer[4] = {0};
int timer_reload[4] = {0};

u8 NR[55];
u8 WAVRAM[0x10];

const int CPUFreq = 4194304;
const int FrameHz = 512;
const int frame_ctr_reload = CPUFreq/FrameHz;
int frame_counter = frame_ctr_reload;
int frame_seq = 7;

void frame_clock()
{
	frame_seq = (frame_seq+1) & 7;

	switch( frame_seq )
	{
	case 1:
	case 3:
	case 5:
		break; // steps 1, 3, and 5 don't do anything
	case 4:
	case 0: 
		length_clock();
		break;
	case 6:
	case 2:
		length_clock();
		sweep_clock();
		break;
	case 7:
		envelope_clock();
		break;
	}

	return;
}

void snd_cycle()
{
	frame_counter--;
	if( frame_counter == 0 )
	{
		frame_counter = frame_ctr_reload;
		frame_clock();
	}



	return;
}


void snd_cycles(int c)
{
	for(int i = 0; i < c; ++i)
		snd_cycle();
	return;
}

void snd_callback(void* userdata, u8* stream, int len)
{
	for(int i = 0; i < len; ++i) stream[i] = 0;


	return;
}

void snd_write8(u16 addr, u8 val)
{
	addr &= 0xff;
	
	if( addr >= 0x30 && addr < 0x40 )
	{
		WAVRAM[addr&0xf] = val;
		return;
	}
	
	switch( addr )
	{
	case 0x10: NR[10] = val; break;
	case 0x11: NR[11] = val; break;
	case 0x12: NR[12] = val; break;
	case 0x13: NR[13] = val; break;
	case 0x14: NR[14] = val; break;
	
	case 0x16: NR[21] = val; break;
	case 0x17: NR[22] = val; break;
	case 0x18: NR[23] = val; break;
	case 0x19: NR[24] = val; break;
	
	case 0x1A: NR[30] = val; break;
	case 0x1B: NR[31] = val; break;
	case 0x1C: NR[32] = val; break;
	case 0x1D: NR[33] = val; break;
	case 0x1E: NR[34] = val; break;
	
	case 0x20: NR[41] = val; break;
	case 0x21: NR[42] = val; break;
	case 0x22: NR[43] = val; break;
	case 0x23: NR[44] = val; break;
	
	case 0x24: NR[50] = val; break;
	case 0x25: NR[51] = val; break;
	case 0x26: NR[52] = val; break;
	default: break;
	}
	return;
}

u8 snd_read8(u16 addr)
{
	addr &= 0xff;
	
	if( addr >= 0x30 && addr < 0x40 )
	{
		return WAVRAM[addr&0xf];
	}
	
	switch( addr )
	{
	case 0x10: return NR[10];
	case 0x11: return NR[11];
	case 0x12: return NR[12];
	case 0x13: return NR[13];
	case 0x14: return NR[14];
	
	case 0x16: return NR[21];
	case 0x17: return NR[22];
	case 0x18: return NR[23];
	case 0x19: return NR[24];
	
	case 0x1A: return NR[30];
	case 0x1B: return NR[31];
	case 0x1C: return NR[32];
	case 0x1D: return NR[33];
	case 0x1E: return NR[34];
	
	case 0x20: return NR[41];
	case 0x21: return NR[42];
	case 0x22: return NR[43];
	case 0x23: return NR[44];
	
	case 0x24: return NR[50];
	case 0x25: return NR[51];
	case 0x26: return NR[52];
	default: break;
	}

	return 0;
}











