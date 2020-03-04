# viewer
Load and display image files

## What is this?
I got so tired of Windows Photo Viewer misbehaving that I decided to write a clone.

Things WPV does that drove me to insanity:
* No support for animated GIFs.
    * I had to resort to opening GIFs in Internet Explorer, truly a cursed experience.
* "Windows Photo Viewer can't display this picture because there might not be enough memory available on your computer"
    * Let me assure you, this is not correct.
    * Afflicted photos open correctly in Photoshop, web browsers, etc. It's just WPV that has a problem.
* Very low FPS and bad screen tearing with no warning when zooming / panning while zoomed in.
    * Not a big deal with a static image but it's really annoying and there's no good reason for it.

Surely something I wrote couldn't be any worse, right?

Things *Viewer* can do:
* Display most common image formats.
* Zoom!
* Pan!
* Wow!

## Requirements:
This project depends on:
* [SDL2](https://www.libsdl.org/download-2.0.php)
* [SDL_image](https://www.libsdl.org/projects/SDL_image/)
 
That's it. So far.

This of course is being written for Windows. It should work with little additional work on Linux but Linux already has many excellent (and complete) image viewers that I would recommend over this.
## Building:
This is where it gets tricky. I'm using Code::Blocks to write this, which abstracts away the build system.

It should work with little extra effort by just plugging the CPP files into GCC and linking in SDL2 and SDL_image.

I am working on migrating to a makefile-type build to simplify this.

## Usage:
First, ensure that the executable has all the `.dll` files next to it. There are two groups you need:
* `SDL2.dll` from the SDL2 download above.
* `SDL2_image.dll` & all the related DLLs from the SDL_image download above.

Then call the program by either dragging an image onto Viewer.exe or by running `Viewer.exe <filename>` in a terminal.

### Controls:
* Zoom is handled by hitting keypad +/- or scrolling. 
* Panning is handled by left-clicking and dragging the image. 
* Zoom and position can be reset by hitting keypad 0 (keypad zero).

### Supported formats:
As listed [here](https://www.libsdl.org/projects/SDL_image/docs/SDL_image.pdf#page=8&zoom=auto,-205,547).

#### Notes:
* Some file types can contain multiple resolutions/variations for an image (ICO, CUR, ...). SDL_image [documentation][1] states that "for files with multiple images, the first one found with the highest color count is chosen."
* Animated GIFs will only show the first frame as an SDL_image limitation. Additional external libraries will be required to view animations. TBA.

### TODO:
This list is maintained in main.cpp as I think of things - check there for an up to date copy.
* Remember window size and position
* Add key to set zoom to 1:1 pixel ratio
* Add key to change sampling mode
* Filename in title
* Transparent background texture
* Possible: partial image metadata? PNG image data support from ProjectPNG?
* Navigate forward/backwards in current directory
* Animated GIFs
* Zoom on mouse instead of window center

[1]: https://www.libsdl.org/projects/SDL_image/docs/SDL_image.pdf#page=21&zoom=auto,-205,720