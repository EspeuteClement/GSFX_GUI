# Gamebuino Sound FX Creation Tool
---
## The tool
This tool was designed as an easy and interactive method for creating sound FX directly on your Gamebuino. It features 64 saves slots (requires an SD card), export via Serial Monitor and up to 8 Sound per FX
## Terms
- _FX_ : A series of sounds that you can play in your game
- _Sound_ : The smallest part of FX. Each sound has a given *Length*, *Volume*, *Period* (for sound pitching) and a *Waveform*. Volume and Period can be altered by sliding up or down the parameter through time

## How to create Sounds

### Controls :
UP/DOWN : Navigate
LEFT/RIGHT : Change value / Navigate
A : Hold to change value faster / Validate
B : Play your sound / Cancel
Menu : Open Menu Bar

## How to play your sound in your own program
To use the sound you created, you need to export export them, to do that, follow these steps :
1. Connect your Gamebuino to your PC via an USB cable
2. Launch your Arduino IDE
3. Go to `Tools > Serial Monitor`
4. On your Gamebuino, press the <key>Menu</key> key and navigate to `file > Export`
5. Some code should appear in the Serial Monitor on your PC.

Now you have two choices :
- If you want to use your sound right away, simply copy the second part of the code that should look like the sample below in the .cpp or .ino where you want to use your sound.
```c++
const Gamebuino_Meta::Sound_FX sfx_0[] = {
    ...
};
```
Then, you just need to call `gb.sound.fx(sfx_0);` when you want your sound to play.

- If you want to use your FX in various places of your program, it's recomended that you use a dedicated `.h` and `.cpp` file. In order to do that, follow these steps :
    + Create a `.h` and `.cpp` with the name that you want (like `sfx.h` for example).
    + In your `.h` file, include Gamebuino-Meta.h like so `#include <Gamebuino-Meta.h>`, add some header guards `#pragma once` then copy the first part of the output in the file.
    + In the `.cpp` file, include your `.h` like so   `#include "sfx.h"` and copy the second part of the output in the file

    + If everything is correct, you should have this
    ```c++
    // Your .H file
    #include <Gamebuino-Meta.h>
    #pragma once

    extern const Gamebuino_Meta::Sound_FX sfx_0[];
    extern const Gamebuino_Meta::Sound_FX sfx_1[];
    extern const Gamebuino_Meta::Sound_FX sfx_2[];
    ...

    // Your .CPP file
    #include "sfx.h"

    const Gamebuino_Meta::Sound_FX sfx_0[] = { ... };
    const Gamebuino_Meta::Sound_FX sfx_1[] = { ... };
    const Gamebuino_Meta::Sound_FX sfx_2[] = { ... };
    ...
    ```
    + If you want to play a FX, simply include your `.h` file like so `#include "sfx.h"` and call `gb.sound.fx(sfx_0);`


