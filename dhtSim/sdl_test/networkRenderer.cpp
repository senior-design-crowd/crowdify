#include <SDL.h>
#include <vector>

using namespace std;

#include "SDLEngine.h"
#include "header.h"

bool RenderDHT(const vector<NodeDHTArea>& dht, const vector<FontTexture>& nodeLabels, SDLEngine* engine, int windowIndex)
{
	int bigAreaColorR = 61, bigAreaColorG = 61, bigAreaColorB = 41;
	int smallAreaColorR = 255, smallAreaColorG = 255, smallAreaColorB = 255;

	const Window&	window = engine->GetWindow(windowIndex);
	SDL_Renderer*	renderer = engine->GetWindow(windowIndex).sdlRenderer;

	int nodeNum = 0;
	for (vector<NodeDHTArea>::const_iterator i = dht.begin(); i != dht.end(); ++i, ++nodeNum) {
		if (i->left == 0.0f && i->right == 0.0f && i->top == 0.0f && i->bottom == 0.0f) {
			continue;
		}

		float rectArea = (i->right - i->left) * (i->bottom - i->top);
		int rectColorR = (int)(rectArea * bigAreaColorR + (1.0f - rectArea) * smallAreaColorR);
		int rectColorG = (int)(rectArea * bigAreaColorG + (1.0f - rectArea) * smallAreaColorG);
		int rectColorB = (int)(rectArea * bigAreaColorB + (1.0f - rectArea) * smallAreaColorB);

		SDL_SetRenderDrawColor(renderer, rectColorR, rectColorG, rectColorB, 255);
		//SDL_SetRenderDrawColor(renderer, bigAreaColorR, bigAreaColorG, bigAreaColorB, 255);

		SDL_Rect rectInScreenSpace;
		rectInScreenSpace.x = (int)(i->left * window.width);
		rectInScreenSpace.y = (int)(i->top * window.height);
		rectInScreenSpace.w = (int)((i->right - i->left) * window.width);
		rectInScreenSpace.h = (int)((i->bottom - i->top) * window.height);

		SDL_RenderFillRect(renderer, &rectInScreenSpace);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &rectInScreenSpace);
		
		// center the node label inside the DHT area rectangle
		SDL_Rect nodeLabelDest;
		nodeLabelDest.w = nodeLabels[nodeNum].width;
		nodeLabelDest.h = nodeLabels[nodeNum].height;
		nodeLabelDest.x = rectInScreenSpace.x + (rectInScreenSpace.w - nodeLabelDest.w) / 2;
		nodeLabelDest.y = rectInScreenSpace.y + (rectInScreenSpace.h - nodeLabelDest.h) / 2;

		engine->RenderTexture(1, nodeLabels[nodeNum].texture, &nodeLabelDest);
	}

	return true;
}