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

namespace IVC {

	/* ABOUT */
	SDL_version SDL_COMPILED_VERSION;
	SDL_version SDL_IMAGE_COMPILED_VERSION;
	SDL_version GCC_COMPILER_VERSION = {__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__};
	const long CPP_STANDARD = __cplusplus;
	const std::string BUILD_DATE = __DATE__;
	const std::string BUILD_TIME = __TIME__;
	std::string VERSION_ABOUT;

	std::string IVVERSION = "0.2.3";

	/* LOGGING */
	const std::string APPLICATION_TITLE = "Image Viewer";
	const std::string LOG_ERROR = "<ERROR> ";
	const std::string LOG_WARNING = "<WARNING> ";
	const std::string LOG_NOTICE = "<NOTICE> ";

	/* WINDOW MANAGEMENT */
	const int WIN_DEFAULT_W = 1600;
	const int WIN_DEFAULT_H = 900;

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

	/* From sdl_image documentation */
	/* Less common formats omitted */
	enum FILE_TYPE {
		JPG,
		PNG,
		GIF,
		BMP,
		TIF,
		TGA
	};
}

/* /// GLOBALS /// */

namespace IVG {

	/* STATE */
	bool WIN_MOVED = false;

	/* DISPLAY */
	int IMAGE_W = 0;
	int IMAGE_H = 0;

	int VIEWPORT_X = 0;
	int VIEWPORT_Y = 0;
	float VIEWPORT_ZOOM = 1.0f;

	SDL_Texture* TEXTURE_TRANSPARENCY_DARK;
	SDL_Texture* TEXTURE_TRANSPARENCY_LIGHT;

	/* INPUT */
	bool MOUSE_CLICK_STATE_LEFT = false;

	/* SETTINGS */
	std::filesystem::path PATH_PROGRAM_CWD;
	std::filesystem::path PATH_FILE_IMAGE;

	std::vector<std::filesystem::path> FILES_IMAGES_ADJACENT;
	uint32_t IMAGE_FILE_INDEX = 0;

	// This the data read from and written to settings file.
	// Adding a new value here will cause old config files to be discarded on first read.
	struct IVSETTINGS {
		int WIN_X;
		int WIN_Y;
		int WIN_W;
		int WIN_H;
		bool MAXIMIZED;
		bool DISPLAY_MODE_DARK;
	} SETTINGS;

	// These are the default values for the settings.
	// If adding a new setting, be sure to add a sensible default here too.
	struct IVSETTINGS DEFAULTS = {
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		IVC::WIN_DEFAULT_W,
		IVC::WIN_DEFAULT_H,
		false,
		true
	};
}

/* /// CODE /// */

/**
* readSettings - Load settings file if possible and recover window position and size.
*/
void readSettings() {
	//if CWD is length 0, program was run from terminal and is CWD is unknown. Use default settings.
	if (IVG::PATH_PROGRAM_CWD.empty()) {
		std::cerr << IVC::LOG_NOTICE << "Unable to load settings file. Using defaults." << std::endl;
		IVG::SETTINGS = IVG::DEFAULTS; //copy defaults
		return;
	}
	//open filestream to read file at the end
	std::ifstream inputStream(IVG::PATH_PROGRAM_CWD / IVC::FILENAME_SETTINGS, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	if (inputStream.is_open()) {
		//current position is end, this will present filesize
		int fileLength = inputStream.tellg();
		if (fileLength == -1) { //read failed somewhere in iostream
			std::cerr << IVC::LOG_NOTICE << "Failed to read settings file!" << std::endl;
			IVG::SETTINGS = IVG::DEFAULTS; //copy defaults
		}
		else if ((unsigned) fileLength < sizeof(IVG::SETTINGS)) { //settings file is too short
			std::cerr << IVC::LOG_WARNING << "Incorrect settings data size! [Expected " << sizeof(IVG::SETTINGS) << ", got " << fileLength << "]" << std::endl;
			IVG::SETTINGS = IVG::DEFAULTS; //copy defaults
		}
		else { //no problems encountered
			inputStream.seekg(std::ios_base::beg);
			inputStream.read(reinterpret_cast<char *>(&IVG::SETTINGS), sizeof(IVG::SETTINGS));
		}
		inputStream.close();
	}
	else { //load defaults
		std::cerr << IVC::LOG_NOTICE << "No settings file located!" << std::endl;
		IVG::SETTINGS = IVG::DEFAULTS; //copy defaults
	}
}

/**
* writeSettings - Collect window settings and write to settings file, if it can be written in a valid location
*/
void writeSettings(Window* win) {
	//working directory wasn't set, don't write anything
	if (IVG::PATH_PROGRAM_CWD.empty()) return;
	std::ofstream outputStream(IVG::PATH_PROGRAM_CWD / IVC::FILENAME_SETTINGS, std::ifstream::out | std::ifstream::binary | std::ifstream::trunc);
	if (outputStream.is_open()) {
		if (IVG::WIN_MOVED) { // window really was moved last
			IVG::SETTINGS.WIN_X = win->x;
			IVG::SETTINGS.WIN_Y = win->y;
		}
		else { // if window was 'moved' during resize, discard
			IVG::SETTINGS.WIN_X = win->px;
			IVG::SETTINGS.WIN_Y = win->py;
		}
		if (IVG::SETTINGS.MAXIMIZED) { // if the window is maximized, write the last known size before it was maximized
			IVG::SETTINGS.WIN_W = win->pw;
			IVG::SETTINGS.WIN_H = win->ph;
		}
		else { // write the current size
			IVG::SETTINGS.WIN_W = win->w;
			IVG::SETTINGS.WIN_H = win->h;
		}

		outputStream.write(reinterpret_cast<char *>(&IVG::SETTINGS), sizeof(IVG::SETTINGS));
		outputStream.close();
	}
	else {
		std::cerr << IVC::LOG_ERROR << "Could not write to settings file!" << std::endl;
	}
}

/**
* redrawImage	- Re-render the display image, respecting zoom and pan positioning
* win 			> Target Window object
* imageTexture 	> Image as SDL_Texture
*/
void redrawImage(Window* win, SDL_Texture* imageTexture) {

	//image is too big for the window
	if (IVG::IMAGE_H > win->h || IVG::IMAGE_W > win->w) {
		//determine the shapes of window and image
		float imageAspectRatio = IVG::IMAGE_W/(float) IVG::IMAGE_H;
		float windowAspectRatio = win->w/(float) win->h;

		//width is priority
		if (imageAspectRatio > windowAspectRatio) {
			//figure out how image will be scaled to fit
			float imageReduction = win->w/(float) IVG::IMAGE_W;
			//apply transformation to height
			int imageTargetHeight = imageReduction * IVG::IMAGE_H;

			//horizontal adjustment
			int xPos = (win->w - win->w * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_X;
			//vertical adjustment
			int yPos = (win->h - imageTargetHeight * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_Y;

			SDL_Rect windowDestination = {xPos, yPos, (int) (win->w * IVG::VIEWPORT_ZOOM), (int) (imageTargetHeight * IVG::VIEWPORT_ZOOM)};
			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
		//height is priority or equal priority
		else {
			//figure out how image will be scaled to fit
			float imageReduction = win->h/(float) IVG::IMAGE_H;
			//apply transformation to width
			int imageTargetWidth = imageReduction * IVG::IMAGE_W;

			//horizontal adjustment
			int xPos = (win->w - imageTargetWidth * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_X;
			//vertical adjustment
			int yPos = (win->h - win->h * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_Y;

			SDL_Rect windowDestination = {xPos, yPos, (int) (imageTargetWidth * IVG::VIEWPORT_ZOOM), (int) (win->h * IVG::VIEWPORT_ZOOM)};
			SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
		}
	}
	//image will fit in existing window
	else {
		int xPos = (win->w - IVG::IMAGE_W * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_X;
		int yPos = (win->h - IVG::IMAGE_H * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_Y;
		SDL_Rect windowDestination = {xPos, yPos, (int) (IVG::IMAGE_W * IVG::VIEWPORT_ZOOM), (int) (IVG::IMAGE_H * IVG::VIEWPORT_ZOOM)};
		SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
	}
}

/**
* drawTileTexture	- Tile texture across window
* win 				> Target Window object
* tileTexture 		> Texture as SDL_Texture to tile
*/
void drawTileTexture(Window* win, SDL_Texture* tileTexture) {
	for (int h = 0; h < win->h; h += IVC::TEXTURE_CHECKERBOARD_RESOLUTION){
		for (int w = 0; w < win->w; w += IVC::TEXTURE_CHECKERBOARD_RESOLUTION){
			SDL_Rect destination = {w, h, IVC::TEXTURE_CHECKERBOARD_RESOLUTION, IVC::TEXTURE_CHECKERBOARD_RESOLUTION};
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
		std::cerr << IVC::LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	IVG::IMAGE_H = imageSurface->h;
	IVG::IMAGE_W = imageSurface->w;

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
	IVG::VIEWPORT_ZOOM = 1.0;
	IVG::VIEWPORT_X = 0;
	IVG::VIEWPORT_Y = 0;
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
* return - int 	< IVC::FILE_TYPE of file type or -1 if no match
*/
int formatSupport(std::string extension) {
	// convert character to upper case for comparison
	auto upper = [](char a) -> char {
		if (a > 0x60 && a < 0x7B) return (char) (a - 0x20);
		else return a;
	};

	// convert extension string, ignoring leading dot
	std::string extensionUppercase = "";
	for (unsigned i = 1; i < extension.length(); i++) {
		extensionUppercase += upper(extension[i]);
	}
	
	// sort by first character of extension
	switch (extensionUppercase[0]) {
		case 'J':
			/* JP(E)G */
			if (!extensionUppercase.compare("JPG"))  return IVC::JPG;
			if (!extensionUppercase.compare("JPEG")) return IVC::JPG;
			break;
		case 'P':
			/* PNG */
			if (!extensionUppercase.compare("PNG"))  return IVC::PNG;
			break;
		case 'G':
			/* GIF */
			if (!extensionUppercase.compare("GIF"))  return IVC::GIF;
			break;
		case 'B':
			/* BMP */
			if (!extensionUppercase.compare("BMP"))  return IVC::BMP;
			break;
		case 'T':
			/* TIF(F) */
			if (!extensionUppercase.compare("TIF"))  return IVC::TIF;
			if (!extensionUppercase.compare("TIFF")) return IVC::TIF;
			/* TGA */
			if (!extensionUppercase.compare("TGA"))  return IVC::TGA;
			break;
		default:
			return -1;
	}

	return -1;
}

/* Format and return version string */
std::string versionToString(SDL_version* version) {
	return std::to_string(version->major) + '.' + std::to_string(version->minor) + '.' + std::to_string(version->patch);
}

/**
* main	- Setup, load image, then manage user input
*/
int main(int argc, char* argv[]) {
	bool quit = false;

	/* Populate version info */
	SDL_VERSION(&IVC::SDL_COMPILED_VERSION);
	SDL_IMAGE_VERSION(&IVC::SDL_IMAGE_COMPILED_VERSION);

	IVC::VERSION_ABOUT = IVC::APPLICATION_TITLE + " " + IVC::IVVERSION
							+ "\nBUILT " + IVC::BUILD_DATE + " " + IVC::BUILD_TIME 
							+ "\nGCC " + versionToString(&IVC::GCC_COMPILER_VERSION)
							+ "\nSTD " + std::to_string(IVC::CPP_STANDARD)
							+ "\nSDL VERSION " + versionToString(&IVC::SDL_COMPILED_VERSION)
							+ "\nSDL IMAGE VERSION " + versionToString(&IVC::SDL_IMAGE_COMPILED_VERSION);

	/* Process any flags */
	for (int i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'v': //-v will print version info
					std::cout << "=== ABOUT: " << IVC::APPLICATION_TITLE << " ===" << std::endl;
					std::cout << IVC::VERSION_ABOUT << std::endl;
					return 0;
				default: ///no other flags defined yet
					std::cout << "Invalid flag: " << argv[i] << std::endl;
					return 0;
			}
		}
	}

	/* Confirm video is available and set up */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << IVC::LOG_ERROR << "SDL COULD NOT BE INITIALIZED!" << std::endl;
		return 1;
	}

	SDL_Event sdlEvent;
	SDL_Texture* imageTexture = nullptr;

	char EXE_PATH[MAX_PATH];
	GetModuleFileNameA(NULL, EXE_PATH, MAX_PATH); //Windows system call to get executable's path
	IVG::PATH_PROGRAM_CWD = std::filesystem::path(EXE_PATH).parent_path(); //Collect parent folder path for CWD

	readSettings();

	/* Create invisible application window */
	Window win(IVG::SETTINGS.WIN_W, IVG::SETTINGS.WIN_H, IVG::SETTINGS.WIN_X, IVG::SETTINGS.WIN_Y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (IVG::SETTINGS.MAXIMIZED ? SDL_WINDOW_MAXIMIZED : 0));
	win.setTitle(IVC::APPLICATION_TITLE.c_str());

	//no file passed in
	if (argc < 2) {
		std::cerr << IVC::LOG_ERROR << "No arguments provided!" << std::endl;
		MessageBox(nullptr, "Please provide a path to an image file!", "No filename provided!", MB_OK | MB_ICONERROR);
		return 1;
	}

	//try to improve zoom quality by improving sampling technique
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cout << IVC::LOG_NOTICE << "Set filtering to linear." << std::endl;
	}
	//note failure and continue with default sampling
	else {
		std::cout << IVC::LOG_NOTICE << "Sampling defaulting to nearest neighbour." << std::endl;
	}

	//create checkerboard background textures for transparent images
	SDL_Surface* transparency_tmp_d = SDL_CreateRGBSurface(0, IVC::TEXTURE_CHECKERBOARD_RESOLUTION, IVC::TEXTURE_CHECKERBOARD_RESOLUTION, 32, 0, 0, 0, 0);
	SDL_Surface* transparency_tmp_l = SDL_CreateRGBSurface(0, IVC::TEXTURE_CHECKERBOARD_RESOLUTION, IVC::TEXTURE_CHECKERBOARD_RESOLUTION, 32, 0, 0, 0, 0);

	uint32_t* pixeldata_d = (uint32_t*) transparency_tmp_d->pixels;
	uint32_t* pixeldata_l = (uint32_t*) transparency_tmp_l->pixels;

	for (int h = 0; h < IVC::TEXTURE_CHECKERBOARD_RESOLUTION; h++) {
		for (int w = 0; w < IVC::TEXTURE_CHECKERBOARD_RESOLUTION; w++) {
			int p = IVC::TEXTURE_CHECKERBOARD_RESOLUTION / 2;
			int offset = h * IVC::TEXTURE_CHECKERBOARD_RESOLUTION + w;
			if ((h < p) ^ (w < p)) {
				*(pixeldata_d + offset) = IVC::CHECKERBOARD_DARK_LIGHT;
				*(pixeldata_l + offset) = IVC::CHECKERBOARD_LIGHT_LIGHT;
			}
			else {
				*(pixeldata_d + offset) = IVC::CHECKERBOARD_DARK_DARK;
				*(pixeldata_l + offset) = IVC::CHECKERBOARD_LIGHT_DARK;
			}
		}
	}

	IVG::TEXTURE_TRANSPARENCY_DARK = SDL_CreateTextureFromSurface(win.renderer, transparency_tmp_d);
	IVG::TEXTURE_TRANSPARENCY_LIGHT = SDL_CreateTextureFromSurface(win.renderer, transparency_tmp_l);
	SDL_FreeSurface(transparency_tmp_d);
	SDL_FreeSurface(transparency_tmp_l);

	//draw background texture before continuing to show program is loading
	draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);

	//load up the image passed in
	if (loadTextureFromFile(std::string(argv[1]), &win, &imageTexture)) {
		std::cerr << IVC::LOG_ERROR << IMG_GetError() << std::endl;
		return 1;
	}

	try {
		//determine image filename
		IVG::PATH_FILE_IMAGE = std::filesystem::canonical(std::filesystem::path(argv[1]));
	} catch (const std::filesystem::filesystem_error& e) {
		//this happens when there are UTF-8 characters in the path. std::fs bug?
		std::cerr << IVC::LOG_ERROR << e.what() << std::endl;
		MessageBox(nullptr, "This can be the result of the path containing UTF-8 characters.\n"
							"Check for invalid or suspect characters in folders or filename.", 
							"Could not resolve canonical file path!", MB_OK | MB_ICONERROR);
		return 1;
	}
	//update window title with image filename
	win.setTitle((IVG::PATH_FILE_IMAGE.filename().string() + " - " + IVC::APPLICATION_TITLE).c_str());

	//finally draw image and background
	draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);

	//iterate image folder and mark position of currently open image
	int pos = 0;
	for(auto& entry : std::filesystem::directory_iterator(IVG::PATH_FILE_IMAGE.parent_path())) {
		//test that file is real file not link, folder, etc. and check it is a supported image format
		if (std::filesystem::is_regular_file(entry) && (formatSupport(entry.path().extension().string()) >= 0)) {
			//add file to list of images
			IVG::FILES_IMAGES_ADJACENT.push_back(entry.path());
			//if this is the image currently open, record position
			if (entry.path() == IVG::PATH_FILE_IMAGE) IVG::IMAGE_FILE_INDEX = pos;
			pos++;
		}
	}

	std::cout << IVC::LOG_NOTICE << "Found " << IVG::FILES_IMAGES_ADJACENT.size() << " images adjacent." << std::endl;

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
							IVG::WIN_MOVED = true;
							win.updateWindowPos();
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							IVG::WIN_MOVED = false;
							IVG::SETTINGS.MAXIMIZED = true;
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							IVG::SETTINGS.MAXIMIZED = false;
							win.updateWindowSize();
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
					}
					break;
				case SDL_KEYDOWN:
					switch (sdlEvent.key.keysym.sym) {
						case SDLK_EQUALS:  //equals with plus secondary
						case SDLK_KP_PLUS: //or keypad plus, zoom in
							IVG::VIEWPORT_ZOOM = std::min(IVC::ZOOM_MAX, IVG::VIEWPORT_ZOOM * 2.0f);
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_MINUS:    //standard minus
						case SDLK_KP_MINUS: //or keypad minus, zoom out
							IVG::VIEWPORT_ZOOM = std::max(IVC::ZOOM_MIN, IVG::VIEWPORT_ZOOM / 2.0f);
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_0:    //standard 0
						case SDLK_KP_0: //or keypad 0, reset zoom and positioning
							resetViewport();
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_ESCAPE: //quit
							quit = true;
							break;
						case SDLK_F1: //help
							MessageBox(nullptr, IVC::VERSION_ABOUT.c_str(), ("About " + IVC::APPLICATION_TITLE).c_str(), MB_OK | MB_ICONINFORMATION);
							break;
						case SDLK_TAB: //toggle light mode
							IVG::SETTINGS.DISPLAY_MODE_DARK = !IVG::SETTINGS.DISPLAY_MODE_DARK;
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						case SDLK_DELETE: //delete image
							if (IDYES == MessageBox(nullptr, "Are you sure you want to permanently delete this image?\nThis action cannot be reversed!", "Delete Image", MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)) {
								try { //success
									std::filesystem::remove(IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX]);
									std::cout << IVC::LOG_NOTICE << "File deleted: " << IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX].string() << std::endl;
								}
								catch (const std::filesystem::filesystem_error& e) { //failure
									std::cerr << IVC::LOG_WARNING << "File could not be deleted: " << IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX].string() << std::endl;
									std::cerr << IVC::LOG_WARNING << e.what() << std::endl;
									MessageBox(nullptr, "Image could not be deleted.", "Image Deletion Failure", MB_OK | MB_ICONERROR);
									break;
								}
								IVG::FILES_IMAGES_ADJACENT.erase(IVG::FILES_IMAGES_ADJACENT.begin() + IVG::IMAGE_FILE_INDEX);
								IVG::IMAGE_FILE_INDEX--;
								SDL_Event sdlENext;
								sdlENext.type = SDL_KEYDOWN;
								sdlENext.key.keysym.sym = SDLK_RIGHT;
								SDL_PushEvent(&sdlENext);
							}
							break;
						//previous image
						case SDLK_LEFT: //move back
							if (IVG::FILES_IMAGES_ADJACENT.size() == 1) break; //there's only one image in the folder so don't move
							if (IVG::IMAGE_FILE_INDEX == 0) IVG::IMAGE_FILE_INDEX = IVG::FILES_IMAGES_ADJACENT.size() - 1; //loop back to end of image file list
							else IVG::IMAGE_FILE_INDEX--;
							win.setTitle((IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX].filename().string() + " - " + IVC::APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX]).string(), &win, &imageTexture)) { //if call returned non-zero, there was an error
								std::cerr << IVC::LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
						//next image
						case SDLK_RIGHT: //move forward
							if (IVG::FILES_IMAGES_ADJACENT.size() == 1) break; //there's only one image in the folder so don't move
							IVG::IMAGE_FILE_INDEX++;
							if (IVG::IMAGE_FILE_INDEX >= IVG::FILES_IMAGES_ADJACENT.size()) IVG::IMAGE_FILE_INDEX = 0; //loop back to start of image file list
							win.setTitle((IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX].filename().string() + " - " + IVC::APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::IMAGE_FILE_INDEX]).string(), &win, &imageTexture)) { //if call returned non-zero, there was an error
								std::cerr << IVC::LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
					if (sdlEvent.wheel.y > 0) {
						IVG::VIEWPORT_ZOOM = std::min(IVC::ZOOM_MAX, IVG::VIEWPORT_ZOOM * IVC::ZOOM_SCROLL_SENSITIVITY);

						/* Correct positioning.
						 * Required as otherwise zoom functions as [center zoom + offset]
						 *  instead of [zoom on offset from center] */
						IVG::VIEWPORT_X *= IVC::ZOOM_SCROLL_SENSITIVITY;
						IVG::VIEWPORT_Y *= IVC::ZOOM_SCROLL_SENSITIVITY;

						draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
					}
					else if (sdlEvent.wheel.y < 0) {
						IVG::VIEWPORT_ZOOM = std::max(IVC::ZOOM_MIN, IVG::VIEWPORT_ZOOM / IVC::ZOOM_SCROLL_SENSITIVITY);

						//correct positioning
						IVG::VIEWPORT_X /= IVC::ZOOM_SCROLL_SENSITIVITY;
						IVG::VIEWPORT_Y /= IVC::ZOOM_SCROLL_SENSITIVITY;

						draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (sdlEvent.button.button) {
						case SDL_BUTTON_LEFT:
							IVG::MOUSE_CLICK_STATE_LEFT = true;
							SDL_GetMouseState(&mouseX, &mouseY);
							break;
						case SDL_BUTTON_MIDDLE:
							resetViewport();
							draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
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
							IVG::MOUSE_CLICK_STATE_LEFT = false;
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					if (IVG::MOUSE_CLICK_STATE_LEFT) {
						mousePreviousX = mouseX;
						mousePreviousY = mouseY;
						SDL_GetMouseState(&mouseX, &mouseY);
						IVG::VIEWPORT_X += mouseX - mousePreviousX;
						IVG::VIEWPORT_Y += mouseY - mousePreviousY;
						draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? IVG::TEXTURE_TRANSPARENCY_DARK : IVG::TEXTURE_TRANSPARENCY_LIGHT, imageTexture);
					}
					break;

			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	writeSettings(&win);

	return 0;
}
