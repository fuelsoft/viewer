/*
VIEWER - MAIN.CPP
NICK WILSON
2020
*/

/* /// IMPORTS /// */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "Window.hpp"

#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

/* /// CONSTANTS /// */

/* LOGGING */
const std::string APPLICATION_TITLE = "Image Viewer";
const std::string LOG_ERROR = "<ERROR> ";
const std::string LOG_WARNING = "<WARNING> ";
const std::string LOG_NOTICE = "<NOTICE> ";

/* WINDOW MANAGEMENT */
const int WINDOW_WIDTH_DEFAULT = 1600;
const int WINDOW_HEIGHT_DEFAULT = 900;

int DISPLAY_WIDTH = 0;
int DISPLAY_HEIGHT = 0;

int WINDOW_WIDTH = 0;
int WINDOW_HEIGHT = 0;

/* IMAGE DISPLAY */
int IMAGE_WIDTH = 0;
int IMAGE_HEIGHT = 0;

float VIEWPORT_ZOOM = 1.0f;
int VIEWPORT_X = 0;
int VIEWPORT_Y = 0;

const float ZOOM_MIN = (1.0/16.0);  //-4x zoom
const float ZOOM_MAX = 256;         // 8x zoom
const float ZOOM_SCROLL_SENSITIVITY = 1.2;

const int TEXTURE_CHECKERBOARD_RESOLUTION = 32;
const uint32_t COLOUR_CHECKERBOARD_LIGHT = 0x00141414;
const uint32_t COLOUR_CHECKERBOARD_DARK = 0x000D0D0D;
SDL_Texture* TEXTURE_TRANSPARENCY;

/* INPUT */
bool MOUSE_CLICK_STATE_LEFT = false;

/* /// CODE /// */

