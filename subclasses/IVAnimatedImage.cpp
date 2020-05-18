/*
IVANIMATEDIMAGE.CPP
NICK WILSON
2020
*/

#include "IVAnimatedImage.hpp"

/* PRIVATE */

/**
* setPalette	- Routine to convert giflib colour palette to SDL colour palette
* colorMap 		> giflib ColorMapObject containing colour count and palette
* surface 		> SDL Surface to apply converted palette to
*/
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

/**
* setIndex	- Set frame index record to provided index with boundaries checked
* index 	> New frame index
*/
void IVAnimatedImage::setIndex(uint16_t index) {
	this->frame_index = index;
	this->frame_index %= this->frame_count;
}

/**
* getDelay	- Load extension chunks related to provided image index and search for a delay value
* index 	> Target frame index
*/
uint16_t IVAnimatedImage::getDelay(uint16_t index) {
	index %= frame_count;
	// iterate and look for graphics extension with timing data
	for (int i = 0; i < gif_data->SavedImages[index].ExtensionBlockCount; i++) {
		if (gif_data->SavedImages[index].ExtensionBlocks[i].Function == GRAPHICS_EXT_FUNC_CODE) { // found it
			this->delay_val = (uint16_t) (gif_data->SavedImages[index].ExtensionBlocks[i].Bytes[1]); // see [1]
			break;
		}
	}
	return this->delay_val;
}

/**
* getDelay - Shortcut for getDelay() defaulting to current frame index
*/
uint16_t IVAnimatedImage::getDelay() {
	return getDelay(frame_index);
}

/**
* animate - The function is called as a thread and will advance the index and manage timing for the animation automatically.
*			If the status is set to paused, it will wait before continuing to animate.
*/
void IVAnimatedImage::animate() {
	while(!this->quit) {
		while (!this->play) {
			if (this->quit) return; //make it possible to break out for quitting while paused
			SDL_Delay(30);
		}
		setIndex(this->frame_index + 1); 	//advance by a frame
		this->ready = true;					//mark frame as ready to be prepare()'d
		getDelay();
		SDL_Delay(this->delay_val * 10);	//wait (gif resolution is only 1/100 of a sec, mult. by 10 for millis)
	}
	return;
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

	this->animated = (gif_data->ImageCount > 1);
	this->renderer = renderer;
	this->w = gif_data->SWidth;
	this->h = gif_data->SHeight;

	this->frame_count = gif_data->ImageCount;

	// NOTE: IN BITS
	this->depth = gif_data->SColorMap->BitsPerPixel;

	/* I came across an example gif which was reported as 6 BPP by giflib.		*/
	/* However, Windows reported it as 8 BPP and when set as 8 BPP, it loaded.	*/
	/* In conclusion, I'm going to treat everything as a multiple of 8.			*/
	if (this->depth % 8) {
		this->depth = 8;
	}

	// Create surface with existing gif data. 8 bit depth will trigger automatic creation of palette to be filled next
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void *) gif_data->SavedImages[0].RasterBits, this->w, this->h, this->depth, this->w * (this->depth >> 3), 0, 0, 0, 0);

	if (gif_data->SColorMap) { // if global colour palette defined
		// convert from global giflib colour to SDL colour and populate palette
		setPalette(gif_data->SColorMap, surface);
	}
	else if (gif_data->SavedImages[this->frame_index].ImageDesc.ColorMap) { // local colour palette
		// convert from local giflib colour to SDL colour and populate palette
		setPalette(gif_data->SavedImages[this->frame_index].ImageDesc.ColorMap, surface);
	}

	this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

	SDL_FreeSurface(surface);

	if (this->animated) animationThread = std::thread(&IVAnimatedImage::animate, this);
}

IVAnimatedImage::~IVAnimatedImage() {
	if (this->animated) {
		this->quit = true;
		animationThread.join();
	}
	DGifCloseFile(this->gif_data, nullptr);
	SDL_DestroyTexture(this->texture);
}

/**
* prepare - Create surface from current index, apply palette and create texture.
*			This should be called as infrequently as possible - static images don't need refreshing.
*/
void IVAnimatedImage::prepare() {
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void *) this->gif_data->SavedImages[frame_index].RasterBits, this->w, this->h, this->depth, this->w * (this->depth >> 3), 0, 0, 0, 0);

	if (gif_data->SColorMap) { // if global colour palette defined
		// convert from global giflib colour to SDL colour and populate palette
		setPalette(gif_data->SColorMap, surface);
	}
	else if (gif_data->SavedImages[this->frame_index].ImageDesc.ColorMap) { // local colour palette
		// convert from local giflib colour to SDL colour and populate palette
		setPalette(gif_data->SavedImages[this->frame_index].ImageDesc.ColorMap, surface);
	}

	SDL_DestroyTexture(this->texture); //delete old texture

	this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

	SDL_FreeSurface(surface);

	this->ready = false; 	// mark current frame as already requested
}

/**
* set_status 	- Set image state - play or pause (or toggle to swap)
* s 			> New IVImage::state state
*/
void IVAnimatedImage::set_status(state s) {
	switch (s) {
		case STATE_PLAY:
			this->play = true;
			break;
		case STATE_PAUSE:
			this->play = false;
			break;
		case STATE_TOGGLE:
			this->play = !this->play;
			break;
		default:
			break;
	}
}

/*
	TODO:
	- Transparency palette index support
*/

/*
[1]: 	Byte index is 1 because giflib cuts off the first 3 bytes of extension chunk.
		Payload is then [Packed Byte], [Upper Delay], [Lower Delay], [Transparency Index].
		Cast to 16 bit int then catches both bytes of the delay value.
*/
