#include <stdio.h>
#include <cstring>
#include <SDL.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <GL/glew.h>
#include "cxxopts.hpp"
#include "types.h"

extern u16 PC;
u8* ROMfile;
bool emubios = true;
extern bool BIOS_On;
extern u8 BIOS[0x100];
extern u32 screen[160*144];

SDL_Window* MainWindow;
SDL_Surface* MainSurf;
SDL_Texture* gfxtex;
SDL_Renderer* MainRend;
bool MainRunning = true;
std::chrono::system_clock::time_point stamp;

void gb_reset();
void gb_interpret();
void gb_mapping();

int main(int argc, char** args)
{
	std::string biosfile = "GBBIOS.BIN";
	cxxopts::Options options("rombro", "rombro, the GB Emulator");
	options.add_options()
		("b,bypass", "Bypass BIOS")
		("d,debug", "Activate debugger (unimplemented)")
		//("i", "Use interpreter rather than the recompiler")
		("B,bios", "set BIOS file (GBBIOS.BIN by default)", cxxopts::value<std::string>())
		("f,file", "binary ROM file to emulate", cxxopts::value<std::string>())
			;
	options.parse_positional({"file"});

	auto result = options.parse(argc, args);
	if( result.count("b") )
	{
		emubios = false;
		BIOS_On = false;
	}
	if( result.count("B") )
	{
		biosfile = result["B"].as<std::string>();
	}
	if( !result.count("f") )
	{
		std::cout << options.help() << std::endl;
		return 0;
	}

	std::string filname = result["f"].as<std::string>();
	FILE* fp = fopen(filname.c_str(), "rb");
	if( !fp )
	{
		printf("error: unable to open file \"%s\"\n", filname.c_str());
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 filsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROMfile = new u8[filsize];
	int unu = fread(ROMfile, 1, filsize, fp);
	fclose(fp);

	if( emubios )
	{
		printf("Info: Using bootrom file <%s>\n", biosfile.c_str());
		FILE* fp = fopen(biosfile.c_str(), "rb");
		if( ! fp )
		{
			printf("error: unable to find BIOS file '%s'.\n", biosfile.c_str());
			printf("Use -b to attempt to run without it.\n");
			return 1;
		}

		int unu = fread(BIOS, 1, 256, fp);
		fclose(fp);
	}
	
	gb_mapping();	
	gb_reset();

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);

	SDL_AudioSpec want, have;
	SDL_AudioDeviceID dev;

	SDL_memset(&want, 0, sizeof(want));
	want.freq = 44100;
	want.format = AUDIO_F32;
	want.channels = 1;
	want.samples = 1024;
	//want.callback = snd_callback;

	/*dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (dev == 0) 
	{
		printf("Error: Failed to open audio: %s\n", SDL_GetError());
		return 1;
	} else if (have.format != want.format) {
		printf("Error: need float32 audio format.\n");
	}
	SDL_PauseAudioDevice(dev, 0);
	*/
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	MainWindow = SDL_CreateWindow("ROMBro", 0, 0, 1200, 720, SDL_WINDOW_OPENGL);
	if( !MainWindow )
	{
		printf("Error creating window\n");
		return -1;
	}
	MainSurf = SDL_GetWindowSurface(MainWindow);
	MainRend = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED);
	if( !MainRend )
	{
		printf("Error creating renderer\n");
		return -1;
	}
	gfxtex = SDL_CreateTexture(MainRend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	if( !gfxtex )
	{
		printf("Error creating texture\n");
		return -1;
	}
	SDL_SetTextureBlendMode(gfxtex, SDL_BLENDMODE_NONE);
	glewInit();

 	SDL_Rect rect{ 0,0, 320, 288 };

	int scanlines = 0;
	
	stamp = std::chrono::system_clock::now();
	
	while( MainRunning )
	{
		SDL_Event event;
		while( SDL_PollEvent(&event) ) 
		{
			switch( event.type )
			{
			case SDL_QUIT:
				MainRunning = false;
				break;
			}
		}

		glViewport(0, 0, 1200, 720);
		glClearColor(0.4f, 0.5f, 0.6f, 0);
		//glClear(GL_COLOR_BUFFER_BIT);

		auto stamp1 = std::chrono::system_clock::now();

		for(int i = 0; i < 70224/164; ++i)
			gb_interpret();

		scanlines++;
		if( scanlines >= 164 )
		{
			scanlines = 0;
			u8* pixels;
			u32 stride;
			SDL_LockTexture(gfxtex, nullptr,(void**) &pixels,(int*) &stride);
			
			memcpy(pixels, screen, 160*144*4);
		
			SDL_UnlockTexture(gfxtex);
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_RenderCopy(MainRend, gfxtex, nullptr, &rect);
		SDL_RenderPresent(MainRend);
	}

	SDL_Quit();
	return 0;
}




