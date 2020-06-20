/*
IVSTATICIMAGE.CPP
NICK WILSON
2020
*/

#include "IVStaticImage.hpp"

/* PUBLIC */

IVStaticImage::IVStaticImage(SDL_Renderer* renderer, std::filesystem::path path) {

	SDL_Surface* surface = nullptr;
	int filetype = IVUTIL::libSupport(path.extension().string());

	if (filetype == IVUTIL::TYPE_SDL) {
		//load SDL image
		surface = IMG_Load(path.string().c_str());

		if (!surface) {
			std::cout << IVUTIL::LOG_ERROR << "COULD NOT CREATE SURFACE" << std::endl;
			throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
		}
	}
	else if (filetype == IVUTIL::TYPE_LIBHEIF) {
		//load HEIF image 
		heif::Context ctx;
		ctx.read_from_file(path.string());
		heif::ImageHandle handle = ctx.get_primary_image_handle();

		//loading as interleaved RGB will force an automatic conversion from RGB(A)/YCbCr to RGBA
		heif::Image img = handle.decode_image(heif_colorspace_RGB, heif_chroma_interleaved_RGBA);

		if (img.has_channel(heif_channel_interleaved)) {
			int im_w = img.get_width(heif_channel_interleaved);
			int im_h = img.get_height(heif_channel_interleaved);
			int im_d = img.get_bits_per_pixel(heif_channel_interleaved); //depth
			int im_p; //libheif calls this 'stride', SDL calls it 'pitch', unused

			surface = SDL_CreateRGBSurfaceWithFormat(0,
				im_w, 
				im_h, 
				im_d,
				SDL_PIXELFORMAT_RGBA32);

			if (!surface) {
				std::cout << IVUTIL::LOG_ERROR << "COULD NOT ALLOCATE SURFACE" << std::endl;
				throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
			}

		 	uint8_t* RGBA = img.get_plane(heif_channel_interleaved, &im_p); //pointer to pixel data

		 	memcpy(surface->pixels, (void *) RGBA, im_w * im_h * (im_d >> 3)); //copy to SDL Surface
		}
		else {
			std::cout << IVUTIL::LOG_ERROR << "UNSUPPORTED COLOUR FORMAT" << std::endl;
			throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
		}
	}
	else {
		//not a supported format
		std::cout << IVUTIL::LOG_ERROR << "UNSUPPORTED IMAGE FORMAT" << std::endl;
		throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
	}

	this->animated = false;
	this->renderer = renderer;
	this->w = surface->w;
	this->h = surface->h;

	this->texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);
}

IVStaticImage::~IVStaticImage() {
	SDL_DestroyTexture(this->texture);
}
