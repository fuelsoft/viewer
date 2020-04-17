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

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

/* /// CONSTANTS /// */

namespace IVCONST {

	/* LOGGING */
	const std::string APPLICATION_TITLE = "Image Viewer";
	const std::string LOG_ERROR = "<ERROR> ";
	const std::string LOG_WARNING = "<WARNING> ";
	const std::string LOG_NOTICE = "<NOTICE> ";

	/* WINDOW MANAGEMENT */
	const int WINDOW_WIDTH_DEFAULT = 1600;
	const int WINDOW_HEIGHT_DEFAULT = 900;

	/* DISPLAY SETTINGS */
	const float ZOOM_MIN = (1.0/16.0);  //-4x zoom
	const float ZOOM_MAX = 256;         // 8x zoom
	const float ZOOM_SCROLL_SENSITIVITY = 1.2;

	const int TEXTURE_CHECKERBOARD_RESOLUTION = 32;
	const uint32_t CHECKERBOARD_DARK_LIGHT = 0x00141414;
	const uint32_t CHECKERBOARD_DARK_DARK = 0x000D0D0D;
	const uint32_t CHECKERBOARD_LIGHT_LIGHT = 0x00F3F3F3;
	const uint32_t CHECKERBOARD_LIGHT_DARK = 0x00DEDEDE;

	const std::string FILENAME_SETTINGS = "settings.cfg";
	const int SETTINGDATASIZE = sizeof(int)
								+ sizeof(int)
								+ sizeof(int)
								+ sizeof(int)
								+ sizeof(bool)
								+ sizeof(bool);

	/* From sdl_image documentation */
	/* Less common formats omitted */
	/* TODO: Remove uppercase duplicates */
	const std::string SUPPORTED_FILETYPES[] = {
		".jpg",
		".jpeg",
		".JPG",
		".JPEG",
		".png",
		".PNG",
		".gif",
		".GIF",
		".bmp",
		".BMP",
		".tif",
		".tiff",
		".tga",
		".TIF",
		".TIFF",
		".TGA",
	};
}

/* /// GLOBALS /// */

namespace IVGLOBAL {

	int WINDOW_X = SDL_WINDOWPOS_UNDEFINED;
	int WINDOW_Y = SDL_WINDOWPOS_UNDEFINED;
	int WINDOW_PREVIOUS_X = SDL_WINDOWPOS_UNDEFINED;
	int WINDOW_PREVIOUS_Y = SDL_WINDOWPOS_UNDEFINED;

	int WINDOW_WIDTH = IVCONST::WINDOW_WIDTH_DEFAULT;
	int WINDOW_HEIGHT = IVCONST::WINDOW_HEIGHT_DEFAULT;
	int WINDOW_PREVIOUS_WIDTH = IVCONST::WINDOW_WIDTH_DEFAULT;
	int WINDOW_PREVIOUS_HEIGHT = IVCONST::WINDOW_HEIGHT_DEFAULT;

	bool WINDOW_MAXIMIZED = 0;
	bool WINDOW_MOVED = 0;

	/* DISPLAY */
	int IMAGE_WIDTH = 0;
	int IMAGE_HEIGHT = 0;

	int VIEWPORT_X = 0;
	int VIEWPORT_Y = 0;
	float VIEWPORT_ZOOM = 1.0f;

	bool DISPLAY_MODE_DARK = true;

	SDL_Texture* TEXTURE_TRANSPARENCY_DARK;
	SDL_Texture* TEXTURE_TRANSPARENCY_LIGHT;

	/* INPUT */
	bool MOUSE_CLICK_STATE_LEFT = false;

	/* SETTINGS */
	std::filesystem::path PATH_PROGRAM_CWD;
	std::filesystem::path PATH_FILE_IMAGE;

	std::vector<std::filesystem::path> FILES_ADJACENT_IMAGES;
	uint32_t IMAGE_FILE_INDEX = 0;

}

/* /// CODE /// */

