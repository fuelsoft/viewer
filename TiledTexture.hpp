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
	int x, y, w, h;
	int px, py, pw, ph;

	SDL_Texture* texture = nullptr;

	TiledTexture() {}

	TiledTexture(int w, int h);

	~TiledTexture();
};

#endif
