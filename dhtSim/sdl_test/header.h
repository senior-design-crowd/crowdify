#ifndef __HEADER_H__
#define __HEADER_H__

#include <string>
#include <vector>
#include <SDL.h>

typedef struct _NodeState {
	int		sendingNode, receivingNode;
	bool	routing;
	bool	isAlive;
} NodeState;

typedef struct _NodeDHTArea {
	float left, right, top, bottom;
} NodeDHTArea;

extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

std::string generateGraph(const std::vector<NodeState>& nodeStates, const std::vector<std::pair<int, int>>& nodeEdges, SDL_Rect* screenRect);
bool renderGraph(const std::string& graphDotStr, SDL_Surface* screenSurface, SDL_Surface** r_pngSurface);

bool RenderDHT(const std::vector<NodeDHTArea>& dht, SDL_Renderer* renderer, SDL_Rect* screenRect);

bool sdl_init();
bool sdl_createWindow(const char* windowName, SDL_Window** r_window, SDL_Renderer** r_windowRenderer, SDL_Surface** r_windowSurface);
void sdl_close(SDL_Window** nodeTransferWin);
SDL_Surface* loadSurface(char* buf, size_t bufSize, SDL_Surface* nodeTransferWinSurface);

#endif