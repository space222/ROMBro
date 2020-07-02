#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "types.h"


// channels are 1 to 4 in the docs. I've made the arrays 5, with index zero going unused
// just for ease of writting.
extern SDL_AudioDeviceID AudioDev;

int c1_sweep = 0;

int chan_enabled[5] = {0};
u32 chan_length[5] = {0};
u32 chan_envelope[5] = {0};

u16 noise_lfsr = 0;

u32 timer[5] = {0};
u32 timer_reload[5] = {0};

u8 NR[55];
u8 WAVRAM[0x10];

const int CPUFreq = 4194304;
const int FrameHz = 512;
const int frame_ctr_reload = CPUFreq/FrameHz;
int frame_counter = frame_ctr_reload;
int frame_seq = 7;

int duty_pos[5] = {0};
u8 duty[] = { 0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0xF0,0xF0,0xF0, 0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0 };

void noise_run();

void length_clock()
{
	for(int i = 1; i < 5; ++i)
	{
		if( chan_enabled[i] && ( NR[0x10 * i + 4] & 0x40 ) )
		{
			chan_length[i]--;
			if( chan_length[i] == 0 )
				chan_enabled[i] = 0;
		}
	}
	
	return;
}

u32 sweep_shadow = 0;
u32 sweep_timer = 0;
u32 sweep_on = 0;

void sweep_clock()
{
	if( !sweep_on || !(NR[10]>>4) ) return;
	
	NR[10] = (NR[10]&~0x70) | (NR[10] - 0x10);
	
	u32 temp = sweep_shadow >> (NR[10] & 7);
	if( NR[10] & 8 ) temp = -temp;
	sweep_shadow += temp;
	
	if( sweep_shadow > 2047 )
	{
		chan_enabled[1] = 0;
	} else {
		NR[13] = sweep_shadow & 0xff;
		NR[14] &= ~7;
		NR[14] |= (sweep_shadow>>8)&7;
		
		timer_reload[1] = ((2048-sweep_shadow)*4);
	}
	return;
}

void envelope_clock()
{
	if( chan_envelope[1] )
	{
		chan_envelope[1]--;
		u8 vol = NR[12]>>4;
		if( NR[12] & 8 )
		{
			if( vol < 15 ) vol++;
		} else {
			if( vol > 0 ) vol--;
		}
		NR[12] &= 0xF; NR[12] |= vol<<4;
	}
	
	if( chan_envelope[2] )
	{
		chan_envelope[2]--;
		u8 vol = NR[22]>>4;
		if( NR[22] & 8 )
		{
			if( vol < 15 ) vol++;
		} else {
			if( vol > 0 ) vol--;
		}
		NR[22] &= 0xF; NR[22] |= vol<<4;
	}
	
	if( chan_envelope[4] )
	{
		chan_envelope[4]--;
		u8 vol = NR[42]>>4;
		if( NR[42] & 8 )
		{
			if( vol < 15 ) vol++;
		} else {
			if( vol > 0 ) vol--;
		}
		NR[42] &= 0xF; NR[42] |= vol<<4;
	}
	
	return;
}

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

float left_buffer[2048] = {0};
u16 right_buffer[2048] = {0};
u16 left_write_pos = 0;
u16 right_write_pos = 0;
u16 left_read_pos = 0;
u16 right_read_pos = 0;

u32 chan_accum[5] = {0};

u32 left_accum = 0;
u32 right_accum = 0;
u32 accum_counter = 0;
u32 accum_max = 95;

void snd_cycle()
{
	frame_counter--;
	if( frame_counter == 0 )
	{
		frame_counter = frame_ctr_reload;
		frame_clock();
	}

	if( chan_enabled[1] )
	{
		timer[1]--;
		if( timer[1] == 0 )
		{
			timer[1] = timer_reload[1];
			duty_pos[1] = (duty_pos[1]+1) & 7;
		}
		
		chan_accum[1] += (duty[(NR[11]>>6)*8 + duty_pos[1]] & NR[12]);
	}
	
	if( chan_enabled[2] )
	{
		timer[2]--;
		if( timer[2] == 0 )
		{
			timer[2] = timer_reload[2];
			duty_pos[2] = (duty_pos[2]+1) & 7;
		}
		
		chan_accum[2] += (duty[(NR[21]>>6)*8 + duty_pos[2]] & NR[22]);
	}
	
	if( chan_enabled[3] )
	{
		timer[3]--;
		if( timer[3] == 0 )
		{
			timer[3] = timer_reload[3];
			duty_pos[3] = (duty_pos[3]+1) & 0xf;
		}
		
		u32 temp = (u32)(u8)WAVRAM[duty_pos[3]>>1];
		if( duty_pos[3] & 1 ) temp <<= 4; else temp &= 0xF0;
		chan_accum[3] += (temp & 0xF0);
	}
	
	if( chan_enabled[4] )
	{
		timer[4]--;
		if( timer[4] == 0 )
		{
			timer[4] = timer_reload[4];
			noise_run();
		}
		
		chan_accum[4] += (noise_lfsr & 1) ? 0 : (NR[42] & 0xF0);
	}
	
	accum_counter++;
	if( accum_counter == accum_max )
	{
		accum_counter = 0;
		for(int i = 1; i < 5; ++i) chan_accum[i] /= accum_max;
		left_buffer[left_write_pos++] = ((chan_accum[1]+chan_accum[2]+chan_accum[3]+chan_accum[4])/4.0f)/255.0f;
		for(int i = 1; i < 5; ++i) chan_accum[i] = 0;
		
		if( left_write_pos == 1024 )
		{
			left_write_pos = 0;
			if( SDL_QueueAudio(AudioDev, &left_buffer[0], 4096) )
			{
				printf("Audio error\n");
				exit(1);
			}
		}
	}

	return;
}

