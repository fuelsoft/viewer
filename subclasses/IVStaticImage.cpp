/*
IVSTATICIMAGE.CPP
NICK WILSON
2020
*/

#include "IVStaticImage.hpp"

/* PUBLIC */

IVStaticImage::IVStaticImage(SDL_Renderer* renderer, std::filesystem::path path) {
	SDL_Surface* surface = IMG_Load(path.string().c_str());

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

void IVStaticImage::printDetails() {
	std::cout << this->w << " x " << this->h << std::endl;
}

/*
TODO: File type check
*/
