#include "types.h"


void snd_cycle()
{





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














