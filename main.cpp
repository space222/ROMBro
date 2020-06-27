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
extern int total_cycles;

u8* keys;
SDL_Window* MainWindow;
SDL_Surface* MainSurf;
SDL_Texture* gfxtex;
SDL_Renderer* MainRend;
bool MainRunning = true;

std::string romname;

void gb_reset();
void gb_interpret();
void gb_mapping();
void mem_save();
void snd_callback(void*, u8*, int);

int controllers_connected = 0;
SDL_Joystick* controller0 = nullptr;

int main(int argc, char** args)
{
	std::string biosfile = "GBBOOT.BIN";
	cxxopts::Options options("ROMBro", "ROMBro, the GB Emulator");
	options.add_options()
		("b,bypass", "Bypass BIOS")
		//("d,debug", "Activate debugger (unimplemented)")
		//("i", "Use interpreter rather than the recompiler")
		("B,bootrom", "set bootrom file (GBBOOT.BIN by default)", cxxopts::value<std::string>())
		("x", "set integer zoom level", cxxopts::value<float>())
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
	float zfactor = 2.0f;
	if( result.count("x") )
	{
		zfactor = result["x"].as<float>();
		if( zfactor < 1.0f ) zfactor = 2.0f;
	}

	romname = result["f"].as<std::string>();
	FILE* fp = fopen(romname.c_str(), "rb");
	if( !fp )
	{
		printf("error: unable to open file \"%s\"\n", romname.c_str());
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
			printf("error: unable to find bootrom file '%s'.\n", biosfile.c_str());
			printf("Use -b to run without it.\n");
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
	want.channels = 2;
	want.samples = 1024;
	want.callback = snd_callback;

	dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (dev == 0) 
	{
		printf("Error: Failed to open audio: %s\n", SDL_GetError());
		return 1;
	} else if (have.format != want.format) {
		printf("Error: need float32 audio format.\n");
	}
	SDL_PauseAudioDevice(dev, 0);
	
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

 	SDL_Rect rect{ 0,0, (int)(160.0f*zfactor), (int)(144.0f*zfactor) };

	int scanlines = 0;
	
	glViewport(0, 0, 1200, 720);
	glClearColor(0.4f, 0.5f, 0.6f, 0);

	while( MainRunning )
	{
		SDL_Event event;
		while( SDL_PollEvent(&event) ) 
		{
			switch( event.type )
			{
			case SDL_JOYDEVICEADDED:
				if( !controllers_connected ) controller0 = SDL_JoystickOpen(controllers_connected);
				controllers_connected++;
				break;
			case SDL_JOYDEVICEREMOVED:
				controllers_connected--;
				if( !controllers_connected ) controller0 = nullptr;
				break;
			case SDL_QUIT:
				MainRunning = false;
				break;
			}
		}

		keys =(u8*) SDL_GetKeyboardState(NULL);
	
		auto stamp1 = std::chrono::system_clock::now();

		while( total_cycles < 70224 ) gb_interpret();
			
		total_cycles -= 70224;
			
		while( std::chrono::system_clock::now() - stamp1 < std::chrono::milliseconds(8) );
		
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_RenderCopy(MainRend, gfxtex, nullptr, &rect);
		SDL_RenderPresent(MainRend);
	}
	
	mem_save();

	SDL_Quit();
	return 0;
}




