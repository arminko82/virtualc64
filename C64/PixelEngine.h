/*
 * (C) 2015 Dirk W. Hoffmann. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// CLEANUP:
// in draw() dc.delay is set to gs_delay.
// If this value is latched in preparePixelEngine... , gs_delay is not needed any more(?)

// TODO:
//
// 1. Texture does not extent to the bottom
//
// 2. Make sprite drawing cycle based.


#ifndef _PIXELENGINGE_INC
#define _PIXELENGINGE_INC

#define DIRK_DEBUG_LINE 77 // REMOVE ASAP

#include "VirtualComponent.h"


// Forward declarations
class VIC;
class C64;

//! Display mode
enum DisplayMode {
    STANDARD_TEXT             = 0x00,
    MULTICOLOR_TEXT           = 0x10,
    STANDARD_BITMAP           = 0x20,
    MULTICOLOR_BITMAP         = 0x30,
    EXTENDED_BACKGROUND_COLOR = 0x40,
    INVALID_TEXT              = 0x50,
    INVALID_STANDARD_BITMAP   = 0x60,
    INVALID_MULTICOLOR_BITMAP = 0x70
};

//! PixelEngine
/*! This component is part of the virtual VICII chip and encapulates all functionality that is related to the
    synthesis of pixels. Its main entry point are prepareForCycle() and draw() which are called in every 
    VIC cycle inside the viewable range.
*/
class PixelEngine : public VirtualComponent {
    
    friend class VIC;
    
public:

    //! Reference to the connected video interface controller (VIC)
    VIC *vic;
    
    //! Constructor
    PixelEngine(C64 *c64);
    
    //! Destructor
    ~PixelEngine();
    
    //! Restore initial state
    void reset();
    
    //! Size of internal state
    uint32_t stateSize() { return 0; }
    
    //! Load state
    void loadFromBuffer(uint8_t **buffer) { }
    
    //! Save state
    void saveToBuffer(uint8_t **buffer) { }

    
    // -----------------------------------------------------------------------------------------------
    //                                     Constant definitions
    // -----------------------------------------------------------------------------------------------

    //! Predefined color schemes
    enum ColorScheme {
        CCS64           = 0x00,
        VICE            = 0x01,
        FRODO           = 0x02,
        PC64            = 0x03,
        C64S            = 0x04,
        ALEC64          = 0x05,
        WIN64           = 0x06,
        C64ALIVE_0_9    = 0x07,
        GODOT           = 0x08,
        C64SALLY        = 0x09,
        PEPTO           = 0x0A,
        GRAYSCALE       = 0x0B
    };
    
    //! VIC colors
    enum Color {
        BLACK   = 0x00,
        WHITE   = 0x01,
        RED     = 0x02,
        CYAN    = 0x03,
        PURPLE  = 0x04,
        GREEN   = 0x05,
        BLUE    = 0x06,
        YELLOW  = 0x07,
        LTBROWN = 0x08,
        BROWN   = 0x09,
        LTRED   = 0x0A,
        GREY1   = 0x0B,
        GREY2   = 0x0C,
        LTGREEN = 0x0D,
        LTBLUE  = 0x0E,
        GREY3   = 0x0F
    };
    
    //! Maximum number of viewable pixels per rasterline
    /*! TODO: 418 MIGHT BE TOO LARGE. CHECK HOW MUCH THE BORDER EXTENDS TO THE LEFT AND RIGHT
        IN BOTH PAL AND NTSC MODE. */
    static const uint16_t MAX_VIEWABLE_PIXELS = 418;

    
    // -----------------------------------------------------------------------------------------------
    //                                    Pixel buffers and colors
    // -----------------------------------------------------------------------------------------------
    
private:
    
    //! Currently used color scheme
    /*! Each color scheme uses slightly different RGB values to mimic the colors used in different emulators.
        Changing the color scheme modifies the colors[] array which stores the actual color codes in RGBA format. */
    /*! TODO: MOVE COLORSCHEME TO GUI CODE. IT'S A "LIFESTYLE" PROPERTY AND NOT A MACHINE FEATURE */
    ColorScheme colorScheme;
    
    //! All 16 C64 colors in RGBA format
    uint32_t colors[16];
    
    //! First screen buffer
    /*! The VIC chip writes it output into this buffer. The contents of the array is later copied into to
        texture RAM of your graphic card by the drawRect method in the OpenGL related code. */
    int screenBuffer1[512 * 512];
    