/**
* readSettings - Load settings file if possible and recover window position and size.
*/
void readSettings() {
	//if CWD is length 0, program was run from terminal and is CWD is unknown. Use default settings.
	if (IVGLOBAL::PATH_PROGRAM_CWD.empty()) {
		std::cerr << IVCONST::LOG_NOTICE << "Unable to load settings file. Using defaults." << std::endl;
		return;
	}
	std::ifstream inputStream(IVGLOBAL::PATH_PROGRAM_CWD / IVCONST::FILENAME_SETTINGS, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	if (inputStream.is_open()) {
		if (inputStream.tellg() >= IVCONST::SETTINGDATASIZE) {
			inputStream.seekg(std::ios_base::beg);
			inputStream.read(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_X), sizeof(IVGLOBAL::WINDOW_X));
			inputStream.read(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_Y), sizeof(IVGLOBAL::WINDOW_Y));
			inputStream.read(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_WIDTH), sizeof(IVGLOBAL::WINDOW_WIDTH));
			inputStream.read(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_HEIGHT), sizeof(IVGLOBAL::WINDOW_HEIGHT));
			inputStream.read(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_MAXIMIZED), sizeof(IVGLOBAL::WINDOW_MAXIMIZED));
			inputStream.read(reinterpret_cast<char *>(&IVGLOBAL::DISPLAY_MODE_DARK), sizeof(IVGLOBAL::DISPLAY_MODE_DARK));
		}
		else {
			std::cerr << IVCONST::LOG_WARNING << "Incorrect settings data size!" << std::endl;
		}
		inputStream.close();
	}
	else { //load defaults
		std::cerr << IVCONST::LOG_NOTICE << "No settings file located!" << std::endl;
	}
}

/**
* writeSettings - Collect window settings and write to settings file, if it can be written in a valid location
*/
void writeSettings() {
	//working directory wasn't set, don't write anything
	if (IVGLOBAL::PATH_PROGRAM_CWD.empty()) return;
	std::ofstream outputStream(IVGLOBAL::PATH_PROGRAM_CWD / IVCONST::FILENAME_SETTINGS, std::ifstream::out | std::ifstream::binary | std::ifstream::trunc);
	if (outputStream.is_open()) {
		//if window was 'moved' during resize, discard
		if (!IVGLOBAL::WINDOW_MAXIMIZED) {
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_PREVIOUS_X), sizeof(IVGLOBAL::WINDOW_PREVIOUS_X));
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_PREVIOUS_Y), sizeof(IVGLOBAL::WINDOW_PREVIOUS_Y));
		}
		//if window was actually moved, record it
		else {
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_X), sizeof(IVGLOBAL::WINDOW_X));
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_Y), sizeof(IVGLOBAL::WINDOW_Y));
		}
		//if the window is maximized, write the last known size before it was maximized
		if (IVGLOBAL::WINDOW_MAXIMIZED) {
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_PREVIOUS_WIDTH), sizeof(IVGLOBAL::WINDOW_PREVIOUS_WIDTH));
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_PREVIOUS_HEIGHT), sizeof(IVGLOBAL::WINDOW_PREVIOUS_HEIGHT));
		}
		//otherwise, write current size
		else {
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_WIDTH), sizeof(IVGLOBAL::WINDOW_WIDTH));
			outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_HEIGHT), sizeof(IVGLOBAL::WINDOW_HEIGHT));
		}
		//write window state
		outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::WINDOW_MAXIMIZED), sizeof(IVGLOBAL::WINDOW_MAXIMIZED));
		//write current theme state
		outputStream.write(reinterpret_cast<char *>(&IVGLOBAL::DISPLAY_MODE_DARK), sizeof(IVGLOBAL::DISPLAY_MODE_DARK));


		outputStream.close();
	}
	else {
		std::cerr << IVCONST::LOG_ERROR << "Could not write to settings file!" << std::endl;
	}
}

