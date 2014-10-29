#include "SPI.h"
#include "Adafruit_WS2801.h"
#include "Timer.h"

/*****************************************************************************
Example sketch for driving Adafruit WS2801 pixels!


  Designed specifically to work with the Adafruit RGB Pixels!
  12mm Bullet shape ----> https://www.adafruit.com/products/322
  12mm Flat shape   ----> https://www.adafruit.com/products/738
  36mm Square shape ----> https://www.adafruit.com/products/683

  These pixels use SPI to transmit the color data, and have built in
  high speed PWM drivers for 24 bit color per pixel
  2 pins are required to interface

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by David Kavanagh (dkavanagh@gmail.com).  
  BSD license, all text above must be included in any redistribution

*****************************************************************************/

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!
uint8_t dataPin  = 13;    // Yellow wire on Adafruit Pixels
uint8_t clockPin = 12;    // Green wire on Adafruit Pixels

Timer t;

// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply

// Set the first variable to the NUMBER of pixels in a row and
// the second value to number of pixels in a column.
Adafruit_WS2801 strip = Adafruit_WS2801((uint16_t)8, (uint16_t)8, dataPin, clockPin);

#define M_WIDTH 8
#define M_HEIGHT 8

typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} ColorRGB;

//a color with 3 components: h, s and v
typedef struct 
{
  unsigned char h;
  unsigned char s;
  unsigned char v;
} ColorHSV;

boolean fadeColor = false;

//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[M_WIDTH][M_HEIGHT]={
    {32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 },
    {64 , 32 , 16 , 0  , 0  , 16 , 32 , 64 },
    {96 , 64 , 32 , 16 , 16 , 32 , 64 , 96 },
    {128, 96 , 60 , 32 , 32 , 60 , 96 , 128},
    {192, 160, 128, 96 , 96 , 128, 160, 192},
    {192, 160, 128, 110, 110, 128, 160, 192},
    {192, 192, 160, 128, 128, 160, 192, 192},
    {255, 192, 160, 150, 150, 160, 192, 255}
};

/*
const unsigned char valueMask[M_WIDTH][M_HEIGHT]={
    {32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 },
    {64 , 32 , 0  , 0  , 0  , 0  , 32 , 64 },
    {96 , 64 , 0  , 0  , 0  , 0  , 64 , 96 },
    {128, 94 , 32 , 0  , 0  , 32 , 94 , 128},
    {160, 128, 64 , 32 , 32 , 64 , 128, 160},
    {192, 160, 96 , 64 , 64 , 96 , 160, 192},
    {192, 192, 128, 96 , 96 , 128, 192, 192},
    {255, 192, 160, 128, 128, 160, 192, 255}
};
*/

//these are the hues for the fire, 
//should be between 0 (red) to about 13 (yellow)
unsigned char hueMask[M_WIDTH][M_HEIGHT]={
    {  0,   4,  10,  20,  25,  20,  10,   0},
    {  0,   3,   5,  10,  20,  10,   3,   0},
    {  0,   3,   5,   6,   8,   6,   3,   0},
    {  0,   2,   4,   5,   6,   5,   2,   0},
    {  0,   1,   4,   4,   4,   4,   1,   0},
    {  0,   0,   1,   3,   3,   1,   0,   0},
    {  0,   0,   0,   1,   1,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0}
};

unsigned char matrix[M_WIDTH][M_HEIGHT];
unsigned char line[M_WIDTH];
int pcnt = 0;
int colorFadeCounter = 60 * 1;
int startHue = 250;
int timerOn = 60 * 5;
int timerOff = 60 * 19;
int timerCount = 0;
boolean modeOn = true;
boolean updateScreen = true;
boolean fade = true;
boolean fadeIn = true;
int fadeValue = 255;

/**
 * Randomly generate the next line (matrix row)
 */
void generateLine(){
  for(unsigned char x=0;x<M_HEIGHT;x++) {
      line[x] = random(64, 255);
  }
}

/**
 * shift all values in the matrix up one row
 */
void shiftUp(){
  ColorRGB colorRGB;
  
  for (unsigned char y=M_WIDTH-1;y>0;y--) {
    for(unsigned char x=0;x<M_HEIGHT;x++) {
        matrix[x][y] = matrix[x][y-1];
    }
  }
  
  for(unsigned char x=0;x<M_HEIGHT;x++) {
      matrix[x][0] = line[x];
  }
}

/**
 * draw a frame, interpolating between 2 "key frames"
 * @param pcnt percentage of interpolation
 */
 
void incHueMask(int incVal) {
  int x;
  int Y;
  for (int x = 0; x < 8; x = x + 1) {
    for (int y = 0; y < 8; y = y + 1) {
      hueMask[x][y] = (hueMask[x][y]+incVal) % 255;
    }
  }
} 

