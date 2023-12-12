#include "MicroBit.h"
#include <stdint.h>
#include <stdbool.h>


MicroBit uBit;

/*
        MICROBIT

        LCD 	MicroBit        VR S    MicroBit
        ----------------        ----------------
        VCC	    3V              +       3V
        GND	    GND             -       GND
        SCL	    P13             I       P0
        SDA	    P15
        RS/DC	P1
        RES	    3V
        CS	    P16
        BL      -

*/

/*
-----------------------------------------------------------------------------------
                                     SCREEN API

                                    LCD CONSTANTS
-----------------------------------------------------------------------------------
*/

// Size of screen expanded as there were inactive pixels when using manufacturer size (OG: 128x160)
#define SCREEN_HEIGHT 162
#define SCREEN_WIDTH 130

#define SCREEN_BACKGROUND 0x0000

/**
*       Screen Connection SPI
*/
NRF52SPI screen_spi(uBit.io.P15, uBit.io.P14, uBit.io.P13);


/**
*       ST7735 Command Set
*/
enum ST7735_Commands {
    NOP     = 0x00, SWRESET = 0x01, RDDID   = 0x04, RDDST   = 0x09, SLPIN   = 0x10,
    SLPOUT  = 0x11, PTLON   = 0x12, NORON   = 0x13, INVOFF  = 0x20, INVON   = 0x21,
    DISPOFF = 0x28, DISPON  = 0x29, CASET   = 0x2A, RASET   = 0x2B, RAMWR   = 0x2C,
    RAMRD   = 0x2E, PTLAR   = 0x30, COLMOD  = 0x3A, MADCTL  = 0x36, FRMCTR1 = 0xB1,
    FRMCTR2 = 0xB2, FRMCTR3 = 0xB3, INVCTR  = 0xB4, DISSET5 = 0xB6, PWCTR1  = 0xC0,
    PWCTR2  = 0xC1, PWCTR3  = 0xC2, PWCTR4  = 0xC3, PWCTR5  = 0xC4, VMCTR1  = 0xC5,
    RDID1   = 0xDA, RDID2   = 0xDB, RDID3   = 0xDC, RDID4   = 0xDD, PWCTR6  = 0xFC,
    GMCTRP1 = 0xE0, GMCTRN1 = 0xE1
};

/**
*       Predefined Colours
*/
enum Display_Colours {
    BLACK   = 0x0000,   RED     = 0x001F,   BLUE    = 0xF800,   GREEN   = 0x07E0,
    CYAN    = 0xFFE0,   MAGENTA = 0xF81F,   YELLOW  = 0x07FF,   WHITE   = 0xFFFF,
    ORANGE  = 0x051F,   PURPLE  = 0xF81B
};

/*
-----------------------------------------------------------------------------------
                                    SPI API
-----------------------------------------------------------------------------------
*/

/**
* Send data through SPI to the screen
* @param paramCommand - Command to be sent
* @param paramBuffer - Data to be sent
*/
void screen_send(uint8_t paramCommand, ManagedBuffer paramBuffer) {

    uBit.io.P1.setDigitalValue(0); // set TFT to command-recieve mode
    uBit.io.P16.setDigitalValue(0); // select TFT controller

    screen_spi.write(paramCommand);
    uBit.serial.printf("%x\r\n", paramCommand);

    uBit.io.P1.setDigitalValue(1); // set TFT back to data recieve mode

    for(int i = 0; i < paramBuffer.length(); i++) {
        screen_spi.write(paramBuffer[i]);
    }

    uBit.io.P16.setDigitalValue(1); // deselect TFT controller
}

/**
* Enter the sending data mode
*/
void screen_enterDataMode() {
    uBit.io.P1.setDigitalValue(0); // Activate command mode
    uBit.io.P16.setDigitalValue(0); // Select TFT as SPI Target

    screen_spi.write(RAMWR); // Transfer command

    uBit.io.P1.setDigitalValue(11); // Activate data mode
}

/**
* Exit the sending data mode
*/
void screen_exitDataMode() {
    uBit.io.P16.setDigitalValue(1); // de-elect the TFT as SPI target
    uBit.io.P1.setDigitalValue(0); // command
}

/**
* Set the current window to edit on the screen
* @param paramX0 - X position of top left corner of rectangle on screen
* @param paramY0 - Y position of top left corner of rectangle on screen
* @param paramX1 - X position of bottom right corner of rectangle on screen
* @param paramY1 - Y position of bottom right corner of rectangle on screen
*/
void screen_setWindow(uint8_t paramX0, uint8_t paramY0, uint8_t paramX1, uint8_t paramY1) {
    ManagedBuffer bufX(4);
    bufX[0] = 0; bufX[1] = paramX0; bufX[2] = 0; bufX[3] = paramX1;

    ManagedBuffer bufY(4);
    bufY[0] = 0; bufY[1] = paramY0; bufY[2] = 0; bufY[3] = paramY1;

    screen_send(CASET, bufX);
    screen_send(RASET, bufY);
}

