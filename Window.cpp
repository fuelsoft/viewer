/*
WINDOW.CPP
NICK WILSON
2018 - 2020
*/

#include "Window.hpp"

/* PUBLIC */

Window::Window(int w, int h, int x, int y, uint32_t flags) {
	window = SDL_CreateWindow("Untitled Window", x, y, w, h, flags);

	this->w = w;
	this->h = h;
	this->x = x;
	this->y = y;

	this->pw = w;
	this->ph = h;
	this->px = x;
	this->py = y;

	if (window != NULL) {
		surface = SDL_GetWindowSurface(window);
	}

	SDL_UpdateWindowSurface(window);

	if (!SDL_GetRenderer(window)) {
		/* Framerate matching handled in main loop now, so vsync is disabled */
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC*/);
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

void Window::updateWindowPos() {
	px = x;
	py = y;
	SDL_GetWindowPosition(window, &x, &y);
}

void Window::updateWindowSize() {
	pw = w;
	ph = h;
	SDL_GetWindowSize(window, &w, &h);
}
