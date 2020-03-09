/*
WINDOW.CPP
NICK WILSON
2018 - 2020
*/

#include "Window.hpp"

/* PUBLIC */

Window::Window(int w, int h, int x, int y, bool visible) {
	window = SDL_CreateWindow("Untitled Window", x, y, w, h, ((visible) ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN) | SDL_WINDOW_RESIZABLE);

	if (window != NULL) {
		surface = SDL_GetWindowSurface(window);
	}

	SDL_UpdateWindowSurface(window);

	if (!SDL_GetRenderer(window)) {
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	}
	else {
		renderer = SDL_GetRenderer(window);
	}
}

Window::~Window() {
	SDL_DestroyWindow(window);
}

/*
Set the title of the window
*/
void Window::setTitle(std::string title) {
	SDL_SetWindowTitle(window, title.c_str());
}

/*
Return the currently set window title
*/
std::string Window::getTitle() {
	return SDL_GetWindowTitle(window);
}

/*
Get the window's surface's pixels.
Read and write.
*/
void* Window::getPixels() {
	return surface->pixels;
}

/*
Useful for dumping surface to either debug or save the contents.
*/
SDL_Surface* Window::getSurface() {
	return surface;
}

/*
Overwrite the window contents with the provided colour.
*/
void Window::clear(int r, int g, int b) {
	if (renderer != nullptr) { //accelerated mode
		SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
	}
	else { //default mode
		SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, r, g, b));
	}
}

void Window::hide() {
	SDL_HideWindow(window);
}

void Window::show() {
	SDL_ShowWindow(window);
}

void Window::minimize() {
    SDL_MinimizeWindow(window);
}

void Window::maximize() {
    SDL_MaximizeWindow(window);
}

void Window::resize(int x, int y) {
    SDL_SetWindowSize(window, x, y);
}
