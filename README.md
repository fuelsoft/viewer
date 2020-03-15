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
* Very low FPS and bad screen tearing with no warning when zooming or when panning while zoomed in.
    * Not a big deal with a static image but it's really annoying and there's no good reason for it.

How hard could it be to write a replacement, right?

Things *Viewer* can do:
* Display most common image formats.
* Zoom!
* Pan!
* Wow!

Also, it's GPU accelerated (via SDL's Accelerated Rendering features). There are trade-offs here:

* **PRO:** Ultra low CPU usage **once image is loaded -- this is still a CPU process.** 
* **PRO:** Panning and zooming are very smooth and take advantage of better GPU hardware rendering.
* **CON:** Startup takes a little longer than WPV due to the conversion of the input image to a GPU texture.
* **CON:** Image size limited by typically smaller VRAM size compared to RAM - *I have yet to find an image large enough to hit this limit though.*

## Requirements:
This project depends on:
* [SDL2](https://www.libsdl.org/download-2.0.php)
* [SDL_image](https://www.libsdl.org/projects/SDL_image/)
* C++17 for std::filesystem
 
That's it. So far.
## Building:
This is where it gets tricky. I'm using Code::Blocks to write this, which abstracts away the build system.

It should work with little extra effort by just plugging the CPP files into GCC and linking in SDL2 and SDL_image.

I will work on migrating to a makefile-type build to simplify this at some point.

## Usage:
First, ensure that the executable has all the `.dll` files next to it. There are two groups you need:
* `SDL2.dll` from the SDL2 download above.
* `SDL2_image.dll` & all the related image-type DLLs from the SDL_image download above.

Then call the program by either dragging an image onto Viewer.exe or by running `Viewer.exe <filename>` in a terminal.

### Controls:
* Zoom is handled by hitting keypad +/- or scrolling. 
* Panning is handled by left-clicking and dragging the image. 
* Zoom and position can be reset by hitting keypad 0 (keypad zero) or center-clicking the mouse.
* Quit by closing the window or hitting Escape.
* Navigave to previous/next image in folder with the left and right arrow keys or back/forward buttons on your mouse (if equipped).

### Supported formats:
As listed [here](https://www.libsdl.org/projects/SDL_image/docs/SDL_image.pdf#page=8&zoom=auto,-205,547).

#### Notes:
* Some file types can contain multiple resolutions/variations of an image (ICO, CUR, ...). SDL_image [documentation][1] states that "for files with multiple images, the first one found with the highest color count is chosen."
* Window size and position are stored in `settings.cfg` in the program folder. If this becomes broken somehow (window appears off-screen, etc.) it is safe to delete this file to restore defaults.
* Setting up a version of GCC new enough to support C++17 is a really bad time on Windows and unless you really need to build from source, I highly recommend you use one of the builds. Otherwise, I'll direct you to [MSYS2](https://www.msys2.org/)'s homepage.

### TODO:
This list is maintained in `main.cpp` as I think of things - check there for an up to date copy.
* Add key to set zoom to 1:1 pixel ratio
* Possible: partial image metadata? PNG image data support from ProjectPNG?
* Animated GIFs
* Zoom on mouse instead of window center
* Stop image from being moved off-screen
* Possible: Image deletion?
* Possible: Image rotation?
* Cursor icon state updates?
* Don't exit if passed no parameters... show pop-up?
* Handle capitalization in file extensions

### Known Problems:
* ICO: Files stored as "NEW PNG" type do not load with SDL_image
* ICO: Files with partially transparent pixels do not render correctly
* GIF: Animated sequences only load the first frame

[1]: https://www.libsdl.org/projects/SDL_image/docs/SDL_image.pdf#page=21&zoom=auto,-205,720