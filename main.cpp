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
#include <fstream>
#include <filesystem>
#include <vector>

#include <Windows.h>

/* /// CONSTANTS /// */

/* LOGGING */
const std::string APPLICATION_TITLE = "Image Viewer";
const std::string LOG_ERROR = "<ERROR> ";
const std::string LOG_WARNING = "<WARNING> ";
const std::string LOG_NOTICE = "<NOTICE> ";

/* WINDOW MANAGEMENT */
const int WINDOW_WIDTH_DEFAULT = 1600;
const int WINDOW_HEIGHT_DEFAULT = 900;

int WINDOW_X = SDL_WINDOWPOS_UNDEFINED;
int WINDOW_Y = SDL_WINDOWPOS_UNDEFINED;
int WINDOW_PREVIOUS_X = SDL_WINDOWPOS_UNDEFINED;
int WINDOW_PREVIOUS_Y = SDL_WINDOWPOS_UNDEFINED;

int WINDOW_WIDTH = WINDOW_WIDTH_DEFAULT;
int WINDOW_HEIGHT = WINDOW_HEIGHT_DEFAULT;
int WINDOW_PREVIOUS_WIDTH = WINDOW_WIDTH_DEFAULT;
int WINDOW_PREVIOUS_HEIGHT = WINDOW_HEIGHT_DEFAULT;

bool WINDOW_MAXIMIZED = 0;
bool WINDOW_MOVED = 0;

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

/* SETTINGS */
std::filesystem::path PATH_PROGRAM_CWD;
std::filesystem::path PATH_FILE_IMAGE;

const std::string FILENAME_SETTINGS = "settings.cfg";
const int settingDataSize = sizeof(WINDOW_X)
							+ sizeof(WINDOW_Y)
							+ sizeof(WINDOW_WIDTH)
							+ sizeof(WINDOW_HEIGHT)
							+ sizeof(WINDOW_MAXIMIZED);

std::vector<std::filesystem::path> FILES_ADJACENT_IMAGES;
uint32_t IMAGE_FILE_INDEX = 0;

/* From sdl_image documentation */
/* Less common formats omitted */
/* TODO: Remove uppercase duplicates */
const std::string SUPPORTED_FILETYPES[] = {
	".jpg",
	".jpeg",
	".png",
	".gif",
	".bmp",
	".tif",
	".tiff",
	".tga",
	".JPG",
	".JPEG",
	".PNG",
	".GIF",
	".BMP",
	".TIF",
	".TIFF",
	".TGA",
};

/* /// CODE /// */