/*
-----------------------------------------------------------------------------------
                                DRAWING METHODS
-----------------------------------------------------------------------------------
*/

/**
* Draw a rectangle on the screen
* @param paramX0 - X position of top left corner of rectangle on screen
* @param paramY0 - Y position of top left corner of rectangle on screen
* @param paramWidth - Width of the rectangle
* @param paramHeight - Height of the rectangle
* @param paramColour - Colour of the rectangle
*/
void screen_drawRectangle(uint8_t paramX0, uint8_t paramY0, uint8_t paramWidth, uint8_t paramHeight, uint16_t paramColour) {

    // verify the rectangle is within the bounds of the screen
    if(paramX0 + paramWidth > SCREEN_WIDTH || paramY0 + paramHeight > SCREEN_HEIGHT) return;
    if(paramX0 + paramWidth < 0 || paramY0 + paramHeight < 0) return;

    // split the colour into two bytes
    uint8_t colourHigh = paramColour >> 8;
    uint8_t colourLow = paramColour & 0xFF;

    // set the current editing window on the screen
    screen_setWindow(paramX0, paramY0, paramX0 + paramWidth - 1, paramY0 + paramHeight - 1);

    // write the data for each pixel to the screen
    screen_enterDataMode();

    for(uint8_t y = 0; y < paramHeight; y++ ) {
        for(uint8_t x = 0; x < paramWidth; x++) {
            screen_spi.write(colourHigh);
            screen_spi.write(colourLow);
        }
    }

    screen_exitDataMode();
}

/**
* Set the entire screen a solid colour
* @param paramColour - Colour to set the screen
*/
void screen_setScreenColour(uint16_t paramColour) {
    screen_drawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, paramColour);
}

/*
-----------------------------------------------------------------------------------
                                SCREEN INIT
-----------------------------------------------------------------------------------
*/

void screen_initialise() {
    screen_spi.frequency(4000000);
    screen_spi.setMode(0);

    // Software reset
    ManagedBuffer buf1(1); buf1[0] = 1; screen_send(SWRESET, buf1);

    // Exit Sleep mode
    ManagedBuffer buf2(1); buf2[0] = 1; screen_send(SLPOUT, buf2);

    // Frame rate control - normal mode
    ManagedBuffer buf3(3); buf3[0] = 0x01; buf3[1] = 0x2C; buf3[2] = 0x2D; screen_send(FRMCTR1, buf3);

    // Frame rate control - idle mode
    ManagedBuffer buf4(6); buf4[0] = 0x01; buf4[1] = 0x2C; buf4[2] = 0x2D; buf4[3] = 0x01; buf4[4] = 0x2C; buf4[5] = 0x2D; screen_send(FRMCTR2, buf4);

    // Display inversion control
    ManagedBuffer buf5(1); buf5[0] = 0x07; screen_send(INVCTR, buf5);

    // Display power control
    ManagedBuffer buf6(3); buf6[0] = 0xA2; buf6[1] = 0x02; buf6[2] = 0x84; screen_send(PWCTR1, buf6);
    ManagedBuffer buf7(2); buf7[0] = 0x8A; buf7[1] = 0x2A; screen_send(PWCTR2, buf7);
    ManagedBuffer buf8(2); buf8[0] = 0x0A; buf8[1] = 0x00; screen_send(PWCTR3, buf8);
    ManagedBuffer buf9(2); buf9[0] = 0x8A; buf9[1] = 0x2A; screen_send(PWCTR4, buf9);
    ManagedBuffer buf10(2); buf10[0] = 0x8A; buf10[1] = 0xEE; screen_send(PWCTR5, buf10);
    ManagedBuffer buf11(1); buf11[0] = 0x0E; screen_send(VMCTR1, buf11);

    // Disable inversion
    ManagedBuffer buf12(0); screen_send(INVOFF, buf12);

    // Memory access control
    ManagedBuffer buf13(1); buf13[0] = 0xC8; screen_send(MADCTL, buf13);

    // Set 16-bit color mode
    ManagedBuffer buf14(1); buf14[0] = 0x05; screen_send(COLMOD, buf14);

    // Column address set
    ManagedBuffer buf15(4); buf15[0] = 0x00; buf15[1] = 0x00; buf15[2] = 0x00; buf15[3] = 0x7F; screen_send(CASET, buf15);

    // Row address set
    ManagedBuffer buf16(4); buf16[0] = 0x00; buf16[1] = 0x00; buf16[2] = 0x00; buf16[3] = 0x9F; screen_send(RASET, buf16);

    // Set Gamma positive correction
    ManagedBuffer buf17(16);
    buf17[0] = 0x02; buf17[1] = 0x1C; buf17[2] = 0x07; buf17[3] = 0x12; buf17[4] = 0x37; buf17[5] = 0x32; buf17[6] = 0x29; buf17[7] = 0x2D;
    buf17[8] = 0x29; buf17[9] = 0x25; buf17[10] = 0x2B; buf17[11] = 0x39; buf17[12] = 0x00; buf17[13] = 0x01; buf17[14] = 0x03; buf17[15] = 0x10;
    screen_send(GMCTRP1, buf17);

    // Set Gamma negative correction
    ManagedBuffer buf18(16);
    buf18[0] = 0x03; buf18[1] = 0x1D; buf18[2] = 0x07; buf18[3] = 0x06; buf18[4] = 0x2E; buf18[5] = 0x2C; buf18[6] = 0x29; buf18[7] = 0x2D;
    buf18[8] = 0x2E; buf18[9] = 0x2E; buf18[10] = 0x37; buf18[11] = 0x3F; buf18[12] = 0x00; buf18[13] = 0x00; buf18[14] = 0x02; buf18[15] = 0x10;
    screen_send(GMCTRN1, buf18);

    // Set normal mode
    ManagedBuffer buf19(0); screen_send(NORON, buf19);

    // Turn display on
    ManagedBuffer buf20(0); screen_send(DISPON, buf20);

    screen_setScreenColour(SCREEN_BACKGROUND);
}

