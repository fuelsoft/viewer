# Makefile for Image Viewer
# Nick Wilson - 2020

### CHANGE THESE VALUES ###

# Modify these to point to your 64 bit mingw g++ and windres
CXX = C:\\msys64\\mingw64\\bin\\x86_64-w64-mingw32-g++.exe
WINDRES = C:\\msys64\\mingw64\\bin\\windres.exe

# Modify these to point to your library include and lib folders
IC = -IC:\\lib\\x86_64\\include
LC = -LC:\\lib\\x86_64\\lib

### NO FURTHER CHANGES REQUIRED ###

INC_FILES = IVUtil.cpp main.cpp TiledTexture.cpp Window.cpp
WARNINGS = -Wextra -Wall
DEBUG = -Og -g
OPT = -O2
STD = -std=c++17

# If run with just 'make' default to the release build
Default: Release

# Debug build is simplified and easier to debug
Debug: $(INC_FILES)
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c IVUtil.cpp -o obj\\Debug\\IVUtil.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c main.cpp -o obj\\Debug\\main.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c TiledTexture.cpp -o obj\\Debug\\TiledTexture.o
	$(CXX) $(WARNINGS) $(STD) $(DEBUG) $(IC) -c Window.cpp -o obj\\Debug\\Window.o
	$(CXX) $(LC) -o bin\\Debug\\Viewer.exe obj\\Debug\\IVUtil.o obj\\Debug\\main.o obj\\Debug\\TiledTexture.o obj\\Debug\\Window.o -lmingw32 -lSDL2main -lSDL2.dll -lSDL2_image.dll -luser32 -lgdi32 -ldxguid

# Release build includes compiler optimization and executable metadata
Release: $(INC_FILES)
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c IVUtil.cpp -o obj\\Debug\\IVUtil.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c main.cpp -o obj\\Debug\\main.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c TiledTexture.cpp -o obj\\Debug\\TiledTexture.o
	$(CXX) $(WARNINGS) $(STD) $(OPT) $(IC) -c Window.cpp -o obj\\Debug\\Window.o
	$(WINDRES) -J rc -O coff -i $(CURDIR)\\meta\\meta.rc -o $(CURDIR)\\obj\\Release\\meta\\meta.res
	$(CXX) $(LC) -o bin\\Release\\Viewer.exe obj\\Release\\IVUtil.o obj\\Release\\main.o obj\\Release\\TiledTexture.o obj\\Release\\Window.o obj\\Release\\meta\\meta.res -s -static-libstdc++ -static-libgcc -static -O2 -lmingw32 -lSDL2main -lSDL2.dll -lSDL2_image.dll -luser32 -lgdi32 -ldxguid -mwindows

