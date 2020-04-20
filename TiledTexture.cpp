/*
TILEDTEXTURE.CPP
NICK WILSON
2020
*/

#include "TiledTexture.hpp"

/* PUBLIC */

TiledTexture::TiledTexture(SDL_Renderer* renderer, int w, int h, uint32_t HIGH, uint32_t LOW) {
	SDL_Surface* surface = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);

	for (int lh = 0; lh < h; lh++) {
		for (int lw = 0; lw < w; lw++) {
			if ((lh < (h/2)) ^ (lw < (w/2))) {
				*((uint32_t *) surface->pixels + (lh * w + lw)) = HIGH;
			}
			else {
				*((uint32_t *) surface->pixels + (lh * w + lw)) = LOW;
			}
		}
	}

	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
}

TiledTexture::~TiledTexture() {
	SDL_DestroyTexture(texture);
}
