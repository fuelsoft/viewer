/*
IVIMAGE.HPP
NICK WILSON
2020
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdint>		//standard number formats
#include <string>		//string type
#include <filesystem>	//fs path

#include "IVUtil.hpp"	//utilities

#ifndef IVIMAGE_H
#define IVIMAGE_H

class IVImage {
protected:
	SDL_Renderer* renderer = nullptr;

public:
	uint16_t w, h;
	bool animated;
	SDL_Texture* texture = nullptr;

	virtual void printDetails() {}

	virtual int next() { return 0; }
};

#endif
