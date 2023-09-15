# PicoVision: Hardware <!-- omit in toc -->

PicoVision is a microcontroller-powered, HDMI stick, using two RP2040's - one Pico W as a "CPU" and one RP2040 as a "GPU" - to produce high (for a microcontroller) resolution HDMI output by swapping two physical - PSRAM backed - frame-buffers back and forth between them.

The two PSRAMs act as a front and back buffer. While the "CPU" (the Pico W) writes to one, the "GPU" (RP2040) reads from the other, applies some "hardware" effects and generates the HDMI signals.

- [GPU Features](#gpu-features)
  - [Display Resolutions \& Pen Types](#display-resolutions--pen-types)
    - [RGB555 or P5 Resolutions](#rgb555-or-p5-resolutions)
    - [Widescreen Modes](#widescreen-modes)
    - [Pixel-doubled Resolutions](#pixel-doubled-resolutions)
    - [Frame vs Display](#frame-vs-display)
  - [Sprites](#sprites)
    - [PicoVision Sprite .pvs Format](#picovision-sprite-pvs-format)
  - [Scanlines](#scanlines)

## GPU Features

### Display Resolutions & Pen Types

PicoVision supports three pen types- RGB888, RGB555 and P5 (32 colour palette mode).

These require three, two and one bytes per pixel respectively. Generally the larger a pen mode (in bytes) the lower its max supported resolution.

#### RGB555 or P5 Resolutions

* 720 x 576 - 5:4 - PAL
* 720 x 480 - 3:2 - NTSC
* 640 x 480 - 4:3 - VGA

#### Widescreen Modes

Some additional (P5 and RGB555) modes are supported with the optional widescreen build of PicoVision's MicroPython firmware:

* 800 x 450 - 16:9
* 800 x 480 - 5:3 - WVGA
* 800 x 600 - 4:3 - SVGA
* 960 x 540 @ 50Hz - 16:9 - 540p / qHD
* 1280 x 720 @ 30Hz - 16:9 - Limited compatibility with displays

:warning: Note that some of these modes run the GPU extremely overclocked, and also use a clock pulse width for the PSRAM that is outside of the range specified in the datasheet.  So while these do work on most hardware, we can't guarantee they will work on all PicoVisions.

#### Pixel-doubled Resolutions

PicoVision's "GPU" can pixel double along the horizontal or vertical axis, opening up a range of weirder resolution options that use half the width or height of a supported resolution and can support larger pen types.

If you're pixel doubling to a widescreen mode, you'll still need the widescreen build.

RGB888 is only supported when using horizontal pixel doubling. Other than that restriction, it can use all the same modes as RGB555 and P5.

Some examples:

* 320 x 240 - 4:3 (640 x 480) - HVGA - Good for DOS, NES, SNES, SMS, SMD
* 360 x 240 - 3:2 (720 x 480)
* 360 x 288 - 5:4 (720 x 576) - Close to CIF
* 400 x 240 - 5:3 (800 x 480) - WQVGA
* 960 x 270 - 32:9 (960 x 540)

These can be used to accomplish non-square pixels, for example if your screen aspect ratio is 16:9 and you output 960 x 270 your pixels will appear twice as tall as they are wide.

If you output 480 x 540 your pixels will now appear twice as wide as they are tall.

:warning: The actual shape your pixels appear will be a function of your mode's aspect ratio and your display's aspect ratio. Simply displaying a 4:3 mode such as 320 x 240 on a 16:9 display will stretch your pixels slightly horizontally.

#### Frame vs Display

Since the PSRAM frame buffers are larger than necessary for storing frame data, it's possible to store a larger frame and display only a portion of it.

### Sprites

The RP2040 serving as a GPU on PicoVision supports blending a handful of additional images over your frame data. These are called sprites.

A sprite refers to a single slot on the GPU, which is capable of displaying an image at a specific X/Y coordinate with vertical scaling and blend options.

The number of sprites you can display at once is governed by the size of the GPU's active sprite buffer. This buffer is 64kB (or 32kB in a widescreen build) and is used to load all of the active sprites into RAM during VSync so they can be blended into the frame scanline by scanline- chasing the beam.

There is, additionally, a hard limit of 32 sprites. (TODO: Can we raise this to 128 with 16x16 pixel sprites assumed to be the canonical size, to match the optimised Tilemap rendering? This really opens the door for sprite powered games)

Each sprite can reference an image up to 8kB in size, this equates to 64x64 pixels in RGB555 mode. Larger sprites will occupy more of the active sprite buffer and permit fewer similtaneous active sprites. In fact with 8kB, 64x64 pixel sprites you would be limited to only eight onscreen sprites at once.

Larger images also occupy more consecutive image indices, for example a 64x64 (8kB) image loaded at slot 1 would occupy indices 1, 2, 3 and 4. (TODO: If we raise the sprite limit and assume 16x16 then sprites will occupy more slots, I think we should make this work like `create_pen` and simply have `load_sprite` return the relevant slot IDs. This would tie into `load_animation` which would also have to work out the correct slot IDs based on image size so it would be ideal if the DVDisplay driver itself kept track of this, making `define_sprite` return the relevant starting index- I'm not sure how this would work across the two, potentially and maybe deliberately out of sync PSRAM buffers.)

Displaying this 64x64 image will have the same cost as displaying four 32x32 pixel sprites, limiting you to fewer consecutive onscreen sprites than the normal 32 (or 16) limit. You can balance this with smaller sprites to return to the 32x32 limit.

In summary:

1. There's a hard limit of 32 sprites on screen and a total of 64kB active sprite data
2. There's a hard limit of 10 simultaneous sprites per scanline
3. Each sprite can be up to 8kB in size, with a maximum width or height of 64 pixels
4. Sprite images are stored in PSRAM in "indexed" locations
5. A single sprite, referenced by its "slot", specifies an index for the image data, X and Y coordinates, vertical scale and blend mode
6. Larger images can be used, but will occupy more of the 64kB active sprite data and so potentially limit you to fewer sprites

#### PicoVision Sprite .pvs Format

If you want to streamline loading sprite image data, you can precompose your images into the following format:

```
Sprite entry:
  1 byte: Width
  1 byte: Height

  For each line:
    1 byte: Offset of first pixel on the line
    1 byte: Line width

  2 bytes: Padding if height is even

  Height times:
    Line width times:
      Sprite pixel data (uint16_t ARGB1555, or uint8_t P5)
```

For P5 the internal pixel format for the pvs file has the palette entry in bits 6-2 and the alpha value in bit 0.

### Scanlines

The GPU works with scanlines, "chasing the beam" to render sprites and apply scrolling effects on the fly.

During VSync, data is loaded into the active sprite buffer in GPU RAM (that's the internal memory of the RP2040 serving as the GPU) since it's not possible to stream it from PSRAM for each scanline at the same time as reading in the scanline data.

Additionally a series of sprite patch buffers - there are ten per scanline - are configured, pointing to the relevant sprite data for each scanline.

When the scanline is being written out to the screen, the patches from the sprite patch buffers are blended into the line immediately before it is encoded into HDMI symbolsfor the display.
