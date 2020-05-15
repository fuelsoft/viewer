/*
WINDOW.HPP
NICK WILSON
2018 - 2020
*/

#include <SDL2/SDL.h>

#include <cstdint>		//standard number formats
#include <string>		//string type

#ifndef WINDOWOBJ_H
#define WINDOWOBJ_H

class Window {

private:

public:
	int x, y, w, h;
	int px, py, pw, ph;

	SDL_Window* window = nullptr;
	SDL_Surface* surface = nullptr;
	SDL_Renderer* renderer = nullptr;

	Window() {}

	Window(int w, int h, int x = SDL_WINDOWPOS_UNDEFINED, int y = SDL_WINDOWPOS_UNDEFINED, uint32_t flags = 0);

	~Window();

	void setTitle(std::string title);

	void updateWindowPos();

	void updateWindowSize();
};

#endif