/**
* redrawImage	- Re-render the display image, respecting zoom and pan positioning
* win 			> Target Window object
* imageTexture 	> Image as SDL_Texture
*/
void redrawImage(Window* win, SDL_Texture* imageTexture) {

	//image is too big for the window
	if (IMAGE_HEIGHT > WINDOW_HEIGHT || IMAGE_WIDTH > WINDOW_WIDTH) {
		//determine the shapes of window and image
		float imageAspectRatio = IMAGE_WIDTH/(float) IMAGE_HEIGHT;
		float windowAspectRatio = WINDOW_WIDTH/(float) WINDOW_HEIGHT;

		//width is priority
		if (imageAspectRatio > windowAspectRatio) {
			//figure out how image will be scaled to fit
			float imageReduction = WINDOW_WIDTH/(float) IMAGE_WIDTH;
			//apply transformation to height
			int imageTargetHeight = imageReduction * IMAGE_HEIGHT;

			//horizontal adjustment
			int xPos = (WINDOW_WIDTH - WINDOW_WIDTH * VIEWPORT_ZOOM)/2 + VIEWPORT_X;
			//vertical adjustment
			int yPos = (WINDOW_HEIGHT - imageTargetHeight * VIEWPORT_ZOOM)/2 + VIEWPORT_Y;

			SDL_Rect windowDestination = {xPos, yPos, WINDOW_WIDTH * VIEWPORT_ZOOM, imageTargetHeight * VIEWPORT_ZOOM};

			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
		//height is priority or equal priority
		else {
			//figure out how image will be scaled to fit
			float imageReduction = WINDOW_HEIGHT/(float) IMAGE_HEIGHT;
			//apply transformation to width
			int imageTargetWidth = imageReduction * IMAGE_WIDTH;

			//horizontal adjustment
			int xPos = (WINDOW_WIDTH - imageTargetWidth * VIEWPORT_ZOOM)/2 + VIEWPORT_X;
			//vertical adjustment
			int yPos = (WINDOW_HEIGHT - WINDOW_HEIGHT * VIEWPORT_ZOOM)/2 + VIEWPORT_Y;

			SDL_Rect windowDestination = {xPos, yPos, imageTargetWidth * VIEWPORT_ZOOM, WINDOW_HEIGHT * VIEWPORT_ZOOM};
			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
	}
	//image will fit in existing window
	else {
		int xPos = (WINDOW_WIDTH - IMAGE_WIDTH * VIEWPORT_ZOOM)/2 + VIEWPORT_X;
		int yPos = (WINDOW_HEIGHT - IMAGE_HEIGHT * VIEWPORT_ZOOM)/2 + VIEWPORT_Y;
		SDL_Rect windowDestination = {xPos, yPos, IMAGE_WIDTH * VIEWPORT_ZOOM, IMAGE_HEIGHT * VIEWPORT_ZOOM};
		SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
	}
}

/**
* drawTileTexture	- Tile texture across window
* win 				> Target Window object
* tileTexture 		> Texture as SDL_Texture to tile
*/
void drawTileTexture(Window* win, SDL_Texture* tileTexture) {
	for (int h = 0; h < WINDOW_HEIGHT; h += TEXTURE_CHECKERBOARD_RESOLUTION){
		for (int w = 0; w < WINDOW_WIDTH; w += TEXTURE_CHECKERBOARD_RESOLUTION){
			SDL_Rect destination = {w, h, TEXTURE_CHECKERBOARD_RESOLUTION, TEXTURE_CHECKERBOARD_RESOLUTION};
			SDL_RenderCopy(win->renderer, tileTexture, 0, &destination);
		}
	}
}

/**
* draw			- Clear display, then draw tiles and image
* win 			> Target Window object
* tileTexture 	> Texture as SDL_Texture to tile
* imageTexture 	> Image as SDL_Texture
*/
void draw(Window* win, SDL_Texture* tile_texture, SDL_Texture* image_texture) {
	SDL_RenderClear(win->renderer);
	drawTileTexture(win, tile_texture);
	redrawImage(win, image_texture);
	SDL_RenderPresent(win->renderer);
}

/**
* main	- Setup, load image, then manage user input
*/
int main(int argc, char * argv[]) {
	bool quit = false;
	int windowDisplayIndex;

	SDL_Event sdlEvent;
	SDL_DisplayMode displayMode;
	SDL_Surface* imageSurface;
	SDL_Texture* imageTexture;

	/* Confirm video is available and set up */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << LOG_ERROR << "SDL COULD NOT BE INITIALIZED!" << std::endl;
		return 1;
	}

	/* Create invisible application window */
	Window win(WINDOW_WIDTH_DEFAULT, WINDOW_HEIGHT_DEFAULT);
	win.setTitle(APPLICATION_TITLE.c_str());

	windowDisplayIndex = SDL_GetWindowDisplayIndex(win.window);
	if (SDL_GetCurrentDisplayMode(windowDisplayIndex, &displayMode)) {
		std::cerr << LOG_NOTICE << "Could not load display properties!" << std::endl;
		return 1;
	}

	DISPLAY_WIDTH = displayMode.w;
	DISPLAY_HEIGHT = displayMode.h;
	WINDOW_WIDTH = WINDOW_WIDTH_DEFAULT;
	WINDOW_HEIGHT = WINDOW_HEIGHT_DEFAULT;

	//no file passed in
	if (argc < 2) {
		std::cerr << LOG_ERROR << "No filename provided!" << std::endl;
		return 1;
	}

	//try to improve zoom quality by improving sampling technique
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cout << LOG_NOTICE << "Set filtering to linear" << std::endl;
	}
	//note failure and continue with default sampling
	else {
		std::cout << LOG_NOTICE << "Sampling defaulting to nearest neighbour" << std::endl;
	}

	//try loading image from filename
	imageSurface = IMG_Load(argv[1]);

	//image load call returned null, indicating failure
	if (!imageSurface) {
		std::cerr << LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	IMAGE_HEIGHT = imageSurface->h;
	IMAGE_WIDTH = imageSurface->w;

	//create hardware accelerated texture from image data and free surface
	imageTexture = SDL_CreateTextureFromSurface(win.renderer, imageSurface);
	SDL_FreeSurface(imageSurface);

	//create checkerboard background texture for transparent images
	SDL_Surface* transparency_tmp = SDL_CreateRGBSurface(0, TEXTURE_CHECKERBOARD_RESOLUTION, TEXTURE_CHECKERBOARD_RESOLUTION, 32, 0, 0, 0, 0);
	uint32_t* pixeldata = (uint32_t*) transparency_tmp->pixels;
	for (int h = 0; h < TEXTURE_CHECKERBOARD_RESOLUTION; h++) {
		for (int w = 0; w < TEXTURE_CHECKERBOARD_RESOLUTION; w++) {
			int p = TEXTURE_CHECKERBOARD_RESOLUTION / 2;
			(*(pixeldata + h * TEXTURE_CHECKERBOARD_RESOLUTION + w)) = (((h < p) ^ (w < p)) ? COLOUR_CHECKERBOARD_LIGHT : COLOUR_CHECKERBOARD_DARK);
		}
	}

	TEXTURE_TRANSPARENCY = SDL_CreateTextureFromSurface(win.renderer, transparency_tmp);
	SDL_FreeSurface(transparency_tmp);

	draw(&win, TEXTURE_TRANSPARENCY, imageTexture);

	int mouseX;
	int mouseY;
	int mousePreviousX;
	int mousePreviousY;

	//While application is running
	while (!quit) {
		//Handle events on queue
		while (SDL_PollEvent(&sdlEvent) != 0) {
			switch (sdlEvent.type) {
				//User requests quit
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_WINDOWEVENT:
					//User requests quit
					switch (sdlEvent.window.event) {
						case SDL_WINDOWEVENT_CLOSE:
							quit = true;
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							SDL_GetWindowSize(win.window, &WINDOW_WIDTH, &WINDOW_HEIGHT);
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
							break;
					}
					break;
				case SDL_KEYDOWN:
					switch (sdlEvent.key.keysym.sym) {
						case SDLK_KP_PLUS: //keypad +, zoom in
							VIEWPORT_ZOOM = std::min(ZOOM_MAX, VIEWPORT_ZOOM * 2.0f);
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
							break;
						case SDLK_KP_MINUS: //keypad -, zoom out
							VIEWPORT_ZOOM = std::max(ZOOM_MIN, VIEWPORT_ZOOM/2.0f);
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
							break;
						case SDLK_KP_0: //keypad 0, reset zoom and positioning
							VIEWPORT_ZOOM = 1.0;
							VIEWPORT_X = 0;
							VIEWPORT_Y = 0;
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
					{
						int yScrollVal = sdlEvent.wheel.y;
						if (yScrollVal > 0) {
							VIEWPORT_ZOOM = std::min(ZOOM_MAX, VIEWPORT_ZOOM * ZOOM_SCROLL_SENSITIVITY);

							/* Correct positioning.
							 * Required as otherwise zoom functions as [center zoom + offset]
							 *  instead of [zoom on offset from center] */
							VIEWPORT_X *= ZOOM_SCROLL_SENSITIVITY;
							VIEWPORT_Y *= ZOOM_SCROLL_SENSITIVITY;

							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
						}
						else if (yScrollVal < 0) {
							VIEWPORT_ZOOM = std::max(ZOOM_MIN, VIEWPORT_ZOOM / ZOOM_SCROLL_SENSITIVITY);

							//correct positioning
							VIEWPORT_X /= ZOOM_SCROLL_SENSITIVITY;
							VIEWPORT_Y /= ZOOM_SCROLL_SENSITIVITY;

							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (sdlEvent.button.button) {
						case SDL_BUTTON_LEFT:
							MOUSE_CLICK_STATE_LEFT = true;
							SDL_GetMouseState(&mouseX, &mouseY);
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch (sdlEvent.button.button) {
						case SDL_BUTTON_LEFT:
							MOUSE_CLICK_STATE_LEFT = false;
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					if (MOUSE_CLICK_STATE_LEFT) {
						mousePreviousX = mouseX;
						mousePreviousY = mouseY;
						SDL_GetMouseState(&mouseX, &mouseY);
						VIEWPORT_X += mouseX - mousePreviousX;
						VIEWPORT_Y += mouseY - mousePreviousY;
						draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
					}
					break;

			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	return 0;
}

/*
TODO:
* Remember window size and position
* Add key to set zoom to 1:1 pixel ratio
* Add key to change sampling mode
* Filename in title
* Transparent background texture
* Possible: partial image metadata? PNG image data support from ProjectPNG?
* Navigate forward/backwards in current directory
* Animated GIFs -> https://stackoverflow.com/questions/36267833/c-sdl2-how-do-i-play-a-gif-in-sdl2/36410301#36410301
* Zoom on mouse instead of window center
* Stop image from being moved off-screen
*/