int addFadeValue(int value) {
  value = value + fadeValue;
  if (value > 255) { value = 255; } 
  return value;
}
 
void drawFrame(int pcnt){
  ColorRGB colorRGB;
  ColorHSV colorHSV;
  int nextv;

  
  //each row interpolates with the one before it
  for (unsigned char y=M_WIDTH-1;y>0;y--) {
    for (unsigned char x=0;x<M_HEIGHT;x++) {
        colorHSV.h = hueMask[y][x];
        colorHSV.s = 255;
        nextv = 
            (((100.0-pcnt)*matrix[x][y] 
          + pcnt*matrix[x][y-1])/100.0) 
          - addFadeValue(valueMask[y][x]);
        colorHSV.v = (unsigned char)max(0, nextv);
        
        HSVtoRGB(&colorRGB, &colorHSV);
        strip.setPixelColor(x, y, colorRGB.r, colorRGB.g, colorRGB.b);
    }
  }
  
  //first row interpolates with the "next" line
  for(unsigned char x=0;x<M_HEIGHT;x++) {
      colorHSV.h = hueMask[0][x];
      colorHSV.s = 255;
      colorHSV.v = (unsigned char)max(0, (((100.0-pcnt)*matrix[x][0] + pcnt*line[x])/100.0) - addFadeValue(valueMask[0][x]));
      
      HSVtoRGB(&colorRGB, &colorHSV);
      strip.setPixelColor(x, 0, colorRGB.r, colorRGB.g, colorRGB.b);
  }
}

void onMinuteTick()
{
  timerCount++;
  if (modeOn) {
    if (timerCount >= timerOn) {
      timerCount = 0;
      // Clear Screen
      fadeValue = 0;
      fadeIn = false;
      fade = true;
      modeOn = false;
    }
  } else {
    if (timerCount >= timerOff) {
      timerCount = 0;
      // Open Screen
      fadeValue = 255;
      fadeIn = true;
      fade = true;
      modeOn = true;
      updateScreen = true;
    }
  }
}

void setup() {
  
  incHueMask(startHue);
    
  strip.begin();

  // Update LED contents, to start they are all 'off'
  strip.show();
  
  randomSeed(analogRead(0));
  generateLine();
  
  //init all pixels to zero
  for (unsigned char y=0;y<M_WIDTH;y++) {
    for(unsigned char x=0;x<M_HEIGHT;x++) {
        matrix[x][y] = 0;
    }
  }
  
  int tickEvent = t.every(60000, onMinuteTick);
  int tickEvent2 = t.every(10, doloop);
  
}


void loop() {
  t.update();

}

void doloop() {
  if (fade) {
    if (fadeIn) {
      fadeValue--;
      if (fadeValue < 0) {
        fade = false;
      }
    } else {
      fadeValue++;
      if (fadeValue > 255) {
        fade = false;
        updateScreen = false;
      }
    }
  }
  
  if (updateScreen) {
    if (fadeColor && colorFadeCounter >= 100) {
      incHueMask(1);
      colorFadeCounter=0;
    }
    if (pcnt >= 100){
          shiftUp();
          generateLine();
          pcnt = 0;
    }
    drawFrame(pcnt);
    strip.show();
    pcnt+=30;
    colorFadeCounter+=1;
  }
}


/* Helper functions */

//Converts an HSV color to RGB color
void HSVtoRGB(void *vRGB, void *vHSV) 
{
  float r, g, b, h, s, v; //this function works with floats between 0 and 1
  float f, p, q, t;
  int i;
  ColorRGB *colorRGB=(ColorRGB *)vRGB;
  ColorHSV *colorHSV=(ColorHSV *)vHSV;

  h = (float)(colorHSV->h / 256.0);
  s = (float)(colorHSV->s / 256.0);
  v = (float)(colorHSV->v / 256.0);

  //if saturation is 0, the color is a shade of grey
  if(s == 0.0) {
    b = v;
    g = b;
    r = g;
  }
  //if saturation > 0, more complex calculations are needed
  else
  {
    h *= 6.0; //to bring hue to a number between 0 and 6, better for the calculations
    i = (int)(floor(h)); //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
    f = h - i;//the fractional part of h

    p = (float)(v * (1.0 - s));
    q = (float)(v * (1.0 - (s * f)));
    t = (float)(v * (1.0 - (s * (1.0 - f))));

    switch(i)
    {
      case 0: r=v; g=t; b=p; break;
      case 1: r=q; g=v; b=p; break;
      case 2: r=p; g=v; b=t; break;
      case 3: r=p; g=q; b=v; break;
      case 4: r=t; g=p; b=v; break;
      case 5: r=v; g=p; b=q; break;
      default: r = g = b = 0; break;
    }
  }
  colorRGB->r = (int)(r * 255.0);
  colorRGB->g = (int)(g * 255.0);
  colorRGB->b = (int)(b * 255.0);
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
