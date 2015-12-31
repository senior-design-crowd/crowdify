#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "header.h"

bool sdl_init()
{
	bool success = true;

	// initialize main SDL system
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		// initialize SDL_Image extension
		int imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
			success = false;
		} else if (!TTF_Init() == -1) {
			printf("SDL_TTF could not initialize! SDL_TTF Error: %s\n", TTF_GetError());
			success = false;
		}
	}

	return success;
}

bool sdl_createWindow(const char* windowName, SDL_Window** r_window, SDL_Renderer** r_windowRenderer, SDL_Surface** r_windowSurface)
{
	// create the main window and retrieve its rendering surface
	bool success = true;

	*r_window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	
	if (*r_window == NULL)
	{
		success = false;
	} else {
		*r_windowRenderer = SDL_CreateRenderer(*r_window, -1, SDL_RENDERER_ACCELERATED);

		if (*r_windowRenderer == NULL) {
			success = false;
		}
	}

	if (success)
	{
		*r_windowSurface = SDL_GetWindowSurface(*r_window);
	} else {
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
	}

	return success;
}

void sdl_close(SDL_Window** nodeTransferWin)
{
	// destroy the main window
	SDL_DestroyWindow(*nodeTransferWin);
	*nodeTransferWin = NULL;

	// quit SDL and SDL_Image
	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
}

SDL_Surface* loadSurface(char* buf, size_t bufSize, SDL_Surface* nodeTransferWinSurface)
{
	SDL_Surface* optimizedSurface = NULL;

	// load image from given buffer
	SDL_Surface* loadedSurface = IMG_Load_RW(SDL_RWFromMem(buf, bufSize), 1);
	if (loadedSurface == NULL)
	{
		printf("Unable to load image from memory! SDL_image Error: %s\n", IMG_GetError());
	}
	else
	{
		// convert surface to same format as screen surface
		optimizedSurface = SDL_ConvertSurface(loadedSurface, nodeTransferWinSurface->format, NULL);
		if (optimizedSurface == NULL)
		{
			printf("Unable to optimize image from memory! SDL Error: %s\n", SDL_GetError());
		}

		// free original surface
		SDL_FreeSurface(loadedSurface);
	}

	return optimizedSurface;
}