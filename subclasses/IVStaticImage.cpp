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
		try {
			ctx.read_from_file(path.string());
		}
		catch (...) {
			std::cout << IVUTIL::LOG_ERROR << "LIBHEIF REPORTED IMAGE LOAD FAILURE" << std::endl;
			throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
		}

		heif::ImageHandle handle = ctx.get_primary_image_handle();
		heif::Image img;

		try {
			//load as R, G and B planes
			img = handle.decode_image(heif_colorspace_RGB, heif_chroma_interleaved_RGB);
		}
		catch (...) {
			std::cout << IVUTIL::LOG_ERROR << "LIBHEIF REPORTED IMAGE DECODE FAILURE" << std::endl;
			throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
		}

		if (img.has_channel(heif_channel_interleaved)) { //interleave mode

			int im_w = img.get_width(heif_channel_interleaved);
			int im_h = img.get_height(heif_channel_interleaved);
			int im_d = img.get_bits_per_pixel(heif_channel_interleaved); //depth (BPP)
			int im_p; //libheif calls this 'stride', SDL calls it 'pitch'

			uint8_t* RGB = img.get_plane(heif_channel_interleaved, &im_p); //pointer to pixel data

			surface = SDL_CreateRGBSurfaceWithFormat(0,
				im_w, 
				im_h, 
				32,
				SDL_PIXELFORMAT_RGBA32);

			if (!surface) {
				std::cout << IVUTIL::LOG_ERROR << "COULD NOT ALLOCATE SURFACE" << std::endl;
				throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
			}

			//constant offset from difference between line size and image width
			int const_offset = im_p - (im_w * (im_d / 8));

			int i, offset = 0;

			for (int y = 0; y < im_h; y++) {
				for (int x = 0; x < im_w; x++) {

					//determine pixel coordinates
					i = y * im_w + x;

					//assign new RGB values
					((uint8_t *) surface->pixels)[i * 4 + 0] = RGB[i * 3 + 0 + offset];
					((uint8_t *) surface->pixels)[i * 4 + 1] = RGB[i * 3 + 1 + offset];
					((uint8_t *) surface->pixels)[i * 4 + 2] = RGB[i * 3 + 2 + offset];
					((uint8_t *) surface->pixels)[i * 4 + 3] = 0xFF;
				}

				//increment line offset
				offset += const_offset;
			}
		}
		else {
			std::cout << IVUTIL::LOG_ERROR << "UNSUPPORTED COLOUR FORMAT" << std::endl;
			std::cout << "CHANNEL INDEX: ";
			for (int i = 0; i < 7; i++) {
				std::cout << (img.has_channel((heif_channel) i) ? '1' : '0');
			}
			std::cout << (img.has_channel((heif_channel) 10) ? '1' : '0') << std::endl;
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