    //! Second screen buffer
    /*! The VIC chip uses double buffering. Once a frame is drawn, the VIC chip writes the next frame to the 
        second buffer. */
    int screenBuffer2[512 * 512];
    
    //! Target screen buffer for all rendering methods
    /*! The variable points either to screenBuffer1 or screenBuffer2 */
    int *currentScreenBuffer;
    
    //! Pointer to the current rasterline
    /*! This pointer is used by all rendering methods to write pixels. It always points to the beginning of a
        rasterline, either in screenBuffer1 or screenBuffer2. It is reset at the beginning of each frame and 
        incremented at the beginning of each rasterline. */
    int *pixelBuffer;
    
    //! Temporary pixel source
    /*! Data is first created here and later copied to pixelBuffer */
    int pixelBufferTmp[2];
    
    //! Z buffer
    /*! Virtual VICII uses depth buffering to determine pixel priority. In the various render routines, a pixel is 
        only written to the screen buffer, if it is closer to the view point. The depth of the closest pixel is kept 
        in the z buffer. The lower the value of the z buffer, the closer it is to the viewer.
        The z buffer is cleared before a new rasterline is drawn.
     */
    int zBuffer[MAX_VIEWABLE_PIXELS];
    
    //! Indicates the source of a drawn pixel
    /*! Whenever a foreground pixel or sprite pixel is drawn, a distinct bit in the pixelSource array is set.
     The information is utilized to detect sprite-sprite and sprite-background collisions.
     */
    int pixelSource[MAX_VIEWABLE_PIXELS];
    
public:
    
    //! Set color scheme
    void setColorScheme(ColorScheme scheme);

    //! Get screen buffer that is currently stable
    /*! This method is called by the OpenGL code at the beginning of each frame. */
    inline void *screenBuffer() { return (currentScreenBuffer == screenBuffer1) ? screenBuffer2 : screenBuffer1; }

    
    // -----------------------------------------------------------------------------------------------
    //                                    Execution functions
    // -----------------------------------------------------------------------------------------------

public:
    
    //! Prepare for new frame
    void beginFrame();
    
    //! Prepare for new rasterline
    void beginRasterline();
    
    //! Finish up rasterline
    void endRasterline();
    
    //! Finish up frame
    void endFrame();

    
    // -----------------------------------------------------------------------------------------------
    //                                   VIC state latching
    // -----------------------------------------------------------------------------------------------

    //! Latched VIC state
    /*! To draw pixels right, it is important to gather the necessary information at the right time. 
        Some VIC and memory registers need to be looked up one cycle before drawing, others need
        to be looked up at the same cycle or even in the middle of drawing an 8 pixel chunk. To make
        this process transparent, all gatheres information is stored in this structure. */

    struct {
        // Gathered one cycle before drawing (in prepareForCycle)
        uint8_t cycle;
        uint32_t yCounter;
        int16_t xCounter;
        bool verticalFrameFF;
        bool mainFrameFF;
        uint8_t data;
        uint8_t character;
        uint8_t color;
        DisplayMode mode;

        // Gathered right before drawing (in draw) TODO: CLEANUP
        uint8_t delay;

        // Gathered in the middle of a 8 pixel chunk (in drawCanvas)
        uint8_t D011;
        uint8_t D016;
        
        // Gathered in the middle of a 8 pixel chunk (in drawCanvas via updateColorRegisters)
        uint8_t borderColor;
        uint8_t backgroundColor[4];
    } dc;
    
    //! Latched portions of the VIC state
    /*! Latches everything that needs to be recorded one cycle prior to drawing */
    void prepareForCycle(uint8_t cycle);

    //! Latched color information
    /*! This needs to be done after the first pixel has been drawn */
    //! Update drawing contents with current colors
    void updateColorRegisters();
    
    
    // -----------------------------------------------------------------------------------------------
    //                        Shift register logic (handled in drawCanvasPixel)
    // -----------------------------------------------------------------------------------------------
    
    //! Shift register
    /*! To synthesize pixels, VICII uses a 8 bit shift register which is loaded whenever the current
     x scroll offset matches the current pixel number. */
    
    struct {
        //! Shift register data
        uint8_t data;

        //! Multi-color synchronization flipflop
        /*! Whenever the shift register is loaded, the synchronization flipflop is also set.
         It is toggled with each pixel and used to synchronize the synthesis of multi-color pixels. */
        bool mc_flop;
        
