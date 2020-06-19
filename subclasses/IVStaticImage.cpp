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
	}
	else if (filetype == IVUTIL::TYPE_LIBHEIF) {
		//load HEIF image 
		heif::Context ctx;
		ctx.read_from_file(path.string());

		heif::ImageHandle handle = ctx.get_primary_image_handle();

		heif::Image img = handle.decode_image(heif_colorspace_undefined, heif_chroma_undefined);

		if (img.has_channel(heif_channel_R)
		 && img.has_channel(heif_channel_G)
		 && img.has_channel(heif_channel_B)) { //RGB mode

		 	surface = SDL_CreateRGBSurface(0,
				img.get_width(heif_channel_G), 
				img.get_height(heif_channel_G), 
				32, //BPP
				0x000000FF, //RGBA
				0x0000FF00, 
				0x00FF0000, 
				0xFF000000);

			/* TODO: RGB MODE */

		 	std::cout << IVUTIL::LOG_NOTICE << "Detected RGB HEIF" << std::endl;
		}
		else if (img.has_channel(heif_channel_Y)
			  && img.has_channel(heif_channel_Cb)
			  && img.has_channel(heif_channel_Cr)) { //YCbCr mode

			int im_w = img.get_width(heif_channel_Y);
			int im_h = img.get_height(heif_channel_Y);
			// int im_d = img.get_bits_per_pixel(heif_channel_Y); //not yet used, silence compiler
			int im_p;

			surface = SDL_CreateRGBSurface(0,
				im_w, 
				im_h, 
				32, //BPP
				0x000000FF, //RGBA
				0x0000FF00, 
				0x00FF0000, 
				0xFF000000);

			/* TODO: ALPHA */
			uint8_t* image_Y_data = img.get_plane(heif_channel_Y, &im_p);

			//for now, display Y channel, B&W data
			//colour math is complicated, I'll do it later
			/* TODO: MATH */
			for (int i = 0; i < im_w * im_h; i++) {
				int lum = image_Y_data[i];
				((uint8_t *) surface->pixels)[i * 4] 	 = lum;
				((uint8_t *) surface->pixels)[i * 4 + 1] = lum;
				((uint8_t *) surface->pixels)[i * 4 + 2] = lum;
				((uint8_t *) surface->pixels)[i * 4 + 3] = 0xFF;
			}

		 	std::cout << IVUTIL::LOG_NOTICE << "Detected YCbCr HEIF" << std::endl;
		}
		else {
			throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
		}
		/* TODO: Channel size checking */

		std::cout << "Success!" << std::endl;

	}
	else {
		//not a supported format
		throw IVUTIL::EXCEPT_IMG_LOAD_FAIL;
	}

	if (!surface) {
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