/**
* redrawImage	- Re-render the display image, respecting zoom and pan positioning
* win 			> Target Window object
* imageTexture 	> Image as SDL_Texture
*/
void redrawImage(Window* win, SDL_Texture* imageTexture) {

	//image is too big for the window
	if (IVGLOBAL::IMAGE_HEIGHT > IVGLOBAL::WINDOW_HEIGHT || IVGLOBAL::IMAGE_WIDTH > IVGLOBAL::WINDOW_WIDTH) {
		//determine the shapes of window and image
		float imageAspectRatio = IVGLOBAL::IMAGE_WIDTH/(float) IVGLOBAL::IMAGE_HEIGHT;
		float windowAspectRatio = IVGLOBAL::WINDOW_WIDTH/(float) IVGLOBAL::WINDOW_HEIGHT;

		//width is priority
		if (imageAspectRatio > windowAspectRatio) {
			//figure out how image will be scaled to fit
			float imageReduction = IVGLOBAL::WINDOW_WIDTH/(float) IVGLOBAL::IMAGE_WIDTH;
			//apply transformation to height
			int imageTargetHeight = imageReduction * IVGLOBAL::IMAGE_HEIGHT;

			//horizontal adjustment
			int xPos = (IVGLOBAL::WINDOW_WIDTH - IVGLOBAL::WINDOW_WIDTH * IVGLOBAL::VIEWPORT_ZOOM)/2 + IVGLOBAL::VIEWPORT_X;
			//vertical adjustment
			int yPos = (IVGLOBAL::WINDOW_HEIGHT - imageTargetHeight * IVGLOBAL::VIEWPORT_ZOOM)/2 + IVGLOBAL::VIEWPORT_Y;

			SDL_Rect windowDestination = {xPos, yPos, (int) (IVGLOBAL::WINDOW_WIDTH * IVGLOBAL::VIEWPORT_ZOOM), (int) (imageTargetHeight * IVGLOBAL::VIEWPORT_ZOOM)};

			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
		//height is priority or equal priority
		else {
			//figure out how image will be scaled to fit
			float imageReduction = IVGLOBAL::WINDOW_HEIGHT/(float) IVGLOBAL::IMAGE_HEIGHT;
			//apply transformation to width
			int imageTargetWidth = imageReduction * IVGLOBAL::IMAGE_WIDTH;

			//horizontal adjustment
			int xPos = (IVGLOBAL::WINDOW_WIDTH - imageTargetWidth * IVGLOBAL::VIEWPORT_ZOOM)/2 + IVGLOBAL::VIEWPORT_X;
			//vertical adjustment
			int yPos = (IVGLOBAL::WINDOW_HEIGHT - IVGLOBAL::WINDOW_HEIGHT * IVGLOBAL::VIEWPORT_ZOOM)/2 + IVGLOBAL::VIEWPORT_Y;

			SDL_Rect windowDestination = {xPos, yPos, (int) (imageTargetWidth * IVGLOBAL::VIEWPORT_ZOOM), (int) (IVGLOBAL::WINDOW_HEIGHT * IVGLOBAL::VIEWPORT_ZOOM)};
			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
	}
	//image will fit in existing window
	else {
		int xPos = (IVGLOBAL::WINDOW_WIDTH - IVGLOBAL::IMAGE_WIDTH * IVGLOBAL::VIEWPORT_ZOOM)/2 + IVGLOBAL::VIEWPORT_X;
		int yPos = (IVGLOBAL::WINDOW_HEIGHT - IVGLOBAL::IMAGE_HEIGHT * IVGLOBAL::VIEWPORT_ZOOM)/2 + IVGLOBAL::VIEWPORT_Y;
		SDL_Rect windowDestination = {xPos, yPos, (int) (IVGLOBAL::IMAGE_WIDTH * IVGLOBAL::VIEWPORT_ZOOM), (int) (IVGLOBAL::IMAGE_HEIGHT * IVGLOBAL::VIEWPORT_ZOOM)};
		SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
	}
}

/**
* drawTileTexture	- Tile texture across window
* win 				> Target Window object
* tileTexture 		> Texture as SDL_Texture to tile
*/
void drawTileTexture(Window* win, SDL_Texture* tileTexture) {
	for (int h = 0; h < IVGLOBAL::WINDOW_HEIGHT; h += IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION){
		for (int w = 0; w < IVGLOBAL::WINDOW_WIDTH; w += IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION){
			SDL_Rect destination = {w, h, IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION, IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION};
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
		std::cerr << IVCONST::LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	IVGLOBAL::IMAGE_HEIGHT = imageSurface->h;
	IVGLOBAL::IMAGE_WIDTH = imageSurface->w;

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
	IVGLOBAL::VIEWPORT_ZOOM = 1.0;
	IVGLOBAL::VIEWPORT_X = 0;
	IVGLOBAL::VIEWPORT_Y = 0;
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
	for (uint8_t i = 0; i < std::size(IVCONST::SUPPORTED_FILETYPES); i++) {
		if (!extension.compare(IVCONST::SUPPORTED_FILETYPES[i])) return (int) i;
	}
	return -1;
}

/**
* main	- Setup, load image, then manage user input
*/
int main(int argc, char* argv[]) {
	bool quit = false;

	/* process any flags */
	for (int i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'v': //-v will print version info
					std::cout << "=== " << IVCONST::APPLICATION_TITLE << " ===" << std::endl;
					std::cout << "Built " << __DATE__ << " " << __TIME__ << std::endl;
					std::cout << "GCC " << __VERSION__ << std::endl;
					std::cout << "STD " << __cplusplus << std::endl;

					SDL_version compile_version;
					SDL_GetVersion(&compile_version);
					std::cout << "SDL " << (int) compile_version.major << "." << (int) compile_version.minor << "." << (int) compile_version.patch << std::endl;
					SDL_IMAGE_VERSION(&compile_version);
					std::cout << "SDL Image " << (int) compile_version.major << "." << (int) compile_version.minor << "." << (int) compile_version.patch << std::endl;
					return 0;
				default: ///no other flags defined yet
					std::cout << "Invalid flag: " << argv[i] << std::endl;
					return 0;
			}
		}
	}

	SDL_Event sdlEvent;
	SDL_Texture* imageTexture = nullptr;

	/* Confirm video is available and set up */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << IVCONST::LOG_ERROR << "SDL COULD NOT BE INITIALIZED!" << std::endl;
		return 1;
	}

	char EXE_PATH[MAX_PATH];
	GetModuleFileNameA(NULL, EXE_PATH, MAX_PATH); //Windows system call to get executable's path
	IVGLOBAL::PATH_PROGRAM_CWD = std::filesystem::path(EXE_PATH).parent_path(); //Collect parent folder path for CWD

	readSettings();

	/* Create invisible application window */
	Window win(IVGLOBAL::WINDOW_WIDTH, IVGLOBAL::WINDOW_HEIGHT, IVGLOBAL::WINDOW_X, IVGLOBAL::WINDOW_Y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (IVGLOBAL::WINDOW_MAXIMIZED ? SDL_WINDOW_MAXIMIZED : 0));
	win.setTitle(IVCONST::APPLICATION_TITLE.c_str());

	//no file passed in
	if (argc < 2) {
		std::cerr << IVCONST::LOG_ERROR << "No arguments provided!" << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
								"No filename provided!",
								"Please provide a path to an image file!",
								win.window);
		return 1;
	}

	//try to improve zoom quality by improving sampling technique
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cout << IVCONST::LOG_NOTICE << "Set filtering to linear." << std::endl;
	}
	//note failure and continue with default sampling
	else {
		std::cout << IVCONST::LOG_NOTICE << "Sampling defaulting to nearest neighbour." << std::endl;
	}

	//create checkerboard background textures for transparent images
	SDL_Surface* transparency_tmp_d = SDL_CreateRGBSurface(0, IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION, IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION, 32, 0, 0, 0, 0);
	SDL_Surface* transparency_tmp_l = SDL_CreateRGBSurface(0, IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION, IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION, 32, 0, 0, 0, 0);

	uint32_t* pixeldata_d = (uint32_t*) transparency_tmp_d->pixels;
	uint32_t* pixeldata_l = (uint32_t*) transparency_tmp_l->pixels;

	for (int h = 0; h < IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION; h++) {
		for (int w = 0; w < IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION; w++) {
			int p = IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION / 2;
			int offset = h * IVCONST::TEXTURE_CHECKERBOARD_RESOLUTION + w;
			if ((h < p) ^ (w < p)) {
				*(pixeldata_d + offset) = IVCONST::CHECKERBOARD_DARK_LIGHT;
				*(pixeldata_l + offset) = IVCONST::CHECKERBOARD_LIGHT_LIGHT;
			}
			else {
				*(pixeldata_d + offset) = IVCONST::CHECKERBOARD_DARK_DARK;
				*(pixeldata_l + offset) = IVCONST::CHECKERBOARD_LIGHT_DARK;
			}
		}
	}

	IVGLOBAL::TEXTURE_TRANSPARENCY_DARK = SDL_CreateTextureFromSurface(win.renderer, transparency_tmp_d);
	IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT = SDL_CreateTextureFromSurface(win.renderer, transparency_tmp_l);
	SDL_FreeSurface(transparency_tmp_d);
	SDL_FreeSurface(transparency_tmp_l);

	//draw background texture before continuing to show program is loading
	draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);

	//load up the image passed in
	if (loadTextureFromFile(std::string(argv[1]), &win, &imageTexture)) {
		std::cerr << IVCONST::LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	try {
		//determine image filename
		IVGLOBAL::PATH_FILE_IMAGE = std::filesystem::canonical(std::filesystem::path(argv[1]));
	} catch (const std::filesystem::filesystem_error& e) {
		//this happens when there are UTF-8 characters in the path. std::fs bug?
		std::cerr << IVCONST::LOG_ERROR << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
								"Could not resolve canonical file path!",
								"This can be the result of the path containing UTF-8 characters.\n"
								"Check for invalid or suspect characters in folders or filename.",
								win.window);
		return 1;
	}
	//update window title with image filename
	win.setTitle((IVGLOBAL::PATH_FILE_IMAGE.filename().string() + " - " + IVCONST::APPLICATION_TITLE).c_str());

	//finally draw image and background
	draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);

	//iterate image folder and mark position of currently open image
	int pos = 0;
	for(auto& entry : std::filesystem::directory_iterator(IVGLOBAL::PATH_FILE_IMAGE.parent_path())) {
		//test that file is real file not link, folder, etc. and check it is a supported image format
		if (std::filesystem::is_regular_file(entry) && (formatSupport(entry.path().extension().string()) >= 0)) {
			//add file to list of images
			IVGLOBAL::FILES_ADJACENT_IMAGES.push_back(entry.path());
			//if this is the image currently open, record position
			if (entry.path() == IVGLOBAL::PATH_FILE_IMAGE) IVGLOBAL::IMAGE_FILE_INDEX = pos;
			pos++;
		}
	}

	std::cout << IVCONST::LOG_NOTICE << "Found " << IVGLOBAL::FILES_ADJACENT_IMAGES.size() << " images adjacent." << std::endl;

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
							IVGLOBAL::WINDOW_MAXIMIZED = true;
							IVGLOBAL::WINDOW_PREVIOUS_X = IVGLOBAL::WINDOW_X;
							IVGLOBAL::WINDOW_PREVIOUS_Y = IVGLOBAL::WINDOW_Y;
							SDL_GetWindowPosition(win.window, &IVGLOBAL::WINDOW_X, &IVGLOBAL::WINDOW_Y);
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							IVGLOBAL::WINDOW_MAXIMIZED = false;
							IVGLOBAL::WINDOW_MAXIMIZED = true;
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							IVGLOBAL::WINDOW_MAXIMIZED = false;
							IVGLOBAL::WINDOW_PREVIOUS_WIDTH = IVGLOBAL::WINDOW_WIDTH;
							IVGLOBAL::WINDOW_PREVIOUS_HEIGHT = IVGLOBAL::WINDOW_HEIGHT;
							SDL_GetWindowSize(win.window, &IVGLOBAL::WINDOW_WIDTH, &IVGLOBAL::WINDOW_HEIGHT);
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
					}
					break;
				case SDL_KEYDOWN:
					switch (sdlEvent.key.keysym.sym) {
						case SDLK_KP_PLUS: //keypad +, zoom in
							IVGLOBAL::VIEWPORT_ZOOM = std::min(IVCONST::ZOOM_MAX, IVGLOBAL::VIEWPORT_ZOOM * 2.0f);
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_KP_MINUS: //keypad -, zoom out
							IVGLOBAL::VIEWPORT_ZOOM = std::max(IVCONST::ZOOM_MIN, IVGLOBAL::VIEWPORT_ZOOM/2.0f);
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_KP_0: //keypad 0, reset zoom and positioning
							resetViewport();
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_ESCAPE:
							quit = true;
							break;
						case SDLK_TAB:
							IVGLOBAL::DISPLAY_MODE_DARK = !IVGLOBAL::DISPLAY_MODE_DARK;
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_DELETE:
							if (IDYES == MessageBox(nullptr, "Are you sure you want to permanently delete this image?\nThis action cannot be reversed!", "Delete Image", MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)) {
								try { //success
									std::filesystem::remove(IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX]);
									std::cout << IVCONST::LOG_NOTICE << "File deleted: " << IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX].string() << std::endl;
								}
								catch (const std::filesystem::filesystem_error& e) { //failure
									std::cerr << IVCONST::LOG_WARNING << "File could not be deleted: " << IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX].string() << std::endl;
									std::cerr << IVCONST::LOG_WARNING << e.what() << std::endl;
									MessageBox(nullptr, "Image could not be deleted.", "Image Deletion Failure", MB_OK | MB_ICONERROR);
									break;
								}
								IVGLOBAL::FILES_ADJACENT_IMAGES.erase(IVGLOBAL::FILES_ADJACENT_IMAGES.begin() + IVGLOBAL::IMAGE_FILE_INDEX);
								IVGLOBAL::IMAGE_FILE_INDEX--;
								SDL_Event sdlENext;
								sdlENext.type = SDL_KEYDOWN;
								sdlENext.key.keysym.sym = SDLK_RIGHT;
								SDL_PushEvent(&sdlENext);
							}
							break;
						//previous image
						case SDLK_LEFT: //move back
							if (IVGLOBAL::FILES_ADJACENT_IMAGES.size() == 1) break; //there's only one image in the folder so don't move
							if (IVGLOBAL::IMAGE_FILE_INDEX == 0) IVGLOBAL::IMAGE_FILE_INDEX = IVGLOBAL::FILES_ADJACENT_IMAGES.size() - 1; //loop back to end of image file list
							else IVGLOBAL::IMAGE_FILE_INDEX--;
							win.setTitle((IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX].filename().string() + " - " + IVCONST::APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(std::filesystem::canonical(IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX]).string(), &win, &imageTexture)) { //if call returned non-zero, there was an error
								std::cerr << IVCONST::LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						//next image
						case SDLK_RIGHT: //move forward
							if (IVGLOBAL::FILES_ADJACENT_IMAGES.size() == 1) break; //there's only one image in the folder so don't move
							IVGLOBAL::IMAGE_FILE_INDEX++;
							if (IVGLOBAL::IMAGE_FILE_INDEX >= IVGLOBAL::FILES_ADJACENT_IMAGES.size()) IVGLOBAL::IMAGE_FILE_INDEX = 0; //loop back to start of image file list
							win.setTitle((IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX].filename().string() + " - " + IVCONST::APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(std::filesystem::canonical(IVGLOBAL::FILES_ADJACENT_IMAGES[IVGLOBAL::IMAGE_FILE_INDEX]).string(), &win, &imageTexture)) { //if call returned non-zero, there was an error
								std::cerr << IVCONST::LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
					if (sdlEvent.wheel.y > 0) {
						IVGLOBAL::VIEWPORT_ZOOM = std::min(IVCONST::ZOOM_MAX, IVGLOBAL::VIEWPORT_ZOOM * IVCONST::ZOOM_SCROLL_SENSITIVITY);

						/* Correct positioning.
						 * Required as otherwise zoom functions as [center zoom + offset]
						 *  instead of [zoom on offset from center] */
						IVGLOBAL::VIEWPORT_X *= IVCONST::ZOOM_SCROLL_SENSITIVITY;
						IVGLOBAL::VIEWPORT_Y *= IVCONST::ZOOM_SCROLL_SENSITIVITY;

						draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
					}
					else if (sdlEvent.wheel.y < 0) {
						IVGLOBAL::VIEWPORT_ZOOM = std::max(IVCONST::ZOOM_MIN, IVGLOBAL::VIEWPORT_ZOOM / IVCONST::ZOOM_SCROLL_SENSITIVITY);

						//correct positioning
						IVGLOBAL::VIEWPORT_X /= IVCONST::ZOOM_SCROLL_SENSITIVITY;
						IVGLOBAL::VIEWPORT_Y /= IVCONST::ZOOM_SCROLL_SENSITIVITY;

						draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (sdlEvent.button.button) {
						case SDL_BUTTON_LEFT:
							IVGLOBAL::MOUSE_CLICK_STATE_LEFT = true;
							SDL_GetMouseState(&mouseX, &mouseY);
							break;
						case SDL_BUTTON_MIDDLE:
							resetViewport();
							draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
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
							IVGLOBAL::MOUSE_CLICK_STATE_LEFT = false;
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					if (IVGLOBAL::MOUSE_CLICK_STATE_LEFT) {
						mousePreviousX = mouseX;
						mousePreviousY = mouseY;
						SDL_GetMouseState(&mouseX, &mouseY);
						IVGLOBAL::VIEWPORT_X += mouseX - mousePreviousX;
						IVGLOBAL::VIEWPORT_Y += mouseY - mousePreviousY;
						draw(&win, (IVGLOBAL::DISPLAY_MODE_DARK) ? IVGLOBAL::TEXTURE_TRANSPARENCY_DARK : IVGLOBAL::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
					}
					break;

			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	writeSettings();

	return 0;
}

/* TODO: IfDef Debug: -v goes to console, otherwise show messagebox with version info */