        //! Latched character info
        /*! Whenever the shift register is loaded, the current character value (which was once read during
         a gAccess) is latched. This value is used until the shift register loads again. */
        uint8_t latchedCharacter;
        
        //! Latched color info
        /*! Whenever the shift register is loaded, the current color value (which was once read during
         a gAccess) is latched. This value is used until the shift register loads again. */
        uint8_t latchedColor;
        
        //! Color bits
        /*! Every second pixel (as synchronized with mc_flop), the  multi-color bits are remembered. */
        uint8_t colorbits;

    } sr;
    
    
    // -----------------------------------------------------------------------------------------------
    //                          High level drawing (canvas, sprites, border)
    // -----------------------------------------------------------------------------------------------

public:
  
    //! Synthesize 8 pixels according the the current drawing context.
    /*! This is the main entry point to all drawing routines.
        To get the correct output, preparePixelEngineForCycle() must be called one cycle before. */
    void draw();

private:
    
    //! Draws 8 canvas pixels
    /*! Invoked inside draw() */
    void drawCanvas();
    
    //! Draws a single canvas pixel
    /*! pixel is the pixel number and must be in the range 0 to 7 */
    void drawCanvasPixel(uint16_t offset, uint8_t pixel);
    
    //! Draws 8 sprite pixels
    /*! Invoked inside draw() */
    // TODO: drawSprintes();

    //! Draws 8 border pixels
    /*! Invoked inside draw() */
    void drawBorder();
    
    
    // -----------------------------------------------------------------------------------------------
    //                         Mid level drawing (semantic pixel rendering)
    // -----------------------------------------------------------------------------------------------

private:
    
    //! This is where loadColors() stores all retrieved colors
    /*! [0] : color for '0' pixels in single color mode or '00' pixels in multicolor mode
        [1] : color for '1' pixels in single color mode or '01' pixels in multicolor mode
        [2] : color for '10' pixels in multicolor mode
        [3] : color for '11' pixels in multicolor mode */
    int col_rgba[4];
    
    //! loadColors() also determines if we are in single-color or multi-color mode
    bool multicol;

public:
    
    // Determine pixel colors accordig to the provided display mode
    void loadColors(DisplayMode mode, uint8_t characterSpace, uint8_t colorSpace);
    
    //! Draw single canvas pixel in single-color mode
    /*! 1s are drawn with setForegroundPixel, 0s are drawn with setBackgroundPixel.
     Uses the drawing colors that are setup by loadColors(). */
    void setSingleColorPixel(unsigned offset, uint8_t bit);
    
    //! Draw single canvas pixel in multi-color mode
    /*! The left of the two color bits determines whether setForegroundPixel or setBackgroundPixel is used.
     Uses the drawing colors that are setup by loadColors(). */
    void setMultiColorPixel(unsigned offset, uint8_t two_bits);
    
    //! Draw a single foreground pixel
    /*! The function may trigger an interrupt, if a sprite/sprite or sprite/background collision is detected. */
    void setSpritePixel(unsigned offset, int color, int nr);

    
    // -----------------------------------------------------------------------------------------------
    //                        Low level drawing (pixel buffer access)
    // -----------------------------------------------------------------------------------------------
    
public:

    //! Draw a single frame pixel
    void setFramePixel(unsigned offset, int rgba);
    
    //! Draw seven frame pixels in a row
    inline void setSevenFramePixels(unsigned offset, int rgba) {
        for (unsigned i = 0; i < 7; i++) setFramePixel(offset++, rgba); }
    
    //! Draw eight frame pixels in a row
    inline void setEightFramePixels(unsigned offset, int rgba) {
        for (unsigned i = 0; i < 8; i++) setFramePixel(offset++, rgba); }
    
    //! Draw a single foreground pixel
    void setForegroundPixel(unsigned offset, int rgba);
    
    //! Draw a single background pixel
    void setBackgroundPixel(unsigned offset, int rgba);

    //! Draw eight background pixels in a row
    inline void setEightBackgroundPixels(unsigned offset, int rgba) {
        for (unsigned i = 0; i < 8; i++) setBackgroundPixel(offset++, rgba); }

    //! Draw a single sprite pixel
    void setSpritePixel(unsigned offset, int rgba, int depth, int source);

    //! Draw a colored line into the screen buffer
    /*! This method is utilized for debugging purposes, only. */
    void markLine(uint8_t color, unsigned start = 0, unsigned end = 512);
};

    
#endif