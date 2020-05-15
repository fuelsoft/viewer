/*
TILEDTEXTURE.HPP
NICK WILSON
2020
*/

#include <SDL2/SDL.h>

#include <cstdint>		//standard number formats
#include <string>		//string type

#ifndef TILEDTEXTUREOBJ_H
#define TILEDTEXTUREOBJ_H

class TiledTexture {

private:

public:
	int w, h;
	uint32_t HIGH, LOW;

	SDL_Texture* texture = nullptr;

	TiledTexture() {}

	TiledTexture(SDL_Renderer* renderer, int w, int h, uint32_t HIGH, uint32_t LOW);

	~TiledTexture();
};

#endif