void snd_cycles(int c)
{
	for(int i = 0; i < c; ++i)
		snd_cycle();
	return;
}

void snd_callback(void*, u8* stream, int len)
{
	int floatlen = len / 4 ;
	float* flstr =(float*) stream;
	//for(int i = 0; i < len; ++i) stream[i] = 0;
	for(int i = 0; i < floatlen; ++i)
	{
		u16 L = left_buffer[left_read_pos++]; left_read_pos &= 0x7ff;
		flstr[i] = L/255.0f;
	}

	return;
}

void noise_run()
{
	u32 nv = ((noise_lfsr>>1) ^ (noise_lfsr)) & 1;
	noise_lfsr >>= 1;
	noise_lfsr |= nv<<15;
	if( NR[43] & 8 )
	{
		noise_lfsr &= ~(1<<6);
		noise_lfsr |= (nv<<6);
	}
	return;
}

void chan1_trigger()
{
	chan_enabled[1] = 1;
	if( chan_length[1] == 0 ) chan_length[1] = 64;
	sweep_shadow = timer_reload[1] = ((NR[14]&7) << 8) | NR[13];
	
	timer[1] = timer_reload[1] = ((2048-timer_reload[1])*4);
	chan_envelope[1] = (NR[12]&7) ? (NR[12]&7) : 8;
	
	if( NR[10] & 0x77 )
	{
		sweep_on = 1;
	} else {
		sweep_on = 0;
	}
	
	return;
}

void chan2_trigger()
{
	chan_enabled[2] = 1;	
	if( chan_length[2] == 0 ) chan_length[2] = 64;
	timer_reload[2] = ((NR[24]&7) << 8) | NR[23];
	timer[2] = timer_reload[2] = ((2048-timer_reload[2])*4);
	chan_envelope[2] = (NR[22]&7) ? (NR[22]&7) : 8;
	return;
}

void chan3_trigger()
{
	chan_enabled[3] = 1;
	if( chan_length[3] == 0 ) chan_length[3] = 256;
	duty_pos[3] = 0;
	timer_reload[3] = ((NR[34]&7) << 8) | NR[33];
	timer[3] = timer_reload[3] = ((2048-timer_reload[3])*2);
	return;
}

void chan4_trigger()
{
	if( !(NR[30] & 0x80) ) return;
	
	chan_enabled[4] = 1;
	if( chan_length[4] == 0 ) chan_length[4] = 64;
	chan_envelope[4] = (NR[42]&7) ? (NR[42]&7) : 8;
	
	timer_reload[4] = (NR[43]&7) ? (NR[43]&7)*16 : 8;
	timer_reload[4] <<= (NR[43]>>4); //+1?  // ??, I found a doc that the above is the actual number of 4MHz clocks to use as period,
					 // but no actual idea what to do with the shift.
	timer[4] = timer_reload[4];
	noise_lfsr = 0x7FFF;
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
	
	if( addr == 0x26 )
	{
		NR[52] = val;
		if( !(NR[52] & 0x80) )
		{
			for(int i = 1; i < 5; ++i) chan_enabled[i] = 0;
		}
		return;
	}
	
	if( ! (NR[52] & 0x80) ) return;
	
	switch( addr )
	{
	case 0x10: NR[10] = val; break;
	case 0x11: NR[11] = val; chan_length[1] = 64-(val&0x3f); break;
	case 0x12: NR[12] = val; break;
	case 0x13: NR[13] = val; break;
	case 0x14: NR[14] = val; if( val & 0x80 ) chan1_trigger(); break;
	
	case 0x16: NR[21] = val; chan_length[2] = 64-(val&0x3f); break;
	case 0x17: NR[22] = val; break;
	case 0x18: NR[23] = val; break;
	case 0x19: NR[24] = val; if( val & 0x80 ) chan2_trigger(); break;
	
	case 0x1A: NR[30] = val; break;
	case 0x1B: NR[31] = val; chan_length[3] = 256-val; break;
	case 0x1C: NR[32] = val; break;
	case 0x1D: NR[33] = val; break;
	case 0x1E: NR[34] = val; if( val & 0x80 ) chan3_trigger(); break;
	
	case 0x20: NR[41] = val; chan_length[4] = 64-(val&0x3f); break;
	case 0x21: NR[42] = val; break;
	case 0x22: NR[43] = val; break;
	case 0x23: NR[44] = val; if( val & 0x80 ) chan4_trigger(); break;
	
	case 0x24: NR[50] = val; break;
	case 0x25: NR[51] = val; break;
	//case 0x26: NR[52] = val; break;
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
	case 0x26: return (NR[52] & 0x80) | (chan_enabled[4]<<3) | (chan_enabled[3]<<2) | (chan_enabled[2]<<1) | chan_enabled[1];
	default: break;
	}

	return 0xff;
}











