#include <SDL.h>
#include <vector>

using namespace std;

#include "header.h"

bool RenderDHT(const vector<NodeDHTArea>& dht, SDL_Renderer* renderer, SDL_Rect* screenRect)
{
	int bigAreaColorR = 61, bigAreaColorG = 61, bigAreaColorB = 41;
	int smallAreaColorR = 255, smallAreaColorG = 255, smallAreaColorB = 255;

	for (vector<NodeDHTArea>::const_iterator i = dht.begin(); i != dht.end(); ++i) {
		float rectArea = (i->right - i->left) * (i->bottom - i->top);
		int rectColorR = rectArea * bigAreaColorR + (1.0f - rectArea) * smallAreaColorR;
		int rectColorG = rectArea * bigAreaColorG + (1.0f - rectArea) * smallAreaColorG;
		int rectColorB = rectArea * bigAreaColorB + (1.0f - rectArea) * smallAreaColorB;

		SDL_SetRenderDrawColor(renderer, rectColorR, rectColorG, rectColorB, 255);

		SDL_Rect rectInScreenSpace;
		rectInScreenSpace.x = i->left * screenRect->w;
		rectInScreenSpace.y = i->top * screenRect->h;
		rectInScreenSpace.w = (i->right - i->left) * screenRect->w;
		rectInScreenSpace.h = (i->bottom - i->top) * screenRect->h;

		SDL_RenderFillRect(renderer, &rectInScreenSpace);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &rectInScreenSpace);
	}

	return true;
}