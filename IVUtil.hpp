/*
IVUTIL.HPP
NICK WILSON
2020
*/

#include <SDL2/SDL.h>

#include <cstdint>		//standard number formats
#include <string>		//string type
#include <iostream>		//console io
#include <fstream>		//file io
#include <filesystem>	//filesystem components

#ifndef IVUTIL_H
#define IVUTIL_H

namespace IVUTIL {

	/* LOGGING */
	const std::string APPLICATION_TITLE = "Image Viewer";
	const std::string LOG_ERROR = "<ERROR> ";
	const std::string LOG_WARNING = "<WARNING> ";
	const std::string LOG_NOTICE = "<NOTICE> ";

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

	/* Home cooked exception codes */
	enum IVEXCEPT {
		EXCEPT_IMG_LOAD_FAIL,
		EXCEPT_IMG_OPEN_FAIL,
	};

	// This the data read from and written to settings file.
	// Adding a new value here will cause old config files to be discarded on first read.
	struct IVSETTINGS {
		int WIN_X;
		int WIN_Y;
		int WIN_W;
		int WIN_H;
		bool MAXIMIZED;
		bool DISPLAY_MODE_DARK;
	};

	int formatSupport(std::string extension);

	void readSettings(std::filesystem::path target, IVSETTINGS* settings);
	
	void writeSettings(std::filesystem::path target, IVSETTINGS* settings);
};

#endif
