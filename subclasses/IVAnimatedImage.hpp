/*
IVANIMATEDIMAGE.HPP
NICK WILSON
2020
*/

#include <thread>

#include "IVUtil.hpp"	//utilities
#include "IVImage.hpp"	//base class

#include "gif_lib.h"	//gif support

#ifndef ANIMATEDIMAGE_H
#define ANIMATEDIMAGE_H

#define GIF_MIN_DELAY 0x02

class IVAnimatedImage : public IVImage{
private:
	GifFileType* gif_data = nullptr;
	SDL_Surface* surface = nullptr;

	uint8_t depth = 0;
	uint16_t frame_index = 0;
	uint16_t delay_val = 0;

	bool play = true;
	bool quit = false;

	std::thread animationThread;

	void setPalette(ColorMapObject* colorMap, SDL_Surface* surface);

	void setIndex(uint16_t index);

	void next();

	uint16_t getDelay(uint16_t index);

	uint16_t getDelay();

	ExtensionBlock* getGraphicsBlock(uint16_t index);

	void animate();

public:
	uint16_t frame_count = 0;
	bool playable;

	IVAnimatedImage() {}

	IVAnimatedImage(SDL_Renderer* renderer, std::filesystem::path path);

	~IVAnimatedImage();

	void prepare();

	void set_status(IVImage::state s);
};

#endif
