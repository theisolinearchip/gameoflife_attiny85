# Game of Life on an attiny85
Classic Conway's **Game of Life** cellular automata running on an **attiny85** with an **SSD1306-based OLED 128x64 mini-display**.

![Game of Life 1](http://albertgonzalez.coffee/projects/gameoflife_attiny85/final_1_small.jpg?) ![Game of Life 2](http://albertgonzalez.coffee/projects/gameoflife_attiny85/final_2_small.jpg) 

## Features

- 32x16 grid
- Classic rules
- Custom made SSD1306 library that allows a small attiny85 handle the full 128x64 screen (by using 32x16 "internal buffers" that are translated into "big pixels" on the real 128x64 OLED screen grid)
- Dead-end detection (auto-reset when the automaton "doesn't change anymore")
- Reset button

## The OLED screen drivers

This is probably the most interesting part of the project: how I manage to made a library to send whatever commands I need to the driver chip and work with small _dynamic buffers_, since the "original ones" (128x64 pixels, that means 128x64/8 = 1024 bytes) were way to big to fit in the 512 bytes of RAM the **attiny85** has (and using the flash memory wasn't a valid option).

There's more info about how it works and why I took this approach in my [hackaday.io project page](https://hackaday.io/project/181421-game-of-life-on-an-attiny85).
