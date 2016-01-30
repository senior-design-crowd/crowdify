#ifndef __SDL_ENGINE_H__
#define __SDL_ENGINE_H__

#include <vector>

#include <SDL.h>

typedef struct
{
	SDL_Window*		sdlWindow;
	SDL_Renderer*	sdlRenderer;
	SDL_Surface*	sdlSurface;
	int				width;
	int				height;
	int				sdlWindowID;
} Window;

typedef struct
{
	SDL_Texture*	texture;
	int				width;
	int				height;
} FontTexture;

class SDLEngine
{
public:
	SDLEngine();
	~SDLEngine();

	bool			ProcessEvents();
	
	SDL_Texture*	LoadSDLImage(char* buf, size_t bufSize, int destinationWindow, int& width, int& height);

	void			RenderTexture(int windowIndex, SDL_Texture* texture, SDL_Rect* dest = NULL, SDL_Rect* src = NULL);
	void			PresentWindow(int windowIndex);

	bool			CreateSDLWindow(const char* windowName, int width = 800, int height = 600);
	void			ClearWindow(int index);
	const Window&	GetWindow(int index);

	void			CreateFontTextures(const char* fontFile, int fontSize, SDL_Color* fontColor, int windowIndex, const std::vector<std::string>& vStrings, std::vector<FontTexture>& r_fontTextures);

private:
	std::vector<Window> m_windows;
};

#endif
