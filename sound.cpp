#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "types.h"

// channels are 1 to 4 in the docs. I've made the arrays 5, with index zero going unused
// just for ease of writting.
extern SDL_AudioDeviceID AudioDev;

#define FILL 0,0,0,0,0,0,0,0,0,0,0,0,0

struct SoundChannel
{
	int id;
	int enabled;
	int length;
	int length_enabled;
	int timer;
	int timer_reload;
	int duty_pos;
	int envelope_period;
	int env_dir;
	int env_timer;
	int volume;
	int sweep_period;
	int sweep_timer;
	int sweep_shadow;
} channels[] = { {0, FILL}, {1, FILL}, {2, FILL}, {3, FILL}, {4, FILL}, {5, FILL} };

u16 noise_lfsr = 0;
u8 NR[55];
u8 WAVRAM[0x10];

const int CPUFreq = 4194304;
const int FrameHz = 512;
const int frame_ctr_reload = CPUFreq/FrameHz;
int frame_counter = frame_ctr_reload;
int frame_seq = 7;

u8 duty[] = { 0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0,0,0xF0, 0xF0,0,0,0,0,0xF0,0xF0,0xF0, 0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0 };

void noise_run();

void length_clock()
{
	for(int i = 1; i < 5; ++i)
	{
		if( channels[i].enabled && channels[i].length_enabled )
		{
			channels[i].length--;
			if( channels[i].length == 0 )
				channels[i].enabled = 0;
		}
	}
	
	return;
}

u16 sweep_shadow = 0;
u32 sweep_timer = 0;
u32 sweep_on = 0;

void sweep_clock()
{
	if( !sweep_on ) return;
	
	channels[1].sweep_timer--;
	if( channels[1].sweep_timer > 0 ) return;
	channels[1].sweep_timer = channels[1].sweep_period;
	
	s16 temp = channels[1].sweep_shadow >> ((NR[10] & 7)); // +1?
	if( NR[10] & 8 ) temp = -temp;
	channels[1].sweep_shadow += temp;
	
	if( channels[1].sweep_shadow > 2047 ) // || channels[1].sweep_shadow < 0 ) ?
	{
		channels[1].enabled = 0;
	} else {
		NR[13] = channels[1].sweep_shadow & 0xff;
		NR[14] &= ~7;
		NR[14] |= (channels[1].sweep_shadow>>8)&7;
		
		channels[1].timer_reload = ((2048-channels[1].sweep_shadow)*4);
	}
	return;
}

void envelope_clock()
{
	for(int i = 1; i < 5; ++i)
	{
		if( i == 3 ) continue;
		
		channels[i].env_timer--;
		if( channels[i].env_timer == 0 )
		{
			channels[i].env_timer = channels[i].envelope_period;
			if( channels[i].env_dir )
			{
				if( channels[i].volume < 15 ) channels[i].volume++;
			} else {
				if( channels[i].volume > 0 ) channels[i].volume--;
			}
		}
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
	
	for(int i = 1; i < 5; ++i)
	{
		SoundChannel& chan = channels[i];
		if( !chan.enabled ) continue;

		chan.timer--;
		if( chan.timer == 0 )
		{
			chan.timer = chan.timer_reload;
			if( chan.id == 3 )
			{
				chan.duty_pos = (chan.duty_pos+1) & 0x1F;
			} else if( chan.id == 4 ) {
				noise_run();
			} else {
				chan.duty_pos = (chan.duty_pos+1) & 7;
			}
		}
	
		if( chan.id == 3 )
		{
			chan_accum[3] += 0xF0 & ((chan.duty_pos & 1) ? (WAVRAM[chan.duty_pos>>1]&0xf0) : (WAVRAM[chan.duty_pos>>1]<<4));
		} else if( chan.id == 4 ) {
			chan_accum[4] += (noise_lfsr & 1) ? 0 : (chan.volume<<4);
		} else {
			chan_accum[i] += (duty[(NR[1+10*i]>>6)*8 + channels[i].duty_pos] & (chan.volume<<4));
		}
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
			if( SDL_QueueAudio(AudioDev, &left_buffer[0], 1024*4) )
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
{ //currently unused
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

void chan_trigger(int C)
{
	if( C == 4 && !(NR[30] & 0x80) ) return;
	SoundChannel& chan = channels[C];
	
	chan.enabled = 1;
	chan.length_enabled = NR[10*C + 4] & 0x40;
	chan.length = ((C==3) ? (256-NR[31]) : (64 - (NR[10*C+1]&0x3F)));
	chan.duty_pos = 0;
	
	chan.volume = NR[10*C + 2]>>4;
	chan.env_dir = NR[10*C + 2]&8;
	chan.envelope_period = NR[10*C + 2] & 7;
	if( chan.envelope_period == 0 ) chan.envelope_period = 8;  // so no way to not use envelope?
	chan.env_timer = chan.envelope_period;
	
	if( C == 4 )
	{
		noise_lfsr = 0x7fff;
		chan.timer_reload = (NR[43]&7) ? (NR[43]&7)*16 : 8;
		chan.timer_reload <<= (NR[43]>>4)+1; //?  // ??, I found a doc that the above is the actual number of 4MHz clocks to use as period,
					 // but no actual idea what to do with the shift.
	} else if( C == 3 ) {
		chan.timer_reload = ((NR[34]&7) << 8) | NR[33];
		chan.timer_reload = (2048-chan.timer_reload)*2;
	} else {
		chan.timer_reload = ((NR[10*C + 4]&7) << 8) | NR[10*C + 3];
		chan.timer_reload = (2048-chan.timer_reload)*4;
	}
	
	chan.timer = chan.timer_reload;
	
	if( C == 1 )
	{ // sweep unique to channel 1
		chan.sweep_shadow = ((NR[14]&7) << 8) | NR[13];
		chan.sweep_period = (NR[10]>>4)&7;
		if( chan.sweep_period == 0 ) chan.sweep_period = 8;
		chan.sweep_timer = chan.sweep_period;
		
		if( NR[10] & 0x70 )
		{
			sweep_on = 1;
		} else {
			sweep_on = 0;
		}
	}
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
			for(int i = 1; i < 5; ++i) channels[i].enabled = 0;
		}
		return;
	}
	
	if( ! (NR[52] & 0x80) ) return;
	
	switch( addr )
	{
	case 0x10: NR[10] = val; break;
	case 0x11: NR[11] = val; break;
	case 0x12: NR[12] = val; break;
	case 0x13: NR[13] = val; break;
	case 0x14: NR[14] = val; if( val & 0x80 ) chan_trigger(1); break;
	
	case 0x16: NR[21] = val; break;
	case 0x17: NR[22] = val; break;
	case 0x18: NR[23] = val; break;
	case 0x19: NR[24] = val; if( val & 0x80 ) chan_trigger(2); break;
	
	case 0x1A: NR[30] = val; break;
	case 0x1B: NR[31] = val; break;
	case 0x1C: NR[32] = val; break;
	case 0x1D: NR[33] = val; break;
	case 0x1E: NR[34] = val; if( val & 0x80 ) chan_trigger(3); break;
	
	case 0x20: NR[41] = val; break;
	case 0x21: NR[42] = val; break;
	case 0x22: NR[43] = val; break;
	case 0x23: NR[44] = val; if( val & 0x80 ) chan_trigger(4); break;
	
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
	case 0x26: return (NR[52] & 0x80) | (channels[4].enabled<<3) | (channels[3].enabled<<2) | (channels[2].enabled<<1) | channels[1].enabled;
	default: break;
	}

	return 0xff;
}