/*
-----------------------------------------------------------------------------------
                                    VRSensor API
-----------------------------------------------------------------------------------
*/

/**
* Get the analog value from the rotation sensor
* Min = 0 | Max = 1024
*/
uint16_t readAnalogValue() {
    return uBit.io.P0.getAnalogValue();
}

/*
-----------------------------------------------------------------------------------
                                    Game Play

                                     CONSANTS
-----------------------------------------------------------------------------------
*/

enum GameState {
    FINISHED, IN_GAME, START
};

struct Game {
    GameState gameState;
}; typedef struct Game Game;

/*
-----------------------------------------------------------------------------------
                                    Sprites
-----------------------------------------------------------------------------------
*/

#define SPRITE_SLIDER_WIDTH 24
#define SPRITE_SLIDER_HEIGHT 5

#define SPRITE_SLIDER_COLOUR 0xFFFF
#define SCREEN_BACKGROUND 0x0000

struct SliderSprite {
    uint8_t position_x;
    uint8_t position_y;

    uint8_t last_position_x;
    uint8_t last_position_y;

    bool drawn;
};
typedef struct SliderSprite SliderSprite;

// Assuming screen_drawRectangle is defined elsewhere
void screen_drawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color);

void redrawSlider(SliderSprite *paramSliderSprite) {
    if (paramSliderSprite->drawn) {
        if (paramSliderSprite->position_x == paramSliderSprite->last_position_x &&
            paramSliderSprite->position_y == paramSliderSprite->last_position_y) {
            return;
        }
    }

    if (paramSliderSprite->drawn) {
        screen_drawRectangle(paramSliderSprite->last_position_x, paramSliderSprite->last_position_y, SPRITE_SLIDER_WIDTH, SPRITE_SLIDER_HEIGHT, SCREEN_BACKGROUND);
    }

    screen_drawRectangle(paramSliderSprite->position_x, paramSliderSprite->position_y, SPRITE_SLIDER_WIDTH, SPRITE_SLIDER_HEIGHT, SPRITE_SLIDER_COLOUR);

    paramSliderSprite->drawn = true;
}

void moveSlider(SliderSprite *paramSliderSprite, uint8_t paramX, uint8_t paramY) {
    paramSliderSprite->last_position_x = paramSliderSprite->position_x;
    paramSliderSprite->last_position_y = paramSliderSprite->position_y;

    paramSliderSprite->position_x = paramX;
    paramSliderSprite->position_y = paramY;

    redrawSlider(paramSliderSprite);
}

SliderSprite *createSliderSprite(uint8_t paramX, uint8_t paramY) {
    SliderSprite *sliderSprite = (SliderSprite *)malloc(sizeof(SliderSprite));
    sliderSprite->position_x = paramX;
    sliderSprite->position_y = paramY;

    sliderSprite->drawn = false; // Initial state

    moveSlider(sliderSprite, paramX, paramY); // Initialize position and draw

    return sliderSprite;
}


/*
-----------------------------------------------------------------------------------
                                    Game API
-----------------------------------------------------------------------------------
*/

/**
* Map the value of the analog input to a position on the screen
*/
uint8_t mapValue() {
    uint16_t value = readAnalogValue();

    // Mid point = 512
    // Cut off ends and set range to 250 -> 774
    // Therefore range = 1024 - 500 = 524
    if(value > 744) {
        value = 774;
    } else if(value < 250) {
        value = 250;
    }
    uint8_t width_area = SCREEN_WIDTH - SPRITE_SLIDER_WIDTH;

    // Interpolation 524

    uint8_t interpolatedValue = round((((float) value - 250) / 524) * (float) width_area);

    return interpolatedValue;
}

/*
        MAIN
*/

void microbit_initialise() {
    uBit.init();
    screen_initialise();
}

int main() {
    microbit_initialise();

    SliderSprite *sprite = createSliderSprite(mapValue(), 20);

    while (true) {
        moveSlider(sprite, mapValue(), 20);
        uBit.sleep(50); // Sleep for 50 ms to update 20 times per
    }


    release_fiber();
}
