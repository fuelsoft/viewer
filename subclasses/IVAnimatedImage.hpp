/*
IVANIMATEDIMAGE.HPP
NICK WILSON
2020
*/

#include "IVUtil.hpp"	//utilities
#include "IVImage.hpp"	//base class

#include "gif_lib.h"	//gif support

#ifndef ANIMATEDIMAGE_H
#define ANIMATEDIMAGE_H

class IVAnimatedImage : public IVImage{
private:
	GifFileType* gif_data;

	void setPalette(ColorMapObject* colorMap, SDL_Surface* surface);

public:
	int frame_count = 0;
	bool playable;

	IVAnimatedImage() {}

	IVAnimatedImage(SDL_Renderer* renderer, std::filesystem::path path);

	~IVAnimatedImage();
};

#endif