/**
* readSettings - Load settings file if possible and recover window position and size.
*/
void readSettings() {
	//if CWD is length 0, program was run from terminal and is CWD is unknown. Use default settings.
	if (PATH_PROGRAM_CWD.empty()) {
		std::cerr << LOG_NOTICE << "Run Viewer using full path to load settings file. Using defaults." << std::endl;
		return;
	}
	std::ifstream inputStream(PATH_PROGRAM_CWD / FILENAME_SETTINGS, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	if (inputStream.is_open()) {
		if (inputStream.tellg() >= settingDataSize) {
			inputStream.seekg(std::ios_base::beg);
			inputStream.read(reinterpret_cast<char *>(&WINDOW_X), sizeof(WINDOW_X));
			inputStream.read(reinterpret_cast<char *>(&WINDOW_Y), sizeof(WINDOW_Y));
			inputStream.read(reinterpret_cast<char *>(&WINDOW_WIDTH), sizeof(WINDOW_WIDTH));
			inputStream.read(reinterpret_cast<char *>(&WINDOW_HEIGHT), sizeof(WINDOW_HEIGHT));
			inputStream.read(reinterpret_cast<char *>(&WINDOW_MAXIMIZED), sizeof(WINDOW_MAXIMIZED));
		}
		else {
			std::cerr << LOG_WARNING << "Incorrect settings data size!" << std::endl;
		}
		inputStream.close();
	}
	else { //load defaults
		std::cerr << LOG_NOTICE << "No settings file located!" << std::endl;
	}
}

/**
* writeSettings - Collect window settings and write to settings file, if it can be written in a valid location
*/
void writeSettings() {
	//working directory wasn't set, don't write anything
	if (PATH_PROGRAM_CWD.empty()) return;
	std::ofstream outputStream(PATH_PROGRAM_CWD / FILENAME_SETTINGS, std::ifstream::out | std::ifstream::binary | std::ifstream::trunc);
	if (outputStream.is_open()) {
		//if window was 'moved' during resize, discard
		if (!WINDOW_MOVED) {
			outputStream.write(reinterpret_cast<char *>(&WINDOW_PREVIOUS_X), sizeof(WINDOW_PREVIOUS_X));
			outputStream.write(reinterpret_cast<char *>(&WINDOW_PREVIOUS_Y), sizeof(WINDOW_PREVIOUS_Y));
		}
		//if window was actually moved, record it
		else {
			outputStream.write(reinterpret_cast<char *>(&WINDOW_X), sizeof(WINDOW_X));
			outputStream.write(reinterpret_cast<char *>(&WINDOW_Y), sizeof(WINDOW_Y));
		}
		//if the window is maximized, write the last known size before it was maximized
		if (WINDOW_MAXIMIZED) {
			outputStream.write(reinterpret_cast<char *>(&WINDOW_PREVIOUS_WIDTH), sizeof(WINDOW_PREVIOUS_WIDTH));
			outputStream.write(reinterpret_cast<char *>(&WINDOW_PREVIOUS_HEIGHT), sizeof(WINDOW_PREVIOUS_HEIGHT));
		}
		//otherwise, write current size
		else {
			outputStream.write(reinterpret_cast<char *>(&WINDOW_WIDTH), sizeof(WINDOW_WIDTH));
			outputStream.write(reinterpret_cast<char *>(&WINDOW_HEIGHT), sizeof(WINDOW_HEIGHT));
		}
		//write window state
		outputStream.write(reinterpret_cast<char *>(&WINDOW_MAXIMIZED), sizeof(WINDOW_MAXIMIZED));

		outputStream.close();
	}
	else {
		std::cerr << LOG_ERROR << "Could not write to settings file!" << std::endl;
	}
}

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

			SDL_Rect windowDestination = {xPos, yPos, (int) (WINDOW_WIDTH * VIEWPORT_ZOOM), (int) (imageTargetHeight * VIEWPORT_ZOOM)};

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

			SDL_Rect windowDestination = {xPos, yPos, (int) (imageTargetWidth * VIEWPORT_ZOOM), (int) (WINDOW_HEIGHT * VIEWPORT_ZOOM)};
			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
	}
	//image will fit in existing window
	else {
		int xPos = (WINDOW_WIDTH - IMAGE_WIDTH * VIEWPORT_ZOOM)/2 + VIEWPORT_X;
		int yPos = (WINDOW_HEIGHT - IMAGE_HEIGHT * VIEWPORT_ZOOM)/2 + VIEWPORT_Y;
		SDL_Rect windowDestination = {xPos, yPos, (int) (IMAGE_WIDTH * VIEWPORT_ZOOM), (int) (IMAGE_HEIGHT * VIEWPORT_ZOOM)};
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
* loadTextureFromFile	- Load a file and convert it to an SDL_Texture
* filePath 				> The path to the image to load
* win					> Target Window object
* imageTexture 			> Image as SDL_Texture
*/
int loadTextureFromFile(std::string filePath, Window* win, SDL_Texture** imageTexture) {
	//try loading image from filename
	SDL_Surface* imageSurface = IMG_Load(filePath.c_str());

	//image load call returned null, indicating failure
	if (!imageSurface) {
		std::cerr << LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	IMAGE_HEIGHT = imageSurface->h;
	IMAGE_WIDTH = imageSurface->w;

	//clear old texture
	SDL_DestroyTexture(*imageTexture);

	//create hardware accelerated texture from image data and free surface
	*imageTexture = SDL_CreateTextureFromSurface(win->renderer, imageSurface);

	SDL_FreeSurface(imageSurface);
	return 0;
}

/**
* resetViewport - It was a bit redundant pasting the same 3 lines over and over
*/
void resetViewport() {
	VIEWPORT_ZOOM = 1.0;
	VIEWPORT_X = 0;
	VIEWPORT_Y = 0;
}

/**
* draw			- Clear display, then draw tiles and image (if provided)
* win 			> Target Window object
* tileTexture 	> Texture as SDL_Texture to tile
* imageTexture 	> Image as SDL_Texture
*/
void draw(Window* win, SDL_Texture* tile_texture, SDL_Texture* image_texture) {
	SDL_RenderClear(win->renderer);
	if (tile_texture) drawTileTexture(win, tile_texture);
	if (image_texture) redrawImage(win, image_texture);
	SDL_RenderPresent(win->renderer);
}

/**
* formatSupport	- Compare file extension provided to known list to confirm support
* extension 	> File extension as String
* return - int 	< Index of extension in list or -1 if not found
*/
int formatSupport(std::string extension) {
	for (uint8_t i = 0; i < std::size(SUPPORTED_FILETYPES); i++) {
		if (!extension.compare(SUPPORTED_FILETYPES[i])) return (int) i;
	}
	return -1;
}

/**
* main	- Setup, load image, then manage user input
*/
int main(int argc, char* argv[]) {
	bool quit = false;

	SDL_Event sdlEvent;
	SDL_Texture* imageTexture = nullptr;

	/* Confirm video is available and set up */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << LOG_ERROR << "SDL COULD NOT BE INITIALIZED!" << std::endl;
		return 1;
	}

	char EXE_PATH[MAX_PATH];
	GetModuleFileNameA(NULL, EXE_PATH, MAX_PATH); //Windows system call to get executable's path
	PATH_PROGRAM_CWD = std::filesystem::path(EXE_PATH).parent_path(); //Collect parent folder path for CWD

	readSettings();

	/* Create invisible application window */
	Window win(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_X, WINDOW_Y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (WINDOW_MAXIMIZED ? SDL_WINDOW_MAXIMIZED : 0));
	win.setTitle(APPLICATION_TITLE.c_str());

	//no file passed in
	if (argc < 2) {
		std::cerr << LOG_ERROR << "No filename provided!" << std::endl;
		return 1;
	}

	//try to improve zoom quality by improving sampling technique
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cout << LOG_NOTICE << "Set filtering to linear." << std::endl;
	}
	//note failure and continue with default sampling
	else {
		std::cout << LOG_NOTICE << "Sampling defaulting to nearest neighbour." << std::endl;
	}

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

	//draw background texture before continuing to show program is loading
	draw(&win, TEXTURE_TRANSPARENCY, nullptr);

	//load up the image passed in
	if (loadTextureFromFile(std::string(argv[1]), &win, &imageTexture)) {
		std::cerr << LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	//determine image filename
	PATH_FILE_IMAGE = std::filesystem::path(argv[1]);
	//update window title with image filename
	win.setTitle((PATH_FILE_IMAGE.filename().string() + " - " + APPLICATION_TITLE).c_str());

	//finally draw image and background
	draw(&win, TEXTURE_TRANSPARENCY, imageTexture);

	//iterate image folder and mark position of currently open image
	int pos = 0;
	for(auto& entry : std::filesystem::directory_iterator(PATH_FILE_IMAGE.parent_path())) {
		//test that file is real file not link, folder, etc. and check it is a supported image format
		if (std::filesystem::is_regular_file(entry) && (formatSupport(entry.path().extension().string()) >= 0)) {
			//add file to list of images
			FILES_ADJACENT_IMAGES.push_back(entry.path());
			//if this is the image currently open, record position
			if (entry.path() == PATH_FILE_IMAGE) IMAGE_FILE_INDEX = pos;
			pos++;
		}
	}

	std::cout << LOG_NOTICE << "Found " << FILES_ADJACENT_IMAGES.size() << " images adjacent." << std::endl;

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
						case SDL_WINDOWEVENT_MOVED:
							WINDOW_MOVED = true;
							WINDOW_PREVIOUS_X = WINDOW_X;
							WINDOW_PREVIOUS_Y = WINDOW_Y;
							SDL_GetWindowPosition(win.window, &WINDOW_X, &WINDOW_Y);
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							WINDOW_MOVED = false;
							WINDOW_MAXIMIZED = true;
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							WINDOW_MAXIMIZED = false;
							WINDOW_PREVIOUS_WIDTH = WINDOW_WIDTH;
							WINDOW_PREVIOUS_HEIGHT = WINDOW_HEIGHT;
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
							resetViewport();
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
							break;
						case SDLK_ESCAPE:
							quit = true;
							break;
						//previous image
						case SDLK_LEFT:
							//move back
							if (IMAGE_FILE_INDEX == 0) IMAGE_FILE_INDEX = FILES_ADJACENT_IMAGES.size() - 1; //loop back to end of image file list
							else IMAGE_FILE_INDEX--;
							//draw(&win, TEXTURE_TRANSPARENCY, nullptr); //draw checkerboard over old image while we wait
							win.setTitle((FILES_ADJACENT_IMAGES[IMAGE_FILE_INDEX].filename().string() + " - " + APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(FILES_ADJACENT_IMAGES[IMAGE_FILE_INDEX].string(), &win, &imageTexture)) { //if call returned non-zero, there was an error
								std::cerr << LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture); //draw new image
							break;
						//next image
						case SDLK_RIGHT:
							IMAGE_FILE_INDEX++; //move forward
							if (IMAGE_FILE_INDEX >= FILES_ADJACENT_IMAGES.size()) IMAGE_FILE_INDEX = 0; //loop back to start of image file list
							//draw(&win, TEXTURE_TRANSPARENCY, nullptr); //draw checkerboard over old image while we wait
							win.setTitle((FILES_ADJACENT_IMAGES[IMAGE_FILE_INDEX].filename().string() + " - " + APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(FILES_ADJACENT_IMAGES[IMAGE_FILE_INDEX].string(), &win, &imageTexture)) { //if call returned non-zero, there was an error
								std::cerr << LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture); //draw new image
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
					if (sdlEvent.wheel.y > 0) {
						VIEWPORT_ZOOM = std::min(ZOOM_MAX, VIEWPORT_ZOOM * ZOOM_SCROLL_SENSITIVITY);

						/* Correct positioning.
						 * Required as otherwise zoom functions as [center zoom + offset]
						 *  instead of [zoom on offset from center] */
						VIEWPORT_X *= ZOOM_SCROLL_SENSITIVITY;
						VIEWPORT_Y *= ZOOM_SCROLL_SENSITIVITY;

						draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
					}
					else if (sdlEvent.wheel.y < 0) {
						VIEWPORT_ZOOM = std::max(ZOOM_MIN, VIEWPORT_ZOOM / ZOOM_SCROLL_SENSITIVITY);

						//correct positioning
						VIEWPORT_X /= ZOOM_SCROLL_SENSITIVITY;
						VIEWPORT_Y /= ZOOM_SCROLL_SENSITIVITY;

						draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (sdlEvent.button.button) {
						case SDL_BUTTON_LEFT:
							MOUSE_CLICK_STATE_LEFT = true;
							SDL_GetMouseState(&mouseX, &mouseY);
							break;
						case SDL_BUTTON_MIDDLE:
							resetViewport();
							draw(&win, TEXTURE_TRANSPARENCY, imageTexture);
							break;
						case SDL_BUTTON_X1:
							//bind mouse back button to left arrow to return to previous image
							SDL_Event sdlEBack;
							sdlEBack.type = SDL_KEYDOWN;
							sdlEBack.key.keysym.sym = SDLK_LEFT;
							SDL_PushEvent(&sdlEBack);
							break;
						case SDL_BUTTON_X2:
							//bind mouse next button to right arrow to advance to next image
							SDL_Event sdlENext;
							sdlENext.type = SDL_KEYDOWN;
							sdlENext.key.keysym.sym = SDLK_RIGHT;
							SDL_PushEvent(&sdlENext);
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
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	writeSettings();

	return 0;
}

/* TODO:
* Add key to set zoom to 1:1 pixel ratio
* Possible: partial image metadata? PNG image data support from ProjectPNG?
* Animated GIFs -> https://stackoverflow.com/questions/36267833/c-sdl2-how-do-i-play-a-gif-in-sdl2/36410301#36410301
* Zoom on mouse instead of window center
* Stop image from being moved off-screen
* Possible: Image deletion?
* Possible: Image rotation?
* Cursor icon state updates?
* Don't exit if passed no parameters... show pop-up?
* Handle capitalization in file extensions
*/

/* KNOWN PROBLEMS:
* ICO: Files stored as "NEW PNG" type do not load with SDL_image
* ICO: Files with partial transparent pixels do not render correctly
* GIF: Animated sequences only load the first frame
*/
