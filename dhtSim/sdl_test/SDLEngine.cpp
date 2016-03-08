#include <iostream>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "SDLEngine.h"

using namespace std;

SDLEngine::SDLEngine()
{
	bool success = true;

	// initialize main SDL system
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << endl;
		success = false;
	}
	else
	{
		// initialize SDL_Image extension
		int imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << endl;
			success = false;
		} else if (TTF_Init() == -1) {
			cout << "SDL_TTF could not initialize! SDL_TTF Error: " << TTF_GetError() << endl;
			success = false;
		}
	}

	// if we can't initialize these things
	// then we're done for lol
	if (!success) {
		throw;
	}
}

SDLEngine::~SDLEngine()
{
	// destroy the windows
	for (vector<Window>::const_iterator i = m_windows.begin(); i != m_windows.end(); ++i) {
		SDL_DestroyRenderer(i->sdlRenderer);
		SDL_DestroyWindow(i->sdlWindow);
	}

	// quit SDL and SDL_Image
	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
}

const Window& SDLEngine::GetWindow(int index)
{
	return m_windows[index];
}

bool SDLEngine::CreateSDLWindow(const char* windowName, int width, int height)
{
	// create the main window and retrieve its rendering surface
	bool success = true;

	Window window;
	window.width = width;
	window.height = height;
	window.sdlWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (window.sdlWindow == NULL) {
		success = false;
	} else {
		window.sdlRenderer = SDL_CreateRenderer(window.sdlWindow, -1, SDL_RENDERER_ACCELERATED);

		if (window.sdlRenderer == NULL) {
			success = false;
		}
	}

	if (success) {
		window.sdlSurface = SDL_GetWindowSurface(window.sdlWindow);
		window.sdlWindowID = SDL_GetWindowID(window.sdlWindow);
		m_windows.push_back(window);
	} else {
		cout << "Window could not be created! SDL Error: " << SDL_GetError() << endl;
	}

	return success;
}

SDL_Texture* SDLEngine::LoadSDLImage(char* buf, size_t bufSize, int destinationWindow, int& width, int& height)
{
	SDL_Texture* finalTexture = NULL;

	// load image from given buffer
	SDL_Surface* loadedSurface = IMG_Load_RW(SDL_RWFromMem(buf, bufSize), 1);

	width = loadedSurface->w;
	height = loadedSurface->h;

	if (loadedSurface == NULL) {
		printf("Unable to load image from memory! SDL_image Error: %s\n", IMG_GetError());
	} else {
		SDL_Surface* windowSurface = m_windows[destinationWindow].sdlSurface;
		SDL_Renderer* windowRenderer = m_windows[destinationWindow].sdlRenderer;

		// convert surface to same format as screen surface
		SDL_Surface* optimizedSurface = SDL_ConvertSurface(loadedSurface, windowSurface->format, NULL);

		if (optimizedSurface == NULL) {
			printf("Unable to optimize image from memory! SDL Error: %s\n", SDL_GetError());
		} else {
			// create final texture
			finalTexture = SDL_CreateTextureFromSurface(windowRenderer, optimizedSurface);
			SDL_FreeSurface(optimizedSurface);
		}

		SDL_FreeSurface(loadedSurface);
	}

	return finalTexture;
}

bool SDLEngine::ProcessEvents()
{
	SDL_Event	e;
	bool		quit = false;

	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
			quit = true;
		} else if (e.type == SDL_WINDOWEVENT) {
			Window* window = NULL;

			for (vector<Window>::iterator i = m_windows.begin(); i != m_windows.end(); ++i) {
				if (e.window.windowID == i->sdlWindowID) {
					window = &(*i);
					break;
				}
			}

			if (e.window.type == SDL_WINDOWEVENT_SIZE_CHANGED) {
				SDL_GetWindowSize(window->sdlWindow, &window->width, &window->height);
				SDL_RenderPresent(window->sdlRenderer);
			} else if (e.window.type == SDL_WINDOWEVENT_EXPOSED) {
				SDL_RenderPresent(window->sdlRenderer);
			} else if (e.window.type == SDL_WINDOWEVENT_CLOSE) {
				quit = true;
				break;
			}
		}
	}

	return quit;
}

