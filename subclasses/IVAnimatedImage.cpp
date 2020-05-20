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
	//call search for graphics block
	ExtensionBlock* gfx = getGraphicsBlock(index);
	//if one was found, update delay
	if (gfx) this->delay_val = (uint16_t) (gfx->Bytes[1]); // see [1]
	return this->delay_val;
}

/**
* getDelay - Shortcut for getDelay() defaulting to current frame index
*/
uint16_t IVAnimatedImage::getDelay() {
	return getDelay(frame_index);
}

/**
* getGraphicsBlock 	- Locate and return the Extension Block of type Graphics Control for provided index
* index 			> Target frame index
*/
ExtensionBlock* IVAnimatedImage::getGraphicsBlock(uint16_t index) {
	// iterate and look for graphics extension with timing data
	for (int i = 0; i < gif_data->SavedImages[index].ExtensionBlockCount; i++) {
		if (gif_data->SavedImages[index].ExtensionBlocks[i].Function == GRAPHICS_EXT_FUNC_CODE) { // found it
			return &gif_data->SavedImages[index].ExtensionBlocks[i];
		}
	}
	return nullptr;
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

	// Gif is a static image, better suited as a IVStaticImage
	if (gif_data->ImageCount == 1) {
		throw IVUTIL::EXCEPT_IMG_GIF_STATIC;
	}

	this->animated = (gif_data->ImageCount > 1);
	this->renderer = renderer;
	this->w = gif_data->SWidth;
	this->h = gif_data->SHeight;

	this->frame_count = gif_data->ImageCount;

	if (gif_data->SColorMap) {
		// NOTE: IN BITS
		this->depth = gif_data->SColorMap->BitsPerPixel;

		/* I came across an example gif which was reported as 6 BPP by giflib.		*/
		/* However, Windows reported it as 8 BPP and when set as 8 BPP, it loaded.	*/
		/* In conclusion, I'm going to treat everything as a multiple of 8.			*/
		if (this->depth % 8) {
			this->depth = 8;
		}
	}
	else {
		// No global palette, assume 8 BPP depth
		this->depth = 8;
	}
	

	// Create surface with existing gif data. 8 bit depth will trigger automatic creation of palette to be filled next
	this->surface = SDL_CreateRGBSurfaceFrom((void *) gif_data->SavedImages[0].RasterBits, this->w, this->h, this->depth, this->w * (this->depth >> 3), 0, 0, 0, 0);

	if (gif_data->SColorMap) { // if global colour palette defined
		// convert from global giflib colour to SDL colour and populate palette
		setPalette(gif_data->SColorMap, this->surface);
	}
	else if (gif_data->SavedImages[this->frame_index].ImageDesc.ColorMap) { // local colour palette
		// convert from local giflib colour to SDL colour and populate palette
		setPalette(gif_data->SavedImages[this->frame_index].ImageDesc.ColorMap, this->surface);
	}

	/* Convert to a more friendly format */
	SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);
	SDL_Surface *output = SDL_ConvertSurface(surface, format, 0);
	SDL_FreeFormat(format);
	SDL_FreeSurface(surface);
	surface = output;

	this->texture = SDL_CreateTextureFromSurface(this->renderer, this->surface);

	if (this->animated) animationThread = std::thread(&IVAnimatedImage::animate, this);
}

IVAnimatedImage::~IVAnimatedImage() {
	if (this->animated) {
		this->quit = true;
		animationThread.join();
	}
	DGifCloseFile(this->gif_data, nullptr);
	SDL_FreeSurface(this->surface);
	SDL_DestroyTexture(this->texture);
}

/**
* prepare - Create surface from current index, apply palette and create texture, keying as required.
*			This should be called as infrequently as possible - static images don't need refreshing.
*/
void IVAnimatedImage::prepare() {
	int local_index = frame_index;

	// shortened for simplicity
	GifImageDesc* im_desc = &this->gif_data->SavedImages[local_index].ImageDesc;

	// destination for copy - if only a region is being updated, this will not cover the whole image
	SDL_Rect dest;
	dest.x = im_desc->Left;
	dest.y = im_desc->Top;
	dest.w = im_desc->Width;
	dest.h = im_desc->Height;

	SDL_Surface* temp = SDL_CreateRGBSurfaceFrom((void *) this->gif_data->SavedImages[local_index].RasterBits, im_desc->Width, im_desc->Height, this->depth, im_desc->Width * (this->depth >> 3), 0, 0, 0, 0);

	if (gif_data->SColorMap) { // if global colour palette defined
		// convert from global giflib colour to SDL colour and populate palette
		setPalette(gif_data->SColorMap, temp);
	}
	else if (gif_data->SavedImages[local_index].ImageDesc.ColorMap) { // local colour palette
		// convert from local giflib colour to SDL colour and populate palette
		setPalette(gif_data->SavedImages[local_index].ImageDesc.ColorMap, temp);
	}

	//get gfx extension block to find transparent palette index
	ExtensionBlock* gfx = getGraphicsBlock(local_index);

	//if gfx block exists and transparency flag is set, set colour key
	if (gfx && (gfx->Bytes[0] & 0x01)) {
		// get a pointer to the referenced target palette index for keying
		uint8_t* gif_plt = ((uint8_t*) (gif_data->SColorMap->Colors)) + ((gfx->Bytes[3]) * 3); // see [1] for Bytes[3] details
		// convert palette to uint32_t for SDL to handle
		uint32_t key_clr = SDL_MapRGB(temp->format, gif_plt[0], gif_plt[1], gif_plt[2]);
		// set the key with SDL_TRUE to enable it
		SDL_SetColorKey(temp, SDL_TRUE, key_clr);
	}

	// copy over region that is being updated, leaving anything else
	SDL_BlitSurface(temp, nullptr, this->surface, &dest);
	SDL_FreeSurface(temp);

	SDL_DestroyTexture(this->texture); //delete old texture
	this->texture = SDL_CreateTextureFromSurface(this->renderer, this->surface);

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
	- Some troublesome gifs will now no longer play at all
	- Scrambled palette bug still exists on some images
*/

/*
[1]: 	Giflib cuts off the first 3 bytes of extension chunk.
		Payload is then [Packed/Flag Byte], [Upper Delay], [Lower Delay], [Transparency Index].
*/
