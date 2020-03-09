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
	SDL_Window* window = nullptr;
	SDL_Surface* surface = nullptr;
	SDL_Renderer* renderer = nullptr;

	Window() {}

	Window(int w, int h, int x = SDL_WINDOWPOS_UNDEFINED, int y = SDL_WINDOWPOS_UNDEFINED, bool visible = true);

	~Window();

	void setTitle(std::string title);

	std::string getTitle();

	void* getPixels();

	SDL_Surface* getSurface();

	void clear(int r, int g, int b);

	void hide();

	void show();

	void minimize();

	void maximize();

	void resize(int x, int y);
};


#endif