void SDLEngine::ClearWindow(int index)
{
	SDL_SetRenderDrawColor(m_windows[index].sdlRenderer, 255, 255, 255, 255);
	SDL_RenderClear(m_windows[index].sdlRenderer);
}

void SDLEngine::RenderTexture(int windowIndex, SDL_Texture* texture, SDL_Rect* dest, SDL_Rect* src)
{
	SDL_RenderCopy(m_windows[windowIndex].sdlRenderer, texture, src, dest);
}

void SDLEngine::PresentWindow(int windowIndex)
{
	SDL_RenderPresent(m_windows[windowIndex].sdlRenderer);
}

void SDLEngine::CreateFontTextures(const char* fontFile, int fontSize, SDL_Color* fontColor, int windowIndex, const vector<string>& vStrings, vector<FontTexture>& r_fontTextures)
{
	TTF_Font* arialFont = TTF_OpenFont(fontFile, fontSize);
	SDL_Renderer* renderer = m_windows[windowIndex].sdlRenderer;
	SDL_Surface* windowSurface = m_windows[windowIndex].sdlSurface;

	for (vector<string>::const_iterator i = vStrings.begin(); i != vStrings.end(); ++i) {
		FontTexture font;
		SDL_Surface* fontSurface = TTF_RenderText_Solid(arialFont, i->c_str(), *fontColor);
		SDL_Surface* optimizedSurface = SDL_ConvertSurface(fontSurface, windowSurface->format, NULL);
		font.texture = SDL_CreateTextureFromSurface(renderer, optimizedSurface);
		font.width = fontSurface->w;
		font.height = fontSurface->h;
		r_fontTextures.push_back(font);
		SDL_FreeSurface(fontSurface);
		SDL_FreeSurface(optimizedSurface);
	}
}

bool SDLEngine::TakeScreenshot(int windowIndex, const char* filename)
{
	// reference: http://stackoverflow.com/questions/20233469/how-do-i-take-and-save-a-bmp-screenshot-in-sdl-2
	// author: neilf

	SDL_Surface* saveSurface = NULL;
	SDL_Surface* infoSurface = NULL;
	infoSurface = SDL_GetWindowSurface(m_windows[windowIndex].sdlWindow);
	if (infoSurface == NULL) {
		std::cerr << "Failed to create info surface from window in saveScreenshotBMP(string), SDL_GetError() - " << SDL_GetError() << "\n";
	} else {
		unsigned char * pixels = new (std::nothrow) unsigned char[infoSurface->w * infoSurface->h * infoSurface->format->BytesPerPixel];
		if (pixels == 0) {
			std::cerr << "Unable to allocate memory for screenshot pixel data buffer!\n";
			return false;
		} else {
			if (SDL_RenderReadPixels(m_windows[windowIndex].sdlRenderer, &infoSurface->clip_rect, infoSurface->format->format, pixels, infoSurface->w * infoSurface->format->BytesPerPixel) != 0) {
				std::cerr << "Failed to read pixel data from SDL_Renderer object. SDL_GetError() - " << SDL_GetError() << "\n";
				pixels = NULL;
				return false;
			} else {
				saveSurface = SDL_CreateRGBSurfaceFrom(pixels, infoSurface->w, infoSurface->h, infoSurface->format->BitsPerPixel, infoSurface->w * infoSurface->format->BytesPerPixel, infoSurface->format->Rmask, infoSurface->format->Gmask, infoSurface->format->Bmask, infoSurface->format->Amask);
				if (saveSurface == NULL) {
					std::cerr << "Couldn't create SDL_Surface from renderer pixel data. SDL_GetError() - " << SDL_GetError() << "\n";
					return false;
				}
				
				SDL_SaveBMP(saveSurface, filename);
				SDL_FreeSurface(saveSurface);
				saveSurface = NULL;
			}

			delete[] pixels;
		}

		SDL_FreeSurface(infoSurface);
		infoSurface = NULL;
	}

	return true;
}