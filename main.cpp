/*
VIEWER - MAIN.CPP
NICK WILSON
2020
*/

/* /// IMPORTS /// */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "IVUtil.hpp"
#include "subclasses/Window.hpp"
#include "subclasses/TiledTexture.hpp"

#include "subclasses/IVImage.hpp"
#include "subclasses/IVStaticImage.hpp"
#include "subclasses/IVAnimatedImage.hpp"

#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <vector>
#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Shellapi.h> //reinclude after excluding with WIN32_LEAN_AND_MEAN

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
	const std::string IVVERSION = "0.3.1";

	/* WINDOW MANAGEMENT */
	const int WIN_DEFAULT_W = 1600;
	const int WIN_DEFAULT_H = 900;

	/* DISPLAY SETTINGS */
	const float ZOOM_MIN = (1.0/16.0);  //-4x zoom
	const float ZOOM_MAX = 256;         // 8x zoom
	const float ZOOM_SCROLL_SENSITIVITY = 1.2;

	const int RES_CHECKERBOARD = 32;
	const uint32_t COLOUR_D_L = 0x00141414;
	const uint32_t COLOUR_D_D = 0x000D0D0D;
	const uint32_t COLOUR_L_L = 0x00F3F3F3;
	const uint32_t COLOUR_L_D = 0x00DEDEDE;

	// These are the default values for the settings.
	// If adding a new setting, be sure to add a sensible default here too.
	const struct IVUTIL::IVSETTINGS DEFAULTS = {
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		IVC::WIN_DEFAULT_W,
		IVC::WIN_DEFAULT_H,
		false,
		true
	};

	const std::string FILENAME_SETTINGS = "settings.cfg";
}

/* /// GLOBALS /// */

namespace IVG {

	/* STATE */
	bool WIN_MOVED = false;

	/* DISPLAY */
	int VIEWPORT_X = 0;
	int VIEWPORT_Y = 0;
	float VIEWPORT_ZOOM = 1.0f;

	// a sensible default
	int REFRESH_RATE = 60;

	/* INPUT */
	bool MOUSE_CLICK_STATE_LEFT = false;

	/* SETTINGS */
	std::filesystem::path PATH_PROGRAM_CWD;
	std::filesystem::path PATH_IMAGE_FILE;

	std::vector<std::filesystem::path> FILES_IMAGES_ADJACENT;
	uint32_t INDEX_IMAGE_FILE = 0;

	/* Set settings to default values, to be overwritten if settings file is loaded */
	struct IVUTIL::IVSETTINGS SETTINGS = IVC::DEFAULTS;

	std::unique_ptr<IVImage> IMAGE_CURRENT;
}

/* /// CODE /// */

void pushSettings(Window* win) {
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
}

