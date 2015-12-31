#ifndef __HEADER_H__
#define __HEADER_H__

#include <string>
#include <vector>
#include <set>
#include <SDL.h>

#include "SDLEngine.h"

typedef struct _NodeState {
	int		sendingNode, receivingNode;
	bool	routing;
	bool	isAlive;
} NodeState;

typedef struct _NodeDHTArea {
	float left, right, top, bottom;
} NodeDHTArea;

std::string		GenerateGraph(const std::vector<NodeState>& nodeStates, const std::vector<std::set<int>>& nodeEdges, SDLEngine* engine, int windowIndex);
bool			RenderGraph(const std::string& graphDotStr, SDLEngine* engine, int windowIndex, SDL_Texture** r_pngTexture, int& width, int& height);
bool			RenderDHT(const std::vector<NodeDHTArea>& dht, const std::vector<FontTexture>& nodeLabels, SDLEngine* engine, int windowIndex);

#endif