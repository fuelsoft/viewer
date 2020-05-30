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
	enum state {
		STATE_PLAY,
		STATE_PAUSE,
		STATE_TOGGLE,
	};

	uint16_t w, h;
	bool animated = false;
	bool ready = false;
	SDL_Texture* texture = nullptr;

	virtual void prepare() {};

	/* [[maybe_unused]] attribute is new to C++17 */
	virtual void set_status([[maybe_unused]] state s) {};

	virtual ~IVImage() {};
};

#endif