/**
* redrawImage	- Re-render the display image, respecting zoom and pan positioning
* win 			> Target Window object
* imageTexture 	> Image as SDL_Texture
*/
void redrawImage(Window* win, SDL_Texture* imageTexture) {

	//image is too big for the window
	if (IVG::IMAGE_CURRENT->h > win->h || IVG::IMAGE_CURRENT->w > win->w) {
		//determine the shapes of window and image
		float imageAspectRatio = IVG::IMAGE_CURRENT->w/(float) IVG::IMAGE_CURRENT->h;
		float windowAspectRatio = win->w/(float) win->h;

		//width is priority
		if (imageAspectRatio > windowAspectRatio) {
			//figure out how image will be scaled to fit
			float imageReduction = win->w/(float) IVG::IMAGE_CURRENT->w;
			//apply transformation to height
			int imageTargetHeight = imageReduction * IVG::IMAGE_CURRENT->h;

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
			float imageReduction = win->h/(float) IVG::IMAGE_CURRENT->h;
			//apply transformation to width
			int imageTargetWidth = imageReduction * IVG::IMAGE_CURRENT->w;

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
		int xPos = (win->w - IVG::IMAGE_CURRENT->w * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_X;
		int yPos = (win->h - IVG::IMAGE_CURRENT->h * IVG::VIEWPORT_ZOOM)/2 + IVG::VIEWPORT_Y;
		SDL_Rect windowDestination = {xPos, yPos, (int) (IVG::IMAGE_CURRENT->w * IVG::VIEWPORT_ZOOM), (int) (IVG::IMAGE_CURRENT->h * IVG::VIEWPORT_ZOOM)};
		SDL_RenderCopy(win->renderer, imageTexture, 0, &windowDestination);
	}
}

/**
* drawTileTexture	- Tile texture across window
* win 				> Target Window object
* tileTexture 		> Texture as SDL_Texture to tile
*/
void drawTileTexture(Window* win, TiledTexture* tiledTexture) {
	for (int h = 0; h < win->h; h += tiledTexture->h){
		for (int w = 0; w < win->w; w += tiledTexture->w){
			SDL_Rect destination = {w, h, tiledTexture->w, tiledTexture->h};
			SDL_RenderCopy(win->renderer, tiledTexture->texture, 0, &destination);
		}
	}
}

/**
* loadTextureFromFile	- Load a file and convert it to an SDL_Texture
* renderer 				> Target SDL_Renderer
* filePath 				> The path to the image to load
* return - int 			< 0 on success or 1 on failure
*/
int loadTextureFromFile(SDL_Renderer* renderer, std::filesystem::path filePath) {
	//try loading image from filename
	int filetype = IVUTIL::formatSupport(filePath.extension().string());
	try {
		if (filetype == IVUTIL::GIF) {
			//load animated image
			IVG::IMAGE_CURRENT.reset(new IVAnimatedImage(renderer, filePath));
		}
		else if (filetype > -1) {
			//load static image
			IVG::IMAGE_CURRENT.reset(new IVStaticImage(renderer, filePath));
		}
		else {
			return 1;
		}
	}
	catch (IVUTIL::IVEXCEPT except) {
		switch (except) {
			case IVUTIL::EXCEPT_IMG_OPEN_FAIL:
				std::cerr << IVUTIL::LOG_WARNING << "Failed to open image \'" << filePath << "\'" << std::endl;
				break;
			case IVUTIL::EXCEPT_IMG_LOAD_FAIL:
				std::cerr << IVUTIL::LOG_WARNING << "Failed to load image \'" << filePath << "\'" << std::endl;
				break;
			case IVUTIL::EXCEPT_IMG_GIF_STATIC:
				std::cout << IVUTIL::LOG_NOTICE << "Image better suited as static, switching" << std::endl;
				/* This may also throw so it needs a t/c block */
				try {
					IVG::IMAGE_CURRENT.reset(new IVStaticImage(renderer, filePath));
					return 0;
				} catch (...) {
					return 1;
				}
				
			default:
				std::cerr << IVUTIL::LOG_WARNING << "Unknown error loading image \'" << filePath << "\'" << std::endl;
				break;
		}
		return 1;
	}
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
void draw(Window* win, TiledTexture* BGTiledTexture, SDL_Texture* image_texture) {
	SDL_RenderClear(win->renderer);
	drawTileTexture(win, BGTiledTexture);
	if (image_texture) redrawImage(win, image_texture);
	SDL_RenderPresent(win->renderer);
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

	IVC::VERSION_ABOUT = IVUTIL::APPLICATION_TITLE + " " + IVC::IVVERSION
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
					std::cout << "=== ABOUT: " << IVUTIL::APPLICATION_TITLE << " ===" << std::endl;
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
		std::cerr << IVUTIL::LOG_ERROR << "SDL COULD NOT BE INITIALIZED!" << std::endl;
		return 1;
	}

	SDL_DisplayMode displayMode;
	SDL_Event sdlEvent;

	char EXE_PATH[MAX_PATH];
	GetModuleFileNameA(NULL, EXE_PATH, MAX_PATH); //Windows system call to get executable's path
	IVG::PATH_PROGRAM_CWD = std::filesystem::path(EXE_PATH).parent_path(); //Collect parent folder path for CWD

	IVUTIL::readSettings(IVG::PATH_PROGRAM_CWD / IVC::FILENAME_SETTINGS, &IVG::SETTINGS);

	/* Create invisible application window */
	Window win(IVG::SETTINGS.WIN_W, IVG::SETTINGS.WIN_H, IVG::SETTINGS.WIN_X, IVG::SETTINGS.WIN_Y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (IVG::SETTINGS.MAXIMIZED ? SDL_WINDOW_MAXIMIZED : 0));
	win.setTitle(IVUTIL::APPLICATION_TITLE.c_str());

	if (!SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(win.window), &displayMode)) {
		IVG::REFRESH_RATE = displayMode.refresh_rate;
	}

	// No file passed in
	if (argc < 2) {
		std::cerr << IVUTIL::LOG_ERROR << "No arguments provided!" << std::endl;
		MessageBox(nullptr, "Please provide a path to an image file!", "No filename provided!", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Try to improve zoom quality by improving sampling technique
	if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cout << IVUTIL::LOG_NOTICE << "Set filtering to linear." << std::endl;
	}
	// Note failure and continue with default sampling
	else {
		std::cout << IVUTIL::LOG_NOTICE << "Sampling defaulting to nearest neighbour." << std::endl;
	}

	// Create checkerboard background textures for transparent images
	TiledTexture TEXTURE_DARK(win.renderer, IVC::RES_CHECKERBOARD, IVC::RES_CHECKERBOARD, IVC::COLOUR_D_L, IVC::COLOUR_D_D);
	TiledTexture TEXTURE_LIGHT(win.renderer, IVC::RES_CHECKERBOARD, IVC::RES_CHECKERBOARD, IVC::COLOUR_L_L, IVC::COLOUR_L_D);

	// Draw background texture before continuing to show program is loading
	draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? &TEXTURE_DARK : &TEXTURE_LIGHT, nullptr);

	try {
		// Determine image filename
		IVG::PATH_IMAGE_FILE = std::filesystem::canonical(std::filesystem::path(argv[1]));
	} catch (const std::filesystem::filesystem_error& e) {
		// This happens when there are UTF-8 characters in the path.
		//TODO: Wide strings
		std::cerr << IVUTIL::LOG_ERROR << e.what() << std::endl;
		MessageBox(nullptr, "This can be the result of the path containing UTF-8 characters.\n"
							"Check for invalid or suspect characters in folders or filename.", 
							"Could not resolve canonical file path!", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Determine if file is image type
	if (IVUTIL::formatSupport(IVG::PATH_IMAGE_FILE.extension().string()) < 0) {
		std::cerr << IVUTIL::LOG_ERROR << "Not a supported image format!" << std::endl;
		MessageBox(nullptr, "Please verify the file has a supported file extension.",
							"Invalid image format!", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Load up the image passed in
	if (loadTextureFromFile(win.renderer, IVG::PATH_IMAGE_FILE)) {
		std::cerr << IVUTIL::LOG_ERROR << IMG_GetError() << std::endl;
		MessageBox(nullptr, "Please verify the file is not corrupt or misformed.",
							"Could not load image!", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Update window title with image filename
	win.setTitle((IVG::PATH_IMAGE_FILE.filename().string() + " - " + IVUTIL::APPLICATION_TITLE).c_str());

	// Finally draw image and background
	draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? &TEXTURE_DARK : &TEXTURE_LIGHT, IVG::IMAGE_CURRENT->texture);

	// Iterate image folder and mark position of currently open image
	int pos = 0;
	for(auto& entry : std::filesystem::directory_iterator(IVG::PATH_IMAGE_FILE.parent_path())) {
		// Test that file is real file not link, folder, etc. and check it is a supported image format
		if (std::filesystem::is_regular_file(entry) && (IVUTIL::formatSupport(entry.path().extension().string()) >= 0)) {
			// Add file to list of images
			IVG::FILES_IMAGES_ADJACENT.push_back(entry.path());
			// If this is the image currently open, record position
			if (entry.path() == IVG::PATH_IMAGE_FILE) IVG::INDEX_IMAGE_FILE = pos;
			pos++;
		}
	}

	std::cout << IVUTIL::LOG_NOTICE << "Found " << IVG::FILES_IMAGES_ADJACENT.size() << " images adjacent." << std::endl;

	int mouseX;
	int mouseY;
	int mousePreviousX;
	int mousePreviousY;

	/* LIMIT REDRAW RATE:
		Instead of redrawing on every event and taking the hit waiting for VSYNC,
		if there are ANY changes requiring redraw, mark them, continue processing
		events and once the queue is empty, draw once.
	*/
	bool redraw = false;
	
	/* IMPROVE FRAMERATE MANAGEMENT:
		Rather than using a constant delay and relying on SDL's renderer vsync,
		manage time tracking here. Enables higher and more consistent frame rates.
	*/
	std::chrono::steady_clock::time_point time_before = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point time_after = std::chrono::steady_clock::now();
	int baseline_delay = 1000/(float) IVG::REFRESH_RATE;
	int execution_time = 0;

	// While application is running
	while (!quit) {
		// Handle events on queue
		while (SDL_PollEvent(&sdlEvent) != 0) {
			switch (sdlEvent.type) {
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
							redraw = true;
							break;
					}
					break;
				case SDL_KEYDOWN:
					switch (sdlEvent.key.keysym.sym) {
						case SDLK_SPACE:	//if gif, toggle pause/play
							if (IVG::IMAGE_CURRENT->animated) IVG::IMAGE_CURRENT->set_status(IVImage::STATE_TOGGLE);
							break;
						case SDLK_EQUALS:  //equals with plus secondary
						case SDLK_KP_PLUS: //or keypad plus, zoom in
							IVG::VIEWPORT_ZOOM = std::min(IVC::ZOOM_MAX, IVG::VIEWPORT_ZOOM * 2.0f);
							redraw = true;
							break;
						case SDLK_MINUS:    //standard minus
						case SDLK_KP_MINUS: //or keypad minus, zoom out
							IVG::VIEWPORT_ZOOM = std::max(IVC::ZOOM_MIN, IVG::VIEWPORT_ZOOM / 2.0f);
							redraw = true;
							break;
						case SDLK_0:    //standard 0
						case SDLK_KP_0: //or keypad 0, reset zoom and positioning
							resetViewport();
							redraw = true;
							break;
						case SDLK_ESCAPE: //quit
							quit = true;
							break;
						case SDLK_F1: //about
							MessageBox(nullptr, IVC::VERSION_ABOUT.c_str(), ("About " + IVUTIL::APPLICATION_TITLE).c_str(), MB_OK | MB_ICONINFORMATION);
							break;
						case SDLK_F2: { // explorer's properties dialog
							SHELLEXECUTEINFO sei;
							size_t path_size = (std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE]).string().length() + 1);
							char* new_path = (char *) malloc(path_size);

							memcpy(new_path, std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE]).string().c_str(), path_size);
							memset(&sei, 0, sizeof(SHELLEXECUTEINFO));

							sei.cbSize = sizeof(SHELLEXECUTEINFO);
							sei.lpFile = new_path;
							sei.nShow = SW_SHOWNORMAL;
							sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_INVOKEIDLIST;
							sei.lpVerb = "properties";
							ShellExecuteEx(&sei);

							free(new_path);
							} break;
						case SDLK_F3: { // open file's containing folder
							std::string function = "explorer.exe /select,\"" + std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE]).string() + "\"";
							// this could also be done with SHOpenFolderAndSelectItems but the setup and code required for that is extensive
							system(function.c_str());
							} break;
						case SDLK_TAB: //toggle light mode
							IVG::SETTINGS.DISPLAY_MODE_DARK = !IVG::SETTINGS.DISPLAY_MODE_DARK;
							redraw = true;
							break;
						case SDLK_DELETE: //delete image
							if (IDYES == MessageBox(nullptr, "Are you sure you want to permanently delete this image?\nThis action cannot be reversed!", "Delete Image", MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)) {
								try { //success
									std::filesystem::remove(IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE]);
									std::cout << IVUTIL::LOG_NOTICE << "File deleted: " << IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE].string() << std::endl;
								}
								catch (const std::filesystem::filesystem_error& e) { //failure
									std::cerr << IVUTIL::LOG_WARNING << "File could not be deleted: " << IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE].string() << std::endl;
									std::cerr << IVUTIL::LOG_WARNING << e.what() << std::endl;
									MessageBox(nullptr, "Image could not be deleted.", "Image Deletion Failure", MB_OK | MB_ICONERROR);
									break;
								}
								IVG::FILES_IMAGES_ADJACENT.erase(IVG::FILES_IMAGES_ADJACENT.begin() + IVG::INDEX_IMAGE_FILE);
								IVG::INDEX_IMAGE_FILE--;
								SDL_Event sdlENext;
								sdlENext.type = SDL_KEYDOWN;
								sdlENext.key.keysym.sym = SDLK_RIGHT;
								SDL_PushEvent(&sdlENext);
							}
							break;
						//previous image
						case SDLK_LEFT: //move back
							if (IVG::FILES_IMAGES_ADJACENT.size() == 1) break; //there's only one image in the folder so don't move
							if (IVG::INDEX_IMAGE_FILE == 0) IVG::INDEX_IMAGE_FILE = IVG::FILES_IMAGES_ADJACENT.size() - 1; //loop back to end of image file list
							else IVG::INDEX_IMAGE_FILE--;
							win.setTitle((IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE].filename().string() + " - " + IVUTIL::APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(win.renderer, std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE]))) { //if call returned non-zero, there was an error
								std::cerr << IVUTIL::LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							redraw = true;
							break;
						//next image
						case SDLK_RIGHT: //move forward
							if (IVG::FILES_IMAGES_ADJACENT.size() == 1) break; //there's only one image in the folder so don't move
							IVG::INDEX_IMAGE_FILE++;
							if (IVG::INDEX_IMAGE_FILE >= IVG::FILES_IMAGES_ADJACENT.size()) IVG::INDEX_IMAGE_FILE = 0; //loop back to start of image file list
							win.setTitle((IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE].filename().string() + " - " + IVUTIL::APPLICATION_TITLE).c_str()); //update window title
							if (loadTextureFromFile(win.renderer, std::filesystem::canonical(IVG::FILES_IMAGES_ADJACENT[IVG::INDEX_IMAGE_FILE]))) { //if call returned non-zero, there was an error
								std::cerr << IVUTIL::LOG_ERROR << IMG_GetError() << std::endl;
								break;
							}
							resetViewport(); //new image so reset zoom and positioning
							redraw = true;
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

						redraw = true;
					}
					else if (sdlEvent.wheel.y < 0) {
						IVG::VIEWPORT_ZOOM = std::max(IVC::ZOOM_MIN, IVG::VIEWPORT_ZOOM / IVC::ZOOM_SCROLL_SENSITIVITY);

						/* Correct positioning. */
						IVG::VIEWPORT_X /= IVC::ZOOM_SCROLL_SENSITIVITY;
						IVG::VIEWPORT_Y /= IVC::ZOOM_SCROLL_SENSITIVITY;

						redraw = true;
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
							redraw = true;
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
						redraw = true;
					}
					break;
				default:
					break;
			}
		}

		if (IVG::IMAGE_CURRENT->animated && IVG::IMAGE_CURRENT->ready) {
			IVG::IMAGE_CURRENT->prepare();
			redraw = true;
		}

		// If something happened that requires a redraw, process it
		if (redraw) {
			redraw = false;
			draw(&win, (IVG::SETTINGS.DISPLAY_MODE_DARK) ? &TEXTURE_DARK : &TEXTURE_LIGHT, IVG::IMAGE_CURRENT->texture);
		}

		// Track the amount of time if took to run loop and subtract it from time per frame to match refresh rate
		time_after = std::chrono::steady_clock::now();
		execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_after - time_before).count();
		SDL_Delay(std::max(0, baseline_delay - execution_time));
		time_before = std::chrono::steady_clock::now();

	}

	pushSettings(&win);
	IVUTIL::writeSettings(IVG::PATH_PROGRAM_CWD / IVC::FILENAME_SETTINGS, &IVG::SETTINGS);

	return 0;
}
