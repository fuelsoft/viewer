# viewer
Load and display image files

## What is this?
I got so tired of Windows Photo Viewer misbehaving that I decided to write a clone.

Things WPV does that drove me to insanity:
* No support for animated GIFs.
    * I had to resort to opening GIFs in Internet Explorer, truly a cursed experience.
    * As of version 0.3.X, animated GIFs are nearly fully supported.
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

Also, it's GPU accelerated (via SDL's Accelerated Rendering features). There are some trade-offs:

* **PRO:** Ultra low CPU usage **once image is loaded -- this is still a CPU process.** 
* **PRO:** Panning and zooming are very smooth and take advantage of better GPU hardware rendering.
* **CON:** Startup takes a little longer than WPV due to the conversion of the input image to a GPU texture.
* **CON:** Image size limited by typically smaller VRAM size compared to RAM - *I have yet to find an image large enough to hit this limit though.*

## Requirements:
This project depends on:
* C++17 for std::filesystem
* MSYS2 for MingW64 and required libraries

Packages you'll need, as they appear in MSYS:
* mingw-w64-x86_64-gcc
* mingw-w64-x86_64-make
* mingw-w64-x86_64-SDL2
* mingw-w64-x86_64-SDL2_image
* mingw-w64-x86_64-giflib

## Building:
Run `make <type>` in the Viewer folder where type is either `Debug` or `Release` (defaults to `Release`).
#### Notes:
* If your `make.exe` isn't on your PATH, you need to launch it via it's complete path or hard link it into the Viewer directory.
* You will need to modify the makefile at the marked lines to point to your mingw64 install (for `g++` and `windres`).
* You will also need to point to the library include and lib folders - they shouldn't require much modification unless MSYS is installed somewhere other than default.
* These are mostly just the build commands as CodeBlocks runs them - they may not always be up to date enough to build the project without modification.

## Usage:
First, ensure that the executable has all the `.dll` files next to it. There are two groups you need:
* `SDL2.dll` from the SDL2 download above.
* `SDL2_image.dll` & all the related image-type DLLs from the SDL_image download above.

Then call the program by either dragging an image onto Viewer.exe or by running `Viewer.exe <filename>` in a terminal.

You can also set Viewer as the default program for some image formats if you want to commit to it.
### Controls:

##### Mouse:

|CONTROL|FUNCTION|NOTES|
|:-----:|:---:|:---:|
|SCROLL UP|Increase zoom|-|
|SCROLL DOWN|Decrease zoom|-|
|LEFT CLICK|Pan image|Must be held while dragging.|
|MIDDLE CLICK|Reset zoom and pan|This is usually a scroll wheel click.|
|MOUSE *X1*|Previous image|If the mouse is equipped with a *back* button.|
|MOUSE *X2*|Next image|If the mouse is equipped with a *forward* button.|

##### Keyboard:

|SYMBOL|CONTROL|FUNCTION|NOTES|
|:---:|:---:|:---:|:---:|
|+|PLUS|Increase zoom|For keyboards without keypad, use equals (`=`).|
|-|MINUS|Decrease zoom|-|
|0|ZERO|Reset zoom and pan|Zero, not the letter O.|
|ESC|ESCAPE|Close program|Identical to window *close* button.|
|<\-|LEFT ARROW|Previous image|-|
|\->|RIGHT ARROW|Next image|-|
|\|<\->\||TAB|Toggle light theme|-|
|␣|SPACEBAR|Play/Pause animation|Only applicable to animated images.|
|DEL|DELETE|Delete image|**Permanent!** This will **not** go to Recycle Bin!|
|F1|F1|View version info|Probably not useful to you if you're reading this.|
|F2|F2|View file info|Display Windows Explorer's *Properties* window for the image.|
|F3|F3|Containing folder|Launches Windows Explorer to the image's parent folder.|

### Supported formats:
As listed [here](https://www.libsdl.org/projects/SDL_image/docs/SDL_image.pdf#page=8&zoom=auto,-205,547). 

#### Notes:
* Some file types can contain multiple resolutions/variations of an image (ICO, CUR, ...). SDL_image [documentation][1] states that "for files with multiple images, the first one found with the highest color count is chosen."
* Window size and position are stored in `settings.cfg` in the program folder. If this becomes broken somehow (window appears off-screen, etc.) it is safe to delete this file to restore defaults.
* Setting up a version of GCC new enough to support C++17 is a really bad time on Windows and unless you really need to build from source, I highly recommend you use one of the builds. Otherwise, I'll direct you to [MSYS2](https://www.msys2.org/)'s homepage.

### TODO:
* Add key to set zoom to 1:1 pixel ratio
* Zoom on mouse instead of window center
* Stop image from being moved off-screen
* Implement HEIF support
* Possible **minimal** RAW support

### Known Problems:
* ICO: Files stored as "NEW PNG" type do not load with SDL_image
* ICO: Files with partially transparent pixels do not render correctly
* ALL: Images loaded from RO-type media (cell phones, cameras) are copied to a temp folder by Windows before being loaded. 
This breaks in-folder navigation. It's unclear to me how WPV works around this.
* ALL: Images with UTF-8 characters anywhere in the path are broken - this is because of the conversion from wchar string to char string done to accommodate SDL_Image. I can't find anything online about how to work around this.

[1]: https://www.libsdl.org/projects/SDL_image/docs/SDL_image.pdf#page=21&zoom=auto,-205,720
