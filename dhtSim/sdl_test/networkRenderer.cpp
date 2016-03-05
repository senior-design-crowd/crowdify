#include <sstream>
#include <SDL.h>
#include <vector>

using namespace std;

#include "SDLEngine.h"
#include "header.h"

bool RenderDHT(const vector<DHTArea>& dht, const vector<FontTexture>& nodeLabels, SDLEngine* engine, int windowIndex)
{
	int bigAreaColorR = 61, bigAreaColorG = 61, bigAreaColorB = 41;
	int smallAreaColorR = 255, smallAreaColorG = 255, smallAreaColorB = 255;

	const Window&	window = engine->GetWindow(windowIndex);
	SDL_Renderer*	renderer = engine->GetWindow(windowIndex).sdlRenderer;

	int nodeNum = 0;
	
	for (vector<DHTArea>::const_iterator i = dht.begin(); i != dht.end(); ++i, ++nodeNum) {
		const vector<DHTRegion>& regionVec = i->GetRegions();

		for (vector<DHTRegion>::const_iterator j = regionVec.begin(); j != regionVec.end(); ++j) {
			if (j->left == 0.0f && j->right == 0.0f && j->top == 0.0f && j->bottom == 0.0f) {
				continue;
			}

			float rectArea = (j->right - j->left) * (j->bottom - j->top);
			int rectColorR = (int)(rectArea * bigAreaColorR + (1.0f - rectArea) * smallAreaColorR);
			int rectColorG = (int)(rectArea * bigAreaColorG + (1.0f - rectArea) * smallAreaColorG);
			int rectColorB = (int)(rectArea * bigAreaColorB + (1.0f - rectArea) * smallAreaColorB);

			SDL_SetRenderDrawColor(renderer, rectColorR, rectColorG, rectColorB, 255);
			//SDL_SetRenderDrawColor(renderer, bigAreaColorR, bigAreaColorG, bigAreaColorB, 255);

			SDL_Rect rectInScreenSpace;
			rectInScreenSpace.x = (int)(j->left * window.width);
			rectInScreenSpace.y = (int)(j->top * window.height);
			rectInScreenSpace.w = (int)((j->right - j->left) * window.width);
			rectInScreenSpace.h = (int)((j->bottom - j->top) * window.height);

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
	}

	return true;
}