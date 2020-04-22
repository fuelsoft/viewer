/*
IVUTIL.CPP
NICK WILSON
2020
*/

#include "IVUtil.hpp"

/**
* formatSupport	- Compare file extension provided to known list to confirm support
* extension 	> File extension as String
* return - int 	< FILE_TYPE of file type or -1 if no match
*/
int IVUTIL::formatSupport(std::string extension) {
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
			if (!extensionUppercase.compare("JPG"))  return JPG;
			if (!extensionUppercase.compare("JPEG")) return JPG;
			break;
		case 'P':
			/* PNG */
			if (!extensionUppercase.compare("PNG"))  return PNG;
			break;
		case 'G':
			/* GIF */
			if (!extensionUppercase.compare("GIF"))  return GIF;
			break;
		case 'B':
			/* BMP */
			if (!extensionUppercase.compare("BMP"))  return BMP;
			break;
		case 'T':
			/* TIF(F) */
			if (!extensionUppercase.compare("TIF"))  return TIF;
			if (!extensionUppercase.compare("TIFF")) return TIF;
			/* TGA */
			if (!extensionUppercase.compare("TGA"))  return TGA;
			break;
		default:
			return -1;
	}

	return -1;
}

/**
* readSettings - Load settings file if possible and recover window position and size.
*/
void IVUTIL::readSettings(std::filesystem::path target, IVSETTINGS* settings) {
	//open filestream to read file at the end
	std::ifstream inputStream(target, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	if (inputStream.is_open()) {
		//current position is end, this will present filesize
		int fileLength = inputStream.tellg();
		if (fileLength == -1) { //read failed somewhere in iostream
			std::cerr << LOG_NOTICE << "Failed to read settings file!" << std::endl;
		}
		else if ((unsigned) fileLength < sizeof(IVSETTINGS)) { //settings file is too short
			std::cerr << LOG_WARNING << "Incorrect settings data size! [Expected " << sizeof(IVSETTINGS) << ", got " << fileLength << "]" << std::endl;
		}
		else { //no problems encountered
			inputStream.seekg(std::ios_base::beg);
			inputStream.read(reinterpret_cast<char *>(settings), sizeof(IVSETTINGS));
		}
		inputStream.close();
	}
	else { //load defaults
		std::cerr << LOG_NOTICE << "No settings file located!" << std::endl;
	}
}

/**
* writeSettings - Write to settings file, if it can be written in a valid location
*/
void IVUTIL::writeSettings(std::filesystem::path target, IVSETTINGS* settings) {
	std::ofstream outputStream(target, std::ifstream::out | std::ifstream::binary | std::ifstream::trunc);
	if (outputStream.is_open()) {
		outputStream.write(reinterpret_cast<char *>(settings), sizeof(IVSETTINGS));
		outputStream.close();
	}
	else {
		std::cerr << LOG_ERROR << "Could not write to settings file!" << std::endl;
	}
}
