/*
IVSTATICIMAGE.HPP
NICK WILSON
2020
*/

#include <libheif/heif_cxx.h>

#include "IVUtil.hpp"	//utilities
#include "IVImage.hpp"	//base class

#ifndef STATICIMAGE_H
#define STATICIMAGE_H

class IVStaticImage : public IVImage {
private:

public:
	IVStaticImage() {}

	IVStaticImage(SDL_Renderer* renderer, std::filesystem::path path);

	~IVStaticImage();
};

#endif
