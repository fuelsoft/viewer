/*
IVANIMATEDIMAGE.CPP
NICK WILSON
2020
*/

#include "IVAnimatedImage.hpp"

/* PRIVATE */

/* Routine to convert giflib colour palette to SDL colour palette */
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
	this->gif_data = DGifOpenFileName(path.string().c_str(), nullptr);

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
	this->frame_count = gif_data->ImageCount;
	this->animated = (gif_data->ImageCount > 1);
	this->renderer = renderer;

	// NOTE: IN BITS
	this->depth = gif_data->SColorMap->BitsPerPixel;

	/* I came across an example gif which was reported as 6 BPP by giflib.		*/
	/* However, Windows reported it as 8 BPP and when set as 8 BPP, it loaded.	*/
	/* In conclusion, I'm going to treat everything as a multiple of 8.			*/
	if (this->depth % 8) {
		this->depth = 8;
	}

	// Create surface with existing gif data. 8 bit depth will trigger automatic creation of palette to be filled next
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void *) gif_data->SavedImages[0].RasterBits, this->w, this->h, depth, this->w * (depth >> 3), 0, 0, 0, 0);

	// If local colour palette defined
	if (gif_data->SavedImages[0].ImageDesc.ColorMap) {
		// Convert from local giflib colour to SDL colur and populate palette
		setPalette(gif_data->SavedImages[0].ImageDesc.ColorMap, surface);
	}
	// If global colour palette defined
	else if (gif_data->SColorMap) {
		// Convert from global giflib colour to SDL colur and populate palette
		setPalette(gif_data->SColorMap, surface);
	}

	this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

	SDL_FreeSurface(surface);
}

IVAnimatedImage::~IVAnimatedImage() {
	DGifCloseFile(this->gif_data, nullptr);
	SDL_DestroyTexture(this->texture);
}

void IVAnimatedImage::printDetails() {
	std::cout << this->w << " x " << this->h << std::endl;
	std::cout << this->frame_count << " frames" << std::endl;
}

int IVAnimatedImage::next() {
	this->frame_index++;
	this->frame_index %= this->frame_count;

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void *) this->gif_data->SavedImages[frame_index].RasterBits, this->w, this->h, this->depth, this->w * (this->depth >> 3), 0, 0, 0, 0);

	// If local colour palette defined
	if (gif_data->SavedImages[frame_index].ImageDesc.ColorMap) {
		// Convert from local giflib colour to SDL colur and populate palette
		setPalette(gif_data->SavedImages[frame_index].ImageDesc.ColorMap, surface);
	}
	// If global colour palette defined
	else if (gif_data->SColorMap) {
		// Convert from global giflib colour to SDL colur and populate palette
		setPalette(gif_data->SColorMap, surface);
	}

	SDL_DestroyTexture(this->texture);
	this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

	SDL_FreeSurface(surface);

	return 100; //TODO: Time in millis to next frame from ExtensionBlock
}

/*
TODO: File type check
*/
