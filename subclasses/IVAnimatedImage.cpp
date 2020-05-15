/*
IVANIMATEDIMAGE.CPP
NICK WILSON
2020
*/

#include "IVAnimatedImage.hpp"

/* PRIVATE */

void IVAnimatedImage::setPalette(ColorMapObject* colorMap, SDL_Surface* surface) {
	// start gif colour at the start
	uint8_t* gc = (uint8_t*) colorMap->Colors;

	// this long, stupid conversion required as SDL *REQUIRES* an alpha channel
	for (int i = 0; i < colorMap->ColorCount; i++) {
		//create corresponding SDL_Color
		SDL_Color sc = {gc[0], gc[1], gc[2], 0};
		//set palette with new colour
		SDL_SetPaletteColors(surface->format->palette, &sc, i, 1);
		// move along to next colour
		gc += sizeof(GifColorType);
	}
}

/* PUBLIC */

IVAnimatedImage::IVAnimatedImage(SDL_Renderer* renderer, std::filesystem::path path) {
	gif_data = DGifOpenFileName(path.string().c_str(), nullptr);

	// Will be null if image metadata could not be read
	if (!gif_data) {
		throw IVUTIL::EXCEPT_IMG_OPEN_FAIL;
	}

	// Will return GIF_ERROR if gif data structure cannot be populated
	if (DGifSlurp(gif_data) == GIF_ERROR) {
		throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
	}

	this->w = gif_data->SWidth;
	this->h = gif_data->SHeight;
	this->animated = false;
	this->renderer = renderer;

	// NOTE: IN BITS
	int depth = gif_data->SColorMap->BitsPerPixel;

	// Create surface with existing gif data. 8 bit depth will trigger automatic creation of palette to be filled next
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void *) gif_data->SavedImages->RasterBits, this->w, this->h, depth, this->w * (depth >> 3), 0, 0, 0, 0);

	// Convert from giflib colour to SDL colur and populate palette
	setPalette(gif_data->SColorMap, surface);

	texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);
}

IVAnimatedImage::~IVAnimatedImage() {
	DGifCloseFile(gif_data, nullptr);
	SDL_DestroyTexture(texture);
}


/*
TODO: File type check
*/
