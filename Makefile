# Makefile for Image Viewer
# Nick Wilson - 2020

### CHANGE THESE VALUES ###

# Modify these to point to your 64 bit mingw g++ and windres
CXX = C:\\msys64\\mingw64\\bin\\x86_64-w64-mingw32-g++.exe
WINDRES = C:\\msys64\\mingw64\\bin\\windres.exe

# Modify these to point to your library include(s) and lib(s) folders
IC = -IC:\\msys64\\mingw64\\include
LC = -LC:\\msys64\\mingw64\\lib

### NO FURTHER CHANGES REQUIRED ###

# Include local directory to simplify includes
IC := $(IC) -I.

INC_FILES = IVUtil.cpp main.cpp subclasses\\IVAnimatedImage.cpp subclasses\\IVStaticImage.cpp subclasses\\TiledTexture.cpp subclasses\\Window.cpp
WARNINGS = -Wextra -Wall
DEBUG = -Og -g
OPT = -O2
STD = -std=c++17
LIBS = -lmingw32 -lSDL2main -lSDL2.dll -lSDL2_image.dll -luser32 -lgdi32 -ldxguid -lgif

# If run with just 'make' default to the release build
Default: Release

# Debug build is simplified and easier to debug
Debug: $(INC_FILES)
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c IVUtil.cpp -o obj\\Debug\\IVUtil.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c main.cpp -o obj\\Debug\\main.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c subclasses\\IVAnimatedImage.cpp -o obj\\Debug\\subclasses\\IVAnimatedImage.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c subclasses\\IVStaticImage.cpp -o obj\\Debug\\subclasses\\IVStaticImage.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c subclasses\\TiledTexture.cpp -o obj\\Debug\\subclasses\\TiledTexture.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c subclasses\\Window.cpp -o obj\\Debug\\subclasses\\Window.o
	$(CXX) $(LC) -o bin\\Debug\\Viewer.exe obj\\Debug\\IVUtil.o obj\\Debug\\main.o obj\\Debug\\subclasses\\IVAnimatedImage.o obj\\Debug\\subclasses\\IVStaticImage.o obj\\Debug\\subclasses\\TiledTexture.o obj\\Debug\\subclasses\\Window.o $(LIBS)

# Release build includes compiler optimization and executable metadata
Release: $(INC_FILES)
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c IVUtil.cpp -o obj\\Debug\\IVUtil.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c main.cpp -o obj\\Debug\\main.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c subclasses\\IVAnimatedImage.cpp -o obj\\Debug\\subclasses\\IVAnimatedImage.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c subclasses\\IVStaticImage.cpp -o obj\\Debug\\subclasses\\IVStaticImage.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c subclasses\\TiledTexture.cpp -o obj\\Debug\\subclasses\\TiledTexture.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c subclasses\\Window.cpp -o obj\\Debug\\subclasses\\Window.o
	$(WINDRES) -J rc -O coff -i $(CURDIR)\\meta\\meta.rc -o $(CURDIR)\\obj\\Release\\meta\\meta.res
	$(CXX) $(OPT) $(LC) -o bin\\Release\\Viewer.exe obj\\Release\\IVUtil.o obj\\Release\\main.o obj\\Release\\subclasses\\IVAnimatedImage.o obj\\Release\\subclasses\\IVStaticImage.o obj\\Release\\subclasses\\TiledTexture.o obj\\Release\\subclasses\\Window.o obj\\Release\\meta\\meta.res -s -static-libstdc++ -static-libgcc -static $(LIBS) -mwindows

