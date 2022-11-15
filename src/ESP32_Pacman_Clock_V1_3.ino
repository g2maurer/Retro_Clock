/*  Retro Pac-Man Clock
  Author: @TechKiwiGadgets Date 08/04/2017

  Licensed as Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
  You are free to:
  Share — copy and redistribute the material in any medium or format
  Adapt — remix, transform, and build upon the material
  Under the following terms:
  Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
  NonCommercial — You may not use the material for commercial purposes.
  ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original

  V67 First Instructables release
*/

#define TFT_SPI           // uncomment if using TFT display with SPI interface
#define LM386_AUDIO
//#define DS3231_RTC

//
//  If Sprite library worked correctly and we used sprites for the Ghost and Pacman
//  we wouldn't have to redraw the dots after the Ghost passed over them.
//
//#define SPRITE        // Sprite library doesn't seem to work correctly.

//
//  The following are for debugging
//    uncomment the '#define' to get debug prints.
//

//#define DEBUG_PACMAN
#ifdef DEBUG_PACMAN
#define PACMAN_PRINT(x) Serial.print(x)
#define PACMAN_PRINTLN(x) Serial.println(x)
#define PACMAN_PRINTF(x,y) Serial.printf(x,y)
#else
#define PACMAN_PRINT(x)
#define PACMAN_PRINTLN(x)
#define PACMAN_PRINTF(x,y)
#endif

//#define DEBUG_CONFIG
#ifdef DEBUG_CONFIG
#define CONFIG_PRINT(x) Serial.print(x)
#define CONFIG_PRINTLN(x) Serial.println(x)
#define CONFIG_PRINTF(x,y) Serial.printf(x,y)
#else
#define CONFIG_PRINT(x)
#define CONFIG_PRINTLN(x)
#define CONFIG_PRINTF(x,y)
#endif

#include "PM_SPIFFS.h"
#include "PM_WiFi.h"
#include "PM_TFT.h"


#include <Arduino.h>
#ifndef DS3231_RTC
#include <WiFi.h>
#include <NTPClient.h>
#endif
#include "XT_DAC_Audio.h"
#include "TFT_eSPI.h"
#include <TimeLib.h>
#include <pgmspace.h>

#ifdef DS3231_RTC
#include "PM_RTC_NTP.h"
#endif



#ifndef TFT_SPI
#include <TouchScreen.h>
#endif

#include "Free_Fonts.h"
#include "SoundData.h"

IPAddress localIP;

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

#ifndef TFT_SPI   // not TFT_SPI
#define YP 14  // must be an analog pin, use "An" notation!
#define XM 27  // must be an analog pin, use "An" notation!
#define YM 4   // can be a digital pin
#define XP 16  // can be a digital pin
#define MAXPRESSURE 799

// Enter Callibration Variables here for the touch screen
// Run callibration sketch first then write down the following coordicates and update sketch

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

boolean debounce = false;

#endif    // not TFT_SPI

//
//  ESP32 pin definitions
//

//
//  AUDIO pins
//
#define MUTE_PIN  32
#ifdef LM386_AUDIO
// using LM386 audio amplifier
#define MUTE  HIGH
#define UNMUTE  LOW
#else
// original uses HW-104 audio amplifier board
#define MUTE  LOW
#define UNMUTE  HIGH
#endif
#define AUDIO_OUT_PIN 25


unsigned long mlStart = 0;
unsigned long mlEnd = 0;


//
//  TFT Brightness Control pins
//
#ifdef TFT_SPI
// SPI TFT ONLY !!!
#define TFT_LED 26      // TFT LED control pin
#define LDR_PIN 39      // Light dependent resistor
int dutyCycle = 255;    // TFT LED dutycycle
#endif

#define LDR_CHECK_TIME_PERIOD 3000    // three seconds
unsigned long timePeriodStart = 0;

//
//  SPIFFS files
//   File names (fn.....)
//
const char* fnTftCal = "/tftcal.bin";
const char* fnWifiConfig = "/wifi.bin";
const char* fnClockConfig = "/config.bin";
const char* fnTimeZoneConfig = "/timezone.bin";
const char* fnBlueGhost = "/blueGhost.bin";
const char* fnRedGhostLeft = "/redGhostL.bin";
const char* fnRedGhostRight = "/redGhostR.bin";
const char* fnRedGhostUp = "/redGhostU.bin";
const char* fnRedGhostDown = "/redGhostD.bin";
const char* fnFruit = "/fruit.bin";
const char* fnPacClosed = "/pacClosed.bin";
const char* fnPacHalf = "/pacHalf.bin";
const char* fnPacOpen = "/pacOpen.bin";
const char* fnMsPacClosed = "/mPacClosed.bin";
const char* fnMsPacHalf = "/mPacHalf.bin";
const char* fnMsPacOpen = "/mPacOpen.bin";


//
//  NTP - Network Time Protocol
//
//const char* TZ_INFO = "NZST-12NZDT-13,M9.4.0/02:00:00,M4.1.0/03:00:00";
//const char* TZ_INFO = "PST8PDT,M3.2.0,M11.1.0";   //"PST8PDT";
//const char* ntpServer1 = "uk.pool.ntp.org";

const char* ntpServer1 = "time.nist.gov";
const char* ntpServer2 = "ptbtime1.ptb.de";
const char* ntpServer3 = "pool.ntp.org";

//
//  Clock Variables
//
byte clockhour; // Holds current hour value
byte clockminute; // Holds current minute value


//
// Alarm Variables
//
boolean soundalarm = false; // Flag to indicate the alarm needs to be initiated
int actr = 3000; // When alarm sounds this is a counter used to reset sound card until screen touched
int act = 0;

//
//  Pac-man Clock Configuration data, saved in SPIFFS, restored on boot/reboot
//
struct CLOCK_CONFIG_STRUCT {
  int pad1;
  boolean alarmStatus;
  int alarmHour;  // hour of alarm setting
  int alarmMinute; // Minute of alarm setting
  int speedSetting;
  int alarmVolume;
  boolean mspacman;
  boolean clock24;
  boolean celsiusFlag;
  int pad3;
} clockConfig;

//
//Dot Array - There are 72 Dots with 4 of them that will turn Ghost Blue!
//
typedef struct DOT_STRUCT {
  bool state;    // true or false, if false it has been gobbled by Pac-Man
  short xPos;
  short yPos;
  byte dotSize;     // small = 2, large = 7
} DOT;


//
//    Dot location Array
//        [state, x, y, size]
//
DOT dots[73] = {
  {0, 0, 0, 0},       // NOT USED
  {true, 19, 19, 2},  // dot 1, Row 1
  {true, 42, 19, 2},
  {true, 65, 19, 2},
  {true, 88, 19, 2},
  {true, 112, 19, 2},
  {true, 136, 19, 2},
  {true, 183, 19, 2},
  {true, 206, 19, 2},
  {true, 229, 19, 2},
  {true, 252, 19, 2}, // dot 10
  {true, 275, 19, 2},
  {true, 298, 19, 2},
  {true, 19, 40, 7},  // dot 13, Row 2  BIG DOT
  {true, 77, 40, 2},
  {true, 136, 40, 2},
  {true, 183, 40, 2},
  {true, 241, 40, 2},
  {true, 298, 40, 7}, // BIG DOT
  {true, 19, 60, 2},  // dot 19, Row 3
  {true, 42, 60, 2},  // dot 20
  {true, 65, 60, 2},
  {true, 88, 60, 2},
  {true, 112, 60, 2},
  {true, 136, 60, 2},
  {true, 160, 60, 2},
  {true, 183, 60, 2},
  {true, 206, 60, 2},
  {true, 229, 60, 2},
  {true, 252, 60, 2},
  {true, 275, 60, 2}, // dot 30
  {true, 298, 60, 2},
  {true, 42, 80, 2},  // dot 32, Row 4
  {true, 275, 80, 2},
  {true, 42, 100, 2}, // dot 34, Row 5
  {true, 275, 100, 2},
  {true, 42, 120, 2}, // dot 36, Row 6
  {true, 275, 120, 2},
  {true, 42, 140, 2}, // dot 38, Row 7
  {true, 275, 140, 2},
  {true, 42, 160, 2}, // dot 40, Row 8
  {true, 275, 160, 2},
  {true, 19, 181, 2}, // dot 42, Row 9
  {true, 42, 181, 2},
  {true, 65, 181, 2},
  {true, 88, 181, 2},
  {true, 112, 181, 2},
  {true, 136, 181, 2},
  {true, 160, 181, 2},
  {true, 183, 181, 2},
  {true, 206, 181, 2},  // dot 50
  {true, 229, 181, 2},
  {true, 252, 181, 2},
  {true, 275, 181, 2},
  {true, 298, 181, 2},
  {true, 19, 201, 7}, // dot 55, Row 10  BIG DOT
  {true, 77, 201, 2},
  {true, 136, 201, 2},
  {true, 183, 201, 2},
  {true, 241, 201, 2},
  {true, 298, 201, 7},  // dot 60  BIG DOT
  {true, 19, 221, 2}, // dot 61, Row 11
  {true, 42, 221, 2},
  {true, 65, 221, 2},
  {true, 88, 221, 2},
  {true, 112, 221, 2},
  {true, 136, 221, 2},
  {true, 183, 221, 2},
  {true, 206, 221, 2},
  {true, 229, 221, 2},
  {true, 252, 221, 2},  // dot 70
  {true, 275, 221, 2},
  {true, 298, 221, 2}
};

//
//  Define the number of Dots in each Row
//
#define DOTS_IN_ROW_1   12
#define DOTS_IN_ROW_2   6
#define DOTS_IN_ROW_3   13
#define DOTS_IN_ROW_4   13
#define DOTS_IN_ROW_5   6
#define DOTS_IN_ROW_6   12

//
//  x axis position for Dots (by Row)
//
short row1n6dots[DOTS_IN_ROW_1] = {4, 28, 52, 74, 98, 120, 168, 192, 216, 238, 262, 284};       // rows 1 & 6
short row2n5dots[DOTS_IN_ROW_2] = {4, 62, 120, 168, 228, 284};                                  // rows 2 & 5
short row3n4dots[DOTS_IN_ROW_3] = {4, 28, 52, 74, 98, 120, 146, 168, 192, 216, 238, 262, 284};  // rows 3 & 4

//
//  Adjacent Dot Lookup Table
//
byte adjacentDots[73][4] = {
  {0, 0, 0, 0},
  {2, 13, 0, 0},    //dot 1, Row 1
  {1, 3, 0, 0},   //dot 2
  {2, 4, 14, 0},    //dot 3
  {3, 5, 14, 0},    //dot 4
  {4, 6, 0, 0},   //dot 5
  {5, 15, 0, 0},    //dot 6
  {8, 16, 0, 0},    //dot 7
  {7, 9, 0, 0},   //dot 8
  {8, 10, 17, 0},   //dot 9
  {9, 11, 17, 0},   //dot 10
  {10, 12, 18, 0},  //dot 11
  {11, 18, 0, 0},   //dot 12
  {1, 19, 0, 0},    //dot 13, Row 2  BIG DOT
  {3, 4, 21, 22},   //dot 14
  {6, 24, 0, 0},    //dot 15
  {7, 26, 0, 0},    //dot 16
  {9, 10, 28, 29},   //dot 17
  {12, 31, 0, 0},   //dot 18  BIG DOT
  {13, 20, 0, 0},   //dot 19, Row 3
  {13, 19, 21, 32}, //dot 20
  {14, 20, 22, 0},  //dot 21
  {14, 21, 23, 0},  //dot 22
  {22, 24, 0, 0},   //dot 23
  {15, 23, 25, 0},  //dot 24
  {24, 26, 0, 0},   //dot 25
  {16, 25, 27, 0},  //dot 26
  {28, 26, 0, 0},   //dot 27
  {17, 27, 29, 0},  //dot 28
  {17, 28, 30, 0},  //dot 29
  {18, 29, 31, 33}, //dot 30
  {18, 30, 0, 0},   //dot 31, end of Row 3
  {20, 34, 0, 0},   //dot 32, column 2
  {30, 35, 0, 0},   //dot 33, column 7
  {32, 36, 0, 0},   //dot 34
  {33, 37, 0, 0},   //dot 35
  {34, 38, 0, 0},   //dot 36
  {35, 39, 0, 0},   //dot 37
  {36, 40, 0, 0},   //dot 38
  {37, 41, 0, 0},   //dot 39
  {38, 43, 0, 0},   //dot 40
  {39, 53, 0, 0},   //dot 41
  {43, 55, 0, 0},   //dot 42, Row 4
  {40, 42, 44, 0},  //dot 43
  {43, 45, 56, 0},  //dot 44
  {44, 46, 56, 0},  //dot 45
  {45, 47, 0, 0},   //dot 46
  {46, 48, 57, 0},  //dot 47
  {47, 49, 0, 0},   //dot 48
  {48, 50, 58, 0},  //dot 49
  {49, 51, 0, 0},   //dot 50
  {50, 52, 59, 0},  //dot 51
  {51, 53, 59, 0},  //dot 52
  {41, 52, 54, 60}, //dot 53
  {53, 60, 0, 0},   //dot 54
  {42, 61, 0, 0},   //dot 55, Row 5  BIG DOT
  {44, 45, 63, 63}, //dot 56
  {47, 66, 0, 0},   //dot 57
  {49, 67, 0, 0},   //dot 58
  {51, 52, 69, 70}, //dot 59
  {54, 72, 0, 0},   //dot 60  BIG DOT
  {55, 62, 0, 0},   //dot 61, Row 6
  {61, 63, 0, 0},   //dot 62
  {56, 62, 64, 0},  //dot 63
  {56, 63, 65, 0},  //dot 64
  {64, 66, 0, 0},   //dot 65
  {57, 65, 0, 0},   //dot 66
  {58, 68, 0, 0},   //dot 67
  {67, 69, 0, 0},   //dot 68
  {59, 68, 70, 0},  //dot 69
  {59, 69, 71, 0},  //dot 70
  {60, 70, 72, 0},  //dot 71
  {60, 71, 0, 0}    //dot 72
};

//
//  ICON definitions
//
#define ICON_SIZE 1568
#define DIR_RIGHT 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_UP 3
#define CLOSED_MOUTH 0
#define HALF_OPEN_MOUTH 1
#define OPEN_MOUTH 2

//
//  array of pointers to Pac-man / Ms Pac-man ICONs
//
unsigned short *ptrPacman[3][4];
//
//  array of pointers to Red Ghost ICONs
//
unsigned short *ptrGhost[4];

//
//  ICONs - loaded from SPIFFS (files)
//
unsigned short pacmanClosedRight[0x310];      // Pacman mouth closed facing right
unsigned short pacmanHalfOpenRight[0x310];    // Pacman mouth half open facing right
unsigned short pacmanOpenRight[0x310];        // Pacman mouth open facing right
unsigned short pacmanClosedDown[0x310];
unsigned short pacmanHalfOpenDown[0x310];
unsigned short pacmanOpenDown[0x310];
unsigned short pacmanClosedLeft[0x310];
unsigned short pacmanHalfOpenLeft[0x310];
unsigned short pacmanOpenLeft[0x310];
unsigned short pacmanClosedUp[0x310];
unsigned short pacmanHalfOpenUp[0x310];
unsigned short pacmanOpenUp[0x310];

unsigned short redGhostEyesRight[0x310];
unsigned short redGhostEyesDown[0x310];
unsigned short redGhostEyesLeft[0x310];
unsigned short redGhostEyesUp[0x310];
unsigned short blueGhost[0x310];
unsigned short fruitIcon[0x310];

//
//  Game variables
//
// Fruit flags
boolean fruitgone = false;
boolean fruitdrawn = false;
boolean fruiteatenpacman = false;
//Pacman & Ghost kill flags
boolean pacmanlost = false;
boolean ghostlost = false;

// Scorecard
int pacmanscore = 0;
int ghostscore = 0;

//int userspeedsetting = 1; // user can set normal, fast, crazy speed for the pacman animation

int gamespeed = 18; //22; // Delay setting in mS for game default is 18
int cstep = 2; // Provides the resolution of the movement for the Ghost and Pacman character. Origially set to 2

// Animation delay to slow movement down
int dly = gamespeed; // Orignally 30

//Alarm setup variables
boolean xsetup = false; // Flag to determine if exiting setup mode

// Time Refresh counter
int rfcvalue = 900; // wait this long until check time for changes
int rfc = 1;

//
// Graphics Drawing Variables
//

//
// Pac-man coordinates (top LHS of 28x28 bitmap)
//
int xP = 4;         // Pac-man x axis
int yP = 108;       // Pac-man y axis
int P = 0;  // Pacman Graphic Flag 0 = Closed, 1 = Medium Open, 2 = Wide Open, 3 = Medium Open
int D = 0;  // Pacman direction 0 = right, 1 = down, 2 = left, 3 = up
int prevD;  // Capture legacy direction to enable adequate blanking of trail
int direct;   //  Random direction variable

//
// Ghost coordinates  (top LHS of 28x28 bitmap)
//
int xG = 288;       // Ghost x axis
int yG = 108;       // Ghost y aXIS
int GD = 2;  // Ghost direction 0 = right, 1 = down, 2 = left, 3 = up
int prevGD;  // Capture legacy direction to enable adequate blanking of trail
int gdirect;   //  Random direction variable

//
// Declare global variables for previous time, to enable refesh of only digits that have changed
// There are four digits that need to be drawn independently to ensure consistent positioning of time
//
int c1 = 20;  // Tens hour digit
int c2 = 20;  // Ones hour digit
int c3 = 20;  // Tens minute digit
int c4 = 20;  // Ones minute digit

TFT_eSPI myGLCD = TFT_eSPI();       // Invoke custom library

const GFXfont *gfxFont = NULL;

//unsigned long runTime = 0;

//
// Create an object of type XT_Wav_Class that is used by
// the dac audio class (below), passing wav data as parameter.
//     Uses GPIO 25, one of the 2 DAC pins and timer 0
//
XT_Wav_Class Pacman(PM);
XT_Wav_Class pacmangobble(gobble);     // create an object of type XT_Wav_Class that is used by
XT_DAC_Audio_Class DacAudio(AUDIO_OUT_PIN, 0);   // Create the main player class object.


#ifndef TFT_SPI     // not TFT SPI
//
// Touchscreen Callibration Coordinates (parallel interface)
//
int tvar = 150; // This number used for + and - boundaries based on callibration

int Ax = 900; int Ay = 2300; // Alarm Hour increment Button
int Bx = 400; int By = 3400; // Alarm Minute increment Button
int Dx = 1400; int Dy = 2200; // Alarm Hour decrement Button
int Ex = 1100; int Ey = 3000; // Alarm Minute decrement Button

int Ix = 600; int Iy = 2600; // Pacman Up
int Jx = 1850; int Jy = 1800; // Pacman Left
int Kx = 850; int Ky = 3600; // Pacman Right
int Hx = 1650; int Hy = 2120; // Pacman Down

int Cx = 1400; int Cy = 2800; // Exit screen
int Fx = 2000; int Fy = 1600; // Alarm Set/Off button and speed button
int Lx = 300; int Ly = 3600; // Alarm Menu button
int Gx = 1350; int Gy = 2250; // Setup Menu and Change Pacman character

//
// Touch screen coordinates (parrallel interface)
//
boolean screenPressed = false;
int xT, yT;
int userT = 4; // flag to indicate directional touch on screen
boolean setupscreen = false; // used to access the setup screen
#endif    // not TFT_SPI


//////////////////////////////////////////////////////////
//
//  Initial Configuration (read from SPIFFS)
//
void initConfig(void)
{
  //deleteFile(fnClockConfig);
  if (!existsFile(fnClockConfig))
  {
    CONFIG_PRINTLN("Creating Clock configuration file");
    clockConfig.alarmStatus = false;
    clockConfig.alarmHour = 7;
    clockConfig.alarmMinute = 30;
    clockConfig.speedSetting = 1;
    clockConfig.alarmVolume = 50;
    clockConfig.mspacman = false;
    clockConfig.clock24 = false;
    clockConfig.celsiusFlag = false;

    wrtFile(fnClockConfig, (char *)&clockConfig, sizeof(clockConfig));
  }
  else
  {
    CONFIG_PRINTLN("Reading Clock configuration file");
    if (rdFile(fnClockConfig, (char *)&clockConfig, sizeof(clockConfig)))
    {
      CONFIG_PRINTLN("\r\n=== Clock Configuration ===");
      if (clockConfig.alarmStatus)
        CONFIG_PRINTLN("\tAlarm Set");
      else
        CONFIG_PRINTLN("\tAlarm OFF");
      CONFIG_PRINT("\t");
      CONFIG_PRINT(clockConfig.alarmHour);
      CONFIG_PRINT(":");
      CONFIG_PRINTLN(clockConfig.alarmMinute);
    }
  }
}


bool WiFi_Connect_Flag = false;

/////////////////////////////////////////////////////////////
//
//        SETUP
//
void setup()
{
  pinMode(MUTE_PIN, OUTPUT); // This pin used to mute the audio when not in use via pin 5 of PAM8403
  pinMode(REPEAT_CAL_PIN, INPUT);

  // MUTE sound to speaker via pin 5 of PAM8403
  digitalWrite(MUTE_PIN, MUTE);

  if (digitalRead(REPEAT_CAL_PIN) == 0)
  {
    Serial.println("Repeat TFT Calibration");
    calibrateTftTouch = true;
  }
  // Randomseed will shuffle the random function
  randomSeed(analogRead(0));

  Serial.begin(115200);

  PACMAN_PRINTF("dot array size = %d\n\r", sizeof(dots));
  initSPIFFS();           // Initialize SPIFFS file system
  initConfig();           // Load saved configuration

  //
  //  Initialize ICON pointers
  //
  ptrGhost[DIR_RIGHT] = redGhostEyesRight;
  ptrGhost[DIR_DOWN] = redGhostEyesDown;
  ptrGhost[DIR_LEFT] = redGhostEyesLeft;
  ptrGhost[DIR_UP] = redGhostEyesUp;

  ptrPacman[CLOSED_MOUTH][DIR_UP] = pacmanClosedUp;
  ptrPacman[CLOSED_MOUTH][DIR_DOWN] = pacmanClosedDown;
  ptrPacman[CLOSED_MOUTH][DIR_LEFT] = pacmanClosedLeft;
  ptrPacman[CLOSED_MOUTH][DIR_RIGHT] = pacmanClosedRight;
  ptrPacman[OPEN_MOUTH][DIR_UP] = pacmanOpenUp;
  ptrPacman[OPEN_MOUTH][DIR_DOWN] = pacmanOpenDown;
  ptrPacman[OPEN_MOUTH][DIR_LEFT] = pacmanOpenLeft;
  ptrPacman[OPEN_MOUTH][DIR_RIGHT] = pacmanOpenRight;
  ptrPacman[HALF_OPEN_MOUTH][DIR_UP] = pacmanHalfOpenUp;
  ptrPacman[HALF_OPEN_MOUTH][DIR_DOWN] = pacmanHalfOpenDown;
  ptrPacman[HALF_OPEN_MOUTH][DIR_LEFT] = pacmanHalfOpenLeft;
  ptrPacman[HALF_OPEN_MOUTH][DIR_RIGHT] = pacmanHalfOpenRight;

  loadIcons();        // Load Pac-man ICONs
  loadGhostsIcons();

#ifdef TFT_SPI
  //
  // Setup TFT LED
  //
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_LED, 0);
  ledcWrite(0, dutyCycle);
#endif

  //
  //  Setup TFT
  //
  myGLCD.begin();
  myGLCD.setRotation(3); // Inverted to accomodate USB cable
#ifdef TFT_SPI
  touch_calibrate();
#endif
  myGLCD.fillScreen(TFT_BLACK);
  //  myGLCD.setSwapBytes(true);
  setGfxFont(NULL);

  // Initialize Dot Array
  for (int dotarray = 1; dotarray < 73; dotarray++) {
    dots[dotarray].state = true;
  }

  // Initialize WiFi connection
  myGLCD.setTextSize(3);
  myGLCD.setTextColor(TFT_GREEN, TFT_BLACK);
  myGLCD.setTextDatum(TC_DATUM);         // Center Text
#ifdef DS3231_RTC
  if (RTC_init())
  {
    myGLCD.drawString("RTC Connected", 160, 100);
    delay(1000);
  }
  else
  {
    myGLCD.drawString("RTC Failed", 160, 90);
    myGLCD.drawString("to Connect", 160, 120);
    delay(3000);
  }
#else
  if (initWiFi())
  {
    WiFi_Connect_Flag = true;
    myGLCD.drawString("WiFi Connected", 160, 100);
    delay(1000);
  }
  else
  {
    myGLCD.drawString("WiFi Failed", 160, 90);
    myGLCD.drawString("to Connect", 160, 120);
    delay(3000);
  }
#endif
  //  delay(10000);
  myGLCD.setTextDatum(TL_DATUM);         // Left Justify Text
  myGLCD.fillScreen(TFT_BLACK);          // erase screen

  // configure the Time Zone
  configTZ();

#ifndef DS3231_RTC
  if (WiFi_Connect_Flag)
  {
    //disconnect WiFi as it's no longer needed
    wifiDisconnect();
  }
#endif


  drawscreen(); // Initiate the game
  UpdateDisp(); // update value to clock

  DacAudio.DacVolume = clockConfig.alarmVolume;
  playalarmsound1(); // Play Alarmsound once on powerup

  // playalarmsound2(); // Play button confirmation sound
}


/////////////////////////////////////////////////////////
//
//      Main Loop
//
void loop()
{
  //  mlStart = millis();
  setgamespeed(); // Set game animation speed
  printscoreboard(); //Print scoreboard
  drawfruit();  // Draw fruit and allocate points
  refreshgame(); // Read the current date and time from the RTC and reset board
  triggeralarm(); //=== Check if Alarm needs to be sounded
  mainuserinput(); // Check if user input to touch screen
  displaypacman(); // Draw Pacman in position on screen
  displayghost(); // Draw Ghost in position on screen
  //  mlEnd = millis();
  //  Serial.println(mlEnd-mlStart);
  delay(dly);
#ifdef TFT_SPI
  //
  //  Check LDR every "LDR_CHECK_TIME_PERIOD" for change in light
  //
  if (millis() - timePeriodStart > LDR_CHECK_TIME_PERIOD)
  {
    setLEDdutyCycle();
    timePeriodStart = millis();
  }
#endif
}

//////////////////////////////////////////////////////////
//
//  Set TFT backlight LED duty cycle (brightness)
//
#ifdef TFT_SPI
//
//  Sets the TFT backlight LED duty cycle
//    dims LED when room is dark.
//
void setLEDdutyCycle(void)
{
  int light = analogRead(LDR_PIN);
  PACMAN_PRINTLN(light);
  dutyCycle = (4095 - light) / 15;
  if (dutyCycle > 240) dutyCycle = 255;
  if (dutyCycle < 20) dutyCycle = 20;
  PACMAN_PRINT(dutyCycle);
  PACMAN_PRINTLN(" = Backlight LED dutycycle");
  ledcWrite(0, dutyCycle);
}
#endif

///////////////////////////////////////////////////////////
//
//
//
void setgamespeed() {

  if (clockConfig.speedSetting == 1) {
    gamespeed = 18;  //22;
  } else if (clockConfig.speedSetting == 2) {
    gamespeed = 12;  //14;
  } else if (clockConfig.speedSetting == 3) {
    gamespeed = 0;
  }
  dly = gamespeed; // Reset the game speed
}

////////////////////////////////////////////////////////////
//
//
//
void printscoreboard() { //Print scoreboard

  if ((ghostscore >= 95) || (pacmanscore >= 95)) { // Reset scoreboard if over 95
    ghostscore = 0;
    pacmanscore = 0;
    for (int dotarray = 1; dotarray < 73; dotarray++) {

      dots[dotarray].state = true;
    }

    // Blank the screen across the digits before redrawing them
    //  myGLCD.setColor(0, 0, 0);
    //  myGLCD.setBackColor(0, 0, 0);

    myGLCD.fillRect(299  , 87  , 15  , 10  , TFT_BLACK); // Blankout ghost score
    myGLCD.fillRect(7  , 87  , 15  , 10  , TFT_BLACK);   // Blankout pacman score

    drawscreen(); // Redraw dots
  }

  myGLCD.setTextColor(TFT_RED, TFT_BLACK);
  myGLCD.setTextSize(1);

  // Account for position issue if over or under 10

  if (ghostscore >= 10) {
    //  myGLCD.setColor(237, 28, 36);
    //  myGLCD.setBackColor(0, 0, 0);
    myGLCD.drawNumber(ghostscore, 301, 88);
  } else {
    //  myGLCD.setColor(237, 28, 36);
    //  myGLCD.setBackColor(0, 0, 0);
    myGLCD.drawNumber(ghostscore, 307, 88); // Account for being less than 10
  }

  myGLCD.setTextColor(TFT_YELLOW, TFT_BLACK);   myGLCD.setTextSize(1);

  if (pacmanscore >= 10) {
    //  myGLCD.setColor(248, 236, 1);
    //  myGLCD.setBackColor(0, 0, 0);
    myGLCD.drawNumber(pacmanscore, 9, 88);
  } else {
    //  myGLCD.setColor(248, 236, 1);
    //  myGLCD.setBackColor(0, 0, 0);
    myGLCD.drawNumber(pacmanscore, 15, 88); // Account for being less than 10
  }
}

///////////////////////////////////////////////////////////
//
//
//
void drawfruit() { // Draw fruit and allocate points

  // Draw fruit
  if ((fruitdrawn == false) && (fruitgone == false)) { // draw fruit and set flag that fruit present so its not drawn again
    //drawicon(146, 168, fruit); //   draw fruit
    myGLCD.pushImage(146, 168, 28, 28, fruitIcon);
    fruitdrawn = true;
  }

  // Redraw fruit if Ghost eats fruit only if Ghost passesover 172 or 120 on the row 186
  if ((fruitdrawn == true) && (fruitgone == false) && (xG >= 168) && (xG <= 170) && (yG >= 168) && (yG <= 180)) {
    //drawicon(146, 168, fruit); //   draw fruit
    myGLCD.pushImage(146, 168, 28, 28, fruitIcon);
  }

  if ((fruitdrawn == true) && (fruitgone == false) && (xG == 120) && (yG == 168)) {
    //drawicon(146, 168, fruit); //   draw fruit
    myGLCD.pushImage(146, 168, 28, 28, fruitIcon);
  }

  // Award Points if Pacman eats Big Dots
  if ((fruitdrawn == true) && (fruitgone == false) && (xP == 146) && (yP == 168)) {
    fruitgone = true; // If Pacman eats fruit then fruit disappears
    pacmanscore = pacmanscore + 5; //Increment pacman score
  }
}

///////////////////////////////////////////////////////////
//
//  Read the current date and time from the RTC and
//      reset the game board.
//
void refreshgame()
{
  // Read the current date and time from the RTC and reset board
#ifdef DS3231_RTC
  if (!digitalRead(RTC_INT_PIN))   // RTC interrupt
  {
    //
    //  RTC alarm #2 interrupt (occurs once a minute)
    //
    if (rtc.alarmFired(ALARM_2))  // clear RTC alarm #2 interrupt (occurs once a minute)
    {
      rtc.clearAlarm(ALARM_2);
      UpdateDisp(); // update value to clock then ...
      fruiteatenpacman =  false; // Turn Ghost red again
      fruitdrawn = false; // If Pacman eats fruit then fruit disappears
      fruitgone = false;
      // Reset every minute both characters
      pacmanlost = false;
      ghostlost = false;
      dly = gamespeed; // reset delay
      rfc = 0;
    }
  }
#else
  rfc++;
  if (rfc >= rfcvalue) { // count cycles and print time
    UpdateDisp(); // update value to clock then ...
    fruiteatenpacman =  false; // Turn Ghost red again
    fruitdrawn = false; // If Pacman eats fruit then fruit disappears
    fruitgone = false;
    // Reset every minute both characters
    pacmanlost = false;
    ghostlost = false;
    dly = gamespeed; // reset delay
    rfc = 0;
  }
#endif
}

///////////////////////////////////////////////////////////
//
//  Check to determine if the Alarm needs to be sounded
//
void triggeralarm()
{
  if (clockConfig.alarmStatus == true) {

    if ( (clockConfig.alarmHour == clockhour) && (clockConfig.alarmMinute == clockminute)) {  // Sound the alarm
      soundalarm = true;
    }

  }
}

/////////////////////////////////////////////////////////////
//
// Check if user input to touch screen
//    UserT sets direction 0 = right, 1 = down, 2 = left, 3 = up, 4 = no touch input
//    Read the Touch Screen Locations
//
//
void mainuserinput()
{
  pushTA();
#ifdef TFT_SPI
  if (touchCheck())
  {
    // If centre of screen touched while alarm sounding then turn off the sound and reset the alarm to not set
    //    if ((touchData.y >= 90) && (touchData.y <= 150))
    //    {
    //      if ((touchData.x >= 130) && (touchData.x <= 190))
    if ((touchData.button == 5) || (touchData.button == 8))   // middle of row 2 & 3
    {
      if (soundalarm == true)
      {
        clockConfig.alarmStatus = false;
        soundalarm = false;
        delay(250);
      }
      else
      {
        // **********************************
        // ******* Enter Setup Mode *********
        // **********************************
        PACMAN_PRINTLN(F("Entering Setup"));
        setupclockmenu();
      }
    }
    //    }
    if (pacmanlost == false)
    {
      TOUCH_PRINTLN("Direction Change");
      switch (touchData.button)
      {
        case 2:   // Request to go UP
          if ( D == 1)
          {
            TOUCH_PRINTLN(F("Go up"));
            D = 3;
          }
          break;
        case 4:   // Request to go LEFT
        case 7:
          if ( D == 0)
          {
            TOUCH_PRINTLN(F("Go left"));
            D = 2;
          }
          break;
        case 6:   // Request to go RIGHT
        case 9:
          if ( D == 2)
          {
            TOUCH_PRINTLN(F("Go right"));
            D = 0;
          }
          break;
        case 8:   // Request to go DOWN
          if (D == 3)
          {
            TOUCH_PRINTLN(F("Go down"));
            D = 1;
          }
          break;
        default:
          break;
      }
    }
    touchData.touchedFlag = false;
    delay(200);
  }
  //=== Start Alarm Sound - Sound pays for 10 seconds then will restart at 20 second mark

  if ((clockConfig.alarmStatus == true) && (soundalarm == true)) { // Set off a counter and take action to restart sound if screen not touched

    playalarmsound1();
    UpdateDisp(); // update value to clock
  }

#else   // not TFT_SPI

  // Check if user input to touch screen
  // UserT sets direction 0 = right, 1 = down, 2 = left, 3 = up, 4 = no touch input
  // Read the Touch Screen Locations
  TSPoint p = ts.getPoint();

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if ((p.z < MAXPRESSURE) && (p.z > 0))
  {
    TOUCH_PRINT("Z = "); TOUCH_PRINTLN(p.z);
  }

  if (( p.z < MAXPRESSURE) && ( p.z > 0)) {

    // **********************************
    // ******* Enter Setup Mode *********
    // **********************************

    // If centre of screen touched while alarm sounding then turn off the sound and reset the alarm to not set

    if ((p.z < MAXPRESSURE) && (clockConfig.alarmStatus == true) && (soundalarm == true)) {
      clockConfig.alarmStatus = false;
      soundalarm = false;
      delay(300);
    }

    if (readtouchscreen(Jx - tvar, Jx + tvar, Jy - tvar, Jy + tvar) == true) {
      if (readtouchscreen(Jx - tvar, Jx + tvar, Jy - tvar, Jy + tvar) == true) {
        userT = 2; // Request to go left   X = 1695  Y = 1959_
      }
    } else if (readtouchscreen(Kx - tvar, Kx + tvar, Ky - tvar, Ky + tvar) == true) {
      if (readtouchscreen(Kx - tvar, Kx + tvar, Ky - tvar, Ky + tvar) == true) {
        userT = 0; // Request to go right   X = 1017  Y = 3317
      }
    } else if (readtouchscreen(Ix - tvar, Ix + tvar, Iy - tvar, Iy + tvar) == true) {
      if (readtouchscreen(Ix - tvar, Ix + tvar, Iy - tvar, Iy + tvar) == true) {
        userT = 3; // Request to go Up   X = 579 Y = 2603
      }
    } else if (readtouchscreen(Hx - tvar, Hx + tvar, Hy - tvar, Hy + tvar) == true) {
      if (readtouchscreen(Hx - tvar, Hx + tvar, Hy - tvar, Hy + tvar) == true) {
        userT = 1; // Request to go Down   X = 1699 Y = 2090
      }
    } else if ((readtouchscreen(Gx - tvar, Gx + tvar, Gy - tvar, Gy + tvar) == true) &&  (soundalarm != true)) { // Call Setup Routine if alarm is not sounding
      if ((readtouchscreen(Gx - tvar, Gx + tvar, Gy - tvar, Gy + tvar) == true) &&  (soundalarm != true)) {
        setupclockmenu(); // Call the setup routine X = 1387  Y = 2280
      }
    }

    if (pacmanlost == false) { // only apply requested changes if Pacman still alive

      if (userT == 2 && D == 0 ) { // Going Right request to turn Left OK
        D = 2;
      }
      if (userT == 0 && D == 2 ) { // Going Left request to turn Right OK
        D = 0;
      }
      if (userT == 1 && D == 3 ) { // Going Up request to turn Down OK
        D = 1;
      }
      if (userT == 3 && D == 1 ) { // Going Down request to turn Up OK
        D = 3;
      }
    }

    screenPressed = true;
  }
  // Doesn't allow holding the screen / you must tap it
  else {
    screenPressed = false;
  }

  //=== Start Alarm Sound - Sound pays for 10 seconds then will restart at 20 second mark

  if ((clockConfig.alarmStatus == true) && (soundalarm == true)) { // Set off a counter and take action to restart sound if screen not touched

    playalarmsound1();
    UpdateDisp(); // update value to clock
  }
#endif
  popTA();
}

////////////////////////////////////////////////////////////
//
//  Gobble Dot (if not gobbled already)
//
void gobbleDot(int dotNum)
{
  if (dots[dotNum].state) {  // Check if dot gobbled already
    dots[dotNum].state = false; // Reset flag to Zero
    pacmanscore++; // Increment pacman score
    if (dots[dotNum].dotSize == 7)
    {
      // Turn Ghost Blue if Pacman eats Big Dots
      fruiteatenpacman = true; // Turn Ghost blue
    }
  }
}

/////////////////////////////////////////////////////////////////
//
//  Fill Dot (if it hasn't been gobbled up)
//
void fillDot(int d)
{
  // Fill dot if it hasn't been gobbled
  if (dots[d].state)
    myGLCD.fillCircle(dots[d].xPos, dots[d].yPos, dots[d].dotSize, TFT_SILVER);
}

///////////////////////////////////////////////////////////////
//
//  Fill Adjscent Dots
//
void fillAdjacentDots(int dot)
{
  for ( int i = 0; i < 4; i++)
  {
    if (adjacentDots[dot][i] != 0) fillDot(adjacentDots[dot][i]);
    else break;
  }
}

//////////////////////////////////////////////////////////////////
//
//  Display Pac-man (Mr or Ms)
//
void displaypacman() { // Draw Pacman in position on screen
  // Pacman Captured
  // If pacman captured then pacman dissapears until reset
  if ((fruiteatenpacman == false) && (abs(xG - xP) <= 5) && (abs(yG - yP) <= 5)) {
    // firstly blank out Pacman
    //    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(xP, yP, 28, 28, TFT_BLACK);

    if (pacmanlost == false) {
      ghostscore = ghostscore + 15;
    }
    pacmanlost = true;
    // Slow down speed of drawing now only one moving charater
    dly = gamespeed;
  }

  if (pacmanlost == false) { // Only draw pacman if he is still alive


    // Draw Pac-Man
    drawPacman(xP, yP, P, D, prevD); // Draws Pacman at these coordinates


    // If Pac-Man is on a dot then print the adjacent dots if they are valid

    //  myGLCD.setColor(200, 200, 200);

    // Check Rows
    byte aDot = 0;
    if (yP == 4) { // if in Row 1 **********************************************************
      for (int i = 0; i < DOTS_IN_ROW_1; i++)
      {
        if (xP == row1n6dots[i])
        {
          fillAdjacentDots(i + 1);    // dots 1-12
          break;
        }
      }

    } else if (yP == 26) { // if in Row 2  **********************************************************

      for ( int i = 0; i < DOTS_IN_ROW_2; i++)
      {
        if (xP == row2n5dots[i])
        {
          fillAdjacentDots(i + 13);   // dots 13-18
          break;
        }
      }

    } else if (yP == 46) { // if in Row 3  **********************************************************

      for ( int i = 0; i < DOTS_IN_ROW_3; i++)
      {
        if (xP == row3n4dots[i])
        {
          fillAdjacentDots(i + 19);   // dots 19-31
          break;
        }
      }

    } else if (yP == 168) { // if in Row 4  **********************************************************

      for ( int i = 0; i < DOTS_IN_ROW_4; i++)
      {
        if (xP == row3n4dots[i])
        {
          fillAdjacentDots(i + 42);   // dots 42-54
          break;
        }
      }
      if ((xP == 120) || (xP == 168)) drawFruitIcon();

    } else if (yP == 188) { // if in Row 5  **********************************************************

      for ( int i = 0; i < DOTS_IN_ROW_5; i++)
      {
        if (xP == row2n5dots[i])
        {
          fillAdjacentDots(i + 55);   // dots 55-60
          break;
        }
      }
      if ((xP == 120) || (xP == 168)) drawFruitIcon();

    } else if (yP == 208) { // if in Row 6  **********************************************************

      for ( int i = 0; i < DOTS_IN_ROW_6; i++)
      {
        if (xP == row1n6dots[i])
        {
          fillAdjacentDots(i + 61);   // dots 61-72
          break;
        }
      }

      // Check Columns
    } else if (xP == 28) { // if in Column 2 **************************

      if (yP == 66)
      { // dot 32
        fillAdjacentDots(32);
      } else if (yP == 86)
      { // dot 34
        fillAdjacentDots(34);
      } else if (yP == 106)
      { // dot 36
        fillAdjacentDots(36);
      } else if (yP == 126)
      { // dot 38
        fillAdjacentDots(38);
      } else if (yP == 146)
      { // dot 40
        fillAdjacentDots(40);
      }
    } else if (xP == 262) { // if in Column 7   ************************

      if (yP == 66)
      { // dot 33
        fillAdjacentDots(33);
      } else if (yP == 86)
      { // dot 35
        fillAdjacentDots(35);
      } else if (yP == 106)
      { // dot 37
        fillAdjacentDots(37);
      } else if (yP == 126)
      { // dot 39
        fillAdjacentDots(39);
      } else if (yP == 146)
      { // dot 41
        fillAdjacentDots(41);
      }
    }


    // increment Pacman Graphic Flag 0 = Closed, 1 = Medium Open, 2 = Wide Open
    P = P + 1;
    if (P == 4) {
      P = 0; // Reset counter to closed
    }

    // Capture legacy direction to enable adequate blanking of trail
    prevD = D;

    /* Temp print variables for testing

      myGLCD.setColor(0, 0, 0);
      myGLCD.setBackColor(114, 198, 206);

      myGLCD.drawString(xT,100,140); // Print xP
      myGLCD.drawString(yT,155,140); // Print yP
    */

    // Check if Dot has been eaten before and incrementing score

    // Check Rows

    switch (yP)
    {
      case 4:  // Row 1, dots 1 - 12
        {
          for (int i = 0; i < DOTS_IN_ROW_1; i++)
          {
            if (xP == row1n6dots[i])
            {
              gobbleDot(i + 1);     // Gobble dot and increment score
              break;
            }
          }
          break;
        }
      case 26:  // Row 2, dots 13 - 18
        {
          for ( int i = 0; i < DOTS_IN_ROW_2; i++)
          {
            if (xP == row2n5dots[i])
            {
              gobbleDot(i + 13);
              break;
            }
          }
          break;
        }
      case 46:  // Row 3, dots 19 - 31
        {
          for ( int i = 0; i < DOTS_IN_ROW_3; i++)
          {
            if (xP == row3n4dots[i])
            {
              gobbleDot(i + 19);
              break;
            }
          }
          break;
        }
      case 66:  // Row 3.1, dots 32 - 33
        {
          if (xP == 28) gobbleDot(32);
          else if (xP == 262) gobbleDot(33);
          break;
        }
      case 86:  // Row 3.1, dots 34 - 35
        {
          if (xP == 28) gobbleDot(34);
          else if (xP == 262) gobbleDot(35);
          break;
        }
      case 108: // Row 3.1, dots 36 - 37
        {
          if (xP == 28) gobbleDot(36);
          else if (xP == 262) gobbleDot(37);
          break;
        }
      case 126: // Row 3.1, dots 38 - 39
        {
          if (xP == 28) gobbleDot(38);
          else if (xP == 262) gobbleDot(39);
          break;
        }
      case 146: // Row 3.1, dots 40 - 41
        {
          if (xP == 28) gobbleDot(40);
          else if (xP == 262) gobbleDot(41);
          break;
        }
      case 168: // Row 4, dots 42 - 54
        {
          for ( int i = 0; i < DOTS_IN_ROW_4; i++)
          {
            if (xP == row3n4dots[i])
            {
              gobbleDot(i + 42);
              break;
            }
          }
          break;
        }
      case 188: // Row 5, dots 55 - 60
        {
          for ( int i = 0; i < DOTS_IN_ROW_5; i++)
          {
            if (xP == row2n5dots[i])
            {
              gobbleDot(i + 55);
              break;
            }
          }
          break;
        }
      case 208: // Row 6, dots 61 - 72
        {
          for ( int i = 0; i < DOTS_IN_ROW_6; i++)
          {
            if (xP == row1n6dots[i])
            {
              gobbleDot(i + 61);
              break;
            }
          }
          break;
        }
    };

    //Pacman wandering Algorithm
    // Note: Keep horizontal and vertical coordinates even numbers only to accomodate increment rate and starting point
    // Pacman direction variable D where 0 = right, 1 = down, 2 = left, 3 = up

    //****************************************************************************************************************************
    //Right hand motion and ***************************************************************************************************
    //****************************************************************************************************************************



    if (D == 0) {
      // Increment xP and then test if any decisions required on turning up or down
      xP = xP + cstep;

      // There are four horizontal rows that need rules

      // First Horizontal Row
      if (yP == 4) {

        // Past first block decide to continue or go down
        if (xP == 62) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past second block only option is down
        if (xP == 120) {
          D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Past third block decide to continue or go down
        if (xP == 228) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past fourth block only option is down
        if (xP == 284) {
          D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 2nd Horizontal Row
      if (yP == 46) {

        // Past upper doorway on left decide to continue or go down
        if (xP == 28) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past first block decide to continue or go up
        if (xP == 62) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past Second block decide to continue or go up
        if (xP == 120) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past Mid Wall decide to continue or go up
        if (xP == 168) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past third block decide to continue or go up
        if (xP == 228) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past last clock digit decide to continue or go down
        if (xP == 262) {
          direct = random(2); // generate random number between 0 and 2
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past fourth block only option is up
        if (xP == 284) {
          D = 3; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // LHS Door Horizontal Row
      if (yP == 108) {

        // Past upper doorway on left decide to go up or go down
        if (xP == 28) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 3;
          }
        }
      }

      // 3rd Horizontal Row
      if (yP == 168) {

        // Past lower doorway on left decide to continue or go up
        if (xP == 28) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past first block decide to continue or go down
        if (xP == 62) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past Second block decide to continue or go down
        if (xP == 120) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past Mid Wall decide to continue or go down
        if (xP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past third block decide to continue or go down
        if (xP == 228) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past last clock digit decide to continue or go up
        if (xP == 262) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past fourth block only option is down
        if (xP == 284) {
          D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }


      // 4th Horizontal Row
      if (yP == 208) {

        // Past first block decide to continue or go up
        if (xP == 62) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past second block only option is up
        if (xP == 120) {
          D = 3; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Past third block decide to continue or go up
        if (xP == 228) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past fourth block only option is up
        if (xP == 284) {
          D = 3; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }
    }

    //****************************************************************************************************************************
    //Left hand motion **********************************************************************************************************
    //****************************************************************************************************************************

    else if (D == 2) {
      // Increment xP and then test if any decisions required on turning up or down
      xP = xP - cstep;

      /* Temp print variables for testing

        myGLCD.setColor(0, 0, 0);
        myGLCD.setBackColor(114, 198, 206);
        myGLCD.drawString(xP,80,165); // Print xP
        myGLCD.drawString(yP,110,165); // Print yP
      */

      // There are four horizontal rows that need rules

      // First Horizontal Row  ******************************
      if (yP == 4) {

        // Past first block only option is down
        if (xP == 4) {
          D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Past second block decide to continue or go down
        if (xP == 62) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past third block only option is down
        if (xP == 168) {
          D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Past fourth block decide to continue or go down
        if (xP == 228) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
      }

      // 2nd Horizontal Row ******************************
      if (yP == 46) {

        // Meet LHS wall only option is up
        if (xP == 4) {
          D = 3; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Meet upper doorway on left decide to continue or go down
        if (xP == 28) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet first block decide to continue or go up
        if (xP == 62) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Meet Second block decide to continue or go up
        if (xP == 120) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet Mid Wall decide to continue or go up
        if (xP == 168) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet third block decide to continue or go up
        if (xP == 228) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet last clock digit decide to continue or go down
        if (xP == 262) {
          direct = random(2); // generate random number between 0 and 3
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
      }

      // 3rd Horizontal Row ******************************
      if (yP == 168) {

        // Meet LHS lower wall only option is down
        if (xP == 4) {
          D = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Meet lower doorway on left decide to continue or go up
        if (xP == 28) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet first block decide to continue or go down
        if (xP == 62) {
          direct = random(2); // generate random number between 0 and 3
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Meet Second block decide to continue or go down
        if (xP == 120) {
          direct = random(2); // generate random number between 0 and 3
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet Mid Wall decide to continue or go down
        if (xP == 168) {
          direct = random(2); // generate random number between 0 and 3
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet third block decide to continue or go down
        if (xP == 228) {
          direct = random(2); // generate random number between 0 and 3
          if (direct == 1) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet last clock digit above decide to continue or go up
        if (xP == 262) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
      }
      // 4th Horizontal Row ******************************
      if (yP == 208) {

        // Meet LHS wall only option is up
        if (xP == 4) {
          D = 3; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Meet first block decide to continue or go up
        if (xP == 62) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Meet bottom divider wall only option is up
        if (xP == 168) {
          D = 3; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Meet 3rd block decide to continue or go up
        if (xP == 228) {
          direct = random(4); // generate random number between 0 and 3
          if (direct == 3) {
            D = direct; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
      }
    }

    //****************************************************************************************************************************
    //Down motion **********************************************************************************************************
    //****************************************************************************************************************************

    else if (D == 1) {
      // Increment yP and then test if any decisions required on turning up or down
      yP = yP + cstep;

      /* Temp print variables for testing

        myGLCD.setColor(0, 0, 0);
        myGLCD.setBackColor(114, 198, 206);
        myGLCD.drawString(xP,80,165); // Print xP
        myGLCD.drawString(yP,110,165); // Print yP
      */

      // There are vertical rows that need rules

      // First Vertical Row  ******************************
      if (xP == 4) {

        // Past first block only option is right
        if (yP == 46) {
          D = 0; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards bottom wall only option right
        if (yP == 208) {
          D = 0; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 2nd Vertical Row ******************************
      if (xP == 28) {

        // Meet bottom doorway on left decide to go left or go right
        if (yP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 3rd Vertical Row ******************************
      if (xP == 62) {

        // Meet top lh digit decide to go left or go right
        if (yP == 46) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet top lh digit decide to go left or go right
        if (yP == 208) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 5th Vertical Row ******************************
      if (xP == 120) {

        // Meet top lh digit decide to go left or go right
        if (yP == 46) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet bottom wall only opgion to go left
        if (yP == 208) {
          D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 6th Vertical Row ******************************
      if (xP == 168) {

        // Meet top lh digit decide to go left or go right
        if (yP == 46) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet bottom wall only opgion to go right
        if (yP == 208) {
          D = 0; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 8th Vertical Row ******************************
      if (xP == 228) {

        // Meet top lh digit decide to go left or go right
        if (yP == 46) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet bottom wall
        if (yP == 208) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 9th Vertical Row ******************************
      if (xP == 262) {

        // Meet bottom right doorway  decide to go left or go right
        if (yP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 10th Vertical Row  ******************************
      if (xP == 284) {

        // Past first block only option is left
        if (yP == 46) {
          D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards bottom wall only option right
        if (yP == 208) {
          D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }
    }

    //****************************************************************************************************************************
    //Up motion **********************************************************************************************************
    //****************************************************************************************************************************

    else if (D == 3) {
      // Decrement yP and then test if any decisions required on turning up or down
      yP = yP - cstep;

      /* Temp print variables for testing

        myGLCD.setColor(0, 0, 0);
        myGLCD.setBackColor(114, 198, 206);
        myGLCD.drawString(xP,80,165); // Print xP
        myGLCD.drawString(yP,110,165); // Print yP
      */


      // There are vertical rows that need rules

      // First Vertical Row  ******************************
      if (xP == 4) {

        // Past first block only option is right
        if (yP == 4) {
          D = 0; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards bottom wall only option right
        if (yP == 168) {
          D = 0; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 2nd Vertical Row ******************************
      if (xP == 28) {

        // Meet top doorway on left decide to go left or go right
        if (yP == 46) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 3rd Vertical Row ******************************
      if (xP == 62) {

        // Meet top lh digit decide to go left or go right
        if (yP == 4) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet top lh digit decide to go left or go right
        if (yP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 5th Vertical Row ******************************
      if (xP == 120) {

        // Meet bottom lh digit decide to go left or go right
        if (yP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet top wall only opgion to go left
        if (yP == 4) {
          D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 6th Vertical Row ******************************
      if (xP == 168) {

        // Meet bottom lh digit decide to go left or go right
        if (yP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet top wall only opgion to go right
        if (yP == 4) {
          D = 0; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 8th Vertical Row ******************************
      if (xP == 228) {

        // Meet bottom lh digit decide to go left or go right
        if (yP == 168) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }

        // Meet top wall go left or right
        if (yP == 4) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 9th Vertical Row ******************************
      if (xP == 262) {

        // Meet top right doorway  decide to go left or go right
        if (yP == 46) {
          direct = random(2); // generate random number between 0 and 1
          if (direct == 1) {
            D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            D = 0;
          }
        }
      }

      // 10th Vertical Row  ******************************
      if (xP == 284) {

        // Past first block only option is left
        if (yP == 168) {
          D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards top wall only option right
        if (yP == 4) {
          D = 2; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }
    }
  }

}

////////////////////////////////////////////////////////////
//
//  Display Ghost
//
void displayghost() { // Draw Ghost in position on screen
  //******************************************************************************************************************
  //Ghost ;
  // Note: Keep horizontal and verticalcoordinates even numbers only to accomodateincrement rate and starting point
  // Ghost direction variable  D where 0 = right, 1 = down, 2 = left, 3 = up

  //****************************************************************************************************************************
  //Right hand motion **********************************************************************************************************
  //****************************************************************************************************************************


  // If ghost captured then ghost dissapears until reset
  if ((fruiteatenpacman == true) && (abs(xG - xP) <= 5) && (abs(yG - yP) <= 5)) {

    if (ghostlost == false) {
      pacmanscore++;
      pacmanscore++;
    }

    ghostlost = true;

    dly = gamespeed; // slowdown now only drawing one item
  }

  if (ghostlost == false) { // only draw ghost if still alive

    drawGhost(xG, yG, GD, prevGD); // Draws Ghost at these coordinates

    // If Ghost is on a dot then print the adjacent dots if they are valid

    //  myGLCD.setColor(200, 200, 200);

    // Check Rows

    if (yG == 4) {  // if in Row 1 **********************************************************
      for (int i = 0; i < DOTS_IN_ROW_1; i++)
      {
        if (xG == row1n6dots[i])
        {
          fillAdjacentDots(i + 1);    // dots 1-12
          break;
        }
      }

    } else if (yG == 26) { // if in Row 2  **********************************************************
      for ( int i = 0; i < DOTS_IN_ROW_2; i++)
      {
        if (xG == row2n5dots[i])
        {
          fillAdjacentDots(i + 13);   // dots 13-18
          break;
        }
      }

    } else if (yG == 46) { // if in Row 3  **********************************************************
      for ( int i = 0; i < DOTS_IN_ROW_3; i++)
      {
        if (xG == row3n4dots[i])
        {
          fillAdjacentDots(i + 19);   // dots 19-31
          break;
        }
      }

    } else if (yG == 168) {  // if in Row 4  **********************************************************
      for ( int i = 0; i < DOTS_IN_ROW_4; i++)
      {
        if (xG == row3n4dots[i])
        {
          fillAdjacentDots(i + 42);   // dots 42-54
          break;
        }
      }
      if ((xG == 120) || (xG == 168)) drawFruitIcon();

    } else if (yG == 188) { // if in Row 5  **********************************************************
      for ( int i = 0; i < DOTS_IN_ROW_5; i++)
      {
        if (xG == row2n5dots[i])
        {
          fillAdjacentDots(i + 55);   // dots 55-60
          break;
        }
      }
      if ((xG == 120) || (xG == 168)) drawFruitIcon();

    } else if (yG == 208) {  // if in Row 6  **********************************************************
      for ( int i = 0; i < DOTS_IN_ROW_6; i++)
      {
        if (xG == row1n6dots[i])
        {
          fillAdjacentDots(i + 61);   // dots 61-72
          break;
        }
      }

      // Check Columns
    } else if (xG == 28) {  // if in Column 2
      if (yG == 66) { // dot 32
        fillAdjacentDots(32);
      } else if (yG == 86) { // dot 34
        fillAdjacentDots(34);
      } else if (yG == 106) { // dot 36
        fillAdjacentDots(36);
      } else if (yG == 126) { // dot 38
        fillAdjacentDots(38);
      } else if (yG == 146) { // dot 40
        fillAdjacentDots(40);
      }

    } else if (xG == 262) { // if in Column 7
      if (yG == 66) { // dot 33
        fillAdjacentDots(33);
      } else if (yG == 86) { // dot 35
        fillAdjacentDots(35);
      } else if (yG == 106) { // dot 37
        fillAdjacentDots(37);
      } else if (yG == 126) { // dot 39
        fillAdjacentDots(39);
      } else if (yG == 146) { // dot 41
        fillAdjacentDots(41);
      }
    }

    // Capture legacy direction to enable adequate blanking of trail
    prevGD = GD;

    if (GD == 0) {
      // Increment xG and then test if any decisions required on turning up or down
      xG = xG + cstep;

      // There are four horizontal rows that need rules

      // First Horizontal Row
      if (yG == 4) {

        // Past first block decide to continue or go down
        if (xG == 62) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past second block only option is down
        if (xG == 120) {
          GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Past third block decide to continue or go down
        if (xG == 228) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past fourth block only option is down
        if (xG == 284) {
          GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 2nd Horizontal Row
      if (yG == 46) {

        // Past upper doorway on left decide to continue right or go down
        if (xG == 28) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past first block decide to continue right or go up
        if (xG == 62) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }
        // Past Second block decide to continue right or go up
        if (xG == 120) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }

        // Past Mid Wall decide to continue right or go up
        if (xG == 168) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }

        // Past third block decide to continue right or go up
        if (xG == 228) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }

        // Past last clock digit decide to continue or go down
        if (xG == 262) {
          gdirect = random(2); // generate random number between 0 and 2
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past fourth block only option is up
        if (xG == 284) {
          GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 3rd Horizontal Row
      if (yG == 168) {

        // Past lower doorway on left decide to continue right or go up
        if (xG == 28) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }

        // Past first block decide to continue or go down
        if (xG == 62) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past Second block decide to continue or go down
        if (xG == 120) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past Mid Wall decide to continue or go down
        if (xG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past third block decide to continue or go down
        if (xG == 228) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Past last clock digit decide to continue right or go up
        if (xG == 262) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }

        // Past fourth block only option is down
        if (xG == 284) {
          GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 4th Horizontal Row
      if (yG == 208) {

        // Past first block decide to continue right or go up
        if (xG == 62) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }
        // Past second block only option is up
        if (xG == 120) {
          GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Past third block decide to continue right or go up
        if (xG == 228) {
          if (random(2) == 0) {
            GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 3;
          }
        }
        // Past fourth block only option is up
        if (xG == 284) {
          GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }
    }

    //****************************************************************************************************************************
    //Left hand motion **********************************************************************************************************
    //****************************************************************************************************************************

    else if (GD == 2) {
      // Increment xG and then test if any decisions required on turning up or down
      xG = xG - cstep;

      // There are four horizontal rows that need rules

      // First Horizontal Row  ******************************
      if (yG == 4) {

        // Past first block only option is down
        if (xG == 4) {
          GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Past second block decide to continue or go down
        if (xG == 62) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Past third block only option is down
        if (xG == 168) {
          GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Past fourth block decide to continue or go down
        if (xG == 228) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
      }

      // 2nd Horizontal Row ******************************
      if (yG == 46) {

        // Meet LHS wall only option is up
        if (xG == 4) {
          GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Meet upper doorway on left decide to continue left or go down
        if (xG == 28) {
          if (random(2) == 0) {
            GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }

        // Meet first block decide to continue left or go up
        if (xG == 62) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }
        // Meet Second block decide to continue left or go up
        if (xG == 120) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }

        // Meet Mid Wall decide to continue left or go up
        if (xG == 168) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }

        // Meet third block decide to continue left or go up
        if (xG == 228) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }

        // Meet last clock digit decide to continue or go down
        if (xG == 262) {
          gdirect = random(2); // generate random number between 0 and 3
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
      }

      // RHS Door Horizontal Row
      if (yG == 108) {

        // Past upper doorway on left decide to go up or go down
        if (xG == 262) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 1; // set Pacman direciton varialble to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 3;
          }
        }
      }

      // 3rd Horizontal Row ******************************
      if (yG == 168) {

        // Meet LHS lower wall only option is down
        if (xG == 4) {
          GD = 1; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Meet lower doorway on left decide to continue left or go up
        if (xG == 28) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }

        // Meet first block decide to continue or go down
        if (xG == 62) {
          gdirect = random(2); // generate random number between 0 and 3
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }
        // Meet Second block decide to continue or go down
        if (xG == 120) {
          gdirect = random(2); // generate random number between 0 and 3
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet Mid Wall decide to continue or go down
        if (xG == 168) {
          gdirect = random(2); // generate random number between 0 and 3
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet third block decide to continue or go down
        if (xG == 228) {
          gdirect = random(2); // generate random number between 0 and 3
          if (gdirect == 1) {
            GD = gdirect; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
        }

        // Meet last clock digit above decide to continue left or go up
        if (xG == 262) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }
      }
      // 4th Horizontal Row ******************************
      if (yG == 208) {

        // Meet LHS wall only option is up
        if (xG == 4) {
          GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Meet first block decide to continue left or go up
        if (xG == 62) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }
        // Meet bottom divider wall only option is up
        if (xG == 168) {
          GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
        // Meet 3rd block decide to continue left or go up
        if (xG == 228) {
          if (random(2) == 0) {
            GD = 3; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          } else {
            GD = 2;
          }
        }
      }
    }


    //****************************************************************************************************************************
    //Down motion **********************************************************************************************************
    //****************************************************************************************************************************

    else if (GD == 1) {
      // Increment yGand then test if any decisions required on turning up or down
      yG = yG + cstep;

      // There are vertical rows that need rules

      // First Vertical Row  ******************************
      if (xG == 4) {

        // Past first block only option is right
        if (yG == 46) {
          GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards bottom wall only option right
        if (yG == 208) {
          GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 2nd Vertical Row ******************************
      if (xG == 28) {

        // Meet bottom doorway on left decide to go left or go right
        if (yG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 3rd Vertical Row ******************************
      if (xG == 62) {

        // Meet top lh digit decide to go left or go right
        if (yG == 46) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet top lh digit decide to go left or go right
        if (yG == 208) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 5th Vertical Row ******************************
      if (xG == 120) {

        // Meet top lh digit decide to go left or go right
        if (yG == 46) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet bottom wall only opgion to go left
        if (yG == 208) {
          GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 6th Vertical Row ******************************
      if (xG == 168) {

        // Meet top lh digit decide to go left or go right
        if (yG == 46) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet bottom wall only opgion to go right
        if (yG == 208) {
          GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 8th Vertical Row ******************************
      if (xG == 228) {

        // Meet top lh digit decide to go left or go right
        if (yG == 46) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet bottom wall
        if (yG == 208) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 9th Vertical Row ******************************
      if (xG == 262) {

        // Meet bottom right doorway  decide to go left or go right
        if (yG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 10th Vertical Row  ******************************
      if (xG == 284) {

        // Past first block only option is left
        if (yG == 46) {
          GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards bottom wall only option right
        if (yG == 208) {
          GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }
    }

    //****************************************************************************************************************************
    //Up motion **********************************************************************************************************
    //****************************************************************************************************************************

    else if (GD == 3) {
      // Decrement yGand then test if any decisions required on turning up or down
      yG = yG - cstep;

      // There are vertical rows that need rules

      // First Vertical Row  ******************************
      if (xG == 4) {

        // Past first block only option is right
        if (yG == 4) {
          GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards bottom wall only option right
        if (yG == 168) {
          GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 2nd Vertical Row ******************************
      if (xG == 28) {

        // Meet top doorway on left decide to go left or go right
        if (yG == 46) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 3rd Vertical Row ******************************
      if (xG == 62) {

        // Meet top lh digit decide to go left or go right
        if (yG == 4) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet top lh digit decide to go left or go right
        if (yG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 5th Vertical Row ******************************
      if (xG == 120) {

        // Meet bottom lh digit decide to go left or go right
        if (yG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet top wall only opgion to go left
        if (yG == 4) {
          GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 6th Vertical Row ******************************
      if (xG == 168) {

        // Meet bottom lh digit decide to go left or go right
        if (yG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet top wall only opgion to go right
        if (yG == 4) {
          GD = 0; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }

      // 8th Vertical Row ******************************
      if (xG == 228) {

        // Meet bottom lh digit decide to go left or go right
        if (yG == 168) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }

        // Meet top wall go left or right
        if (yG == 4) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 9th Vertical Row ******************************
      if (xG == 262) {

        // Meet top right doorway  decide to go left or go right
        if (yG == 46) {
          gdirect = random(2); // generate random number between 0 and 1
          if (gdirect == 1) {
            GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
          }
          else {
            GD = 0;
          }
        }
      }

      // 10th Vertical Row  ******************************
      if (xG == 284) {

        // Past first block only option is left
        if (yG == 168) {
          GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }

        // Towards top wall only option right
        if (yG == 4) {
          GD = 2; // set Ghost direction variable to new direction D where 0 = right, 1 = down, 2 = left, 3 = up
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////
//
//  Update Time Display
//
void UpdateDisp() {

  printLocalTime(); // Recalculate current time


  int h; // Hour value in 24 hour format
  int e; // Minute value in minute format
  int pm = 0; // Flag to detrmine if PM or AM

  // There are four digits that need to be drawn independently to ensure consisitent positioning of time
  int d1;  // Tens hour digit
  int d2;  // Ones hour digit
  int d3;  // Tens minute digit
  int d4;  // Ones minute digit


  h = clockhour; // 24 hour RT clock value
  e = clockminute;

  /* TEST
    h = 12;
    e = 8;
  */
  // h = 23;
  // e = 38;

  // Calculate hour digit values for time
  if (clockConfig.clock24)
  {
    if (h < 10)   // hours 01 - 09
    {
      d1 = 0;
      d2 = h;
    }
    else if ((h >= 10) && (h <= 19)) {     // hours 10 - 19
      d1 = 1;               // calculate Tens hour digit
      d2 = h - 10;          // calculate Ones hour digit
    } else {                // hours 20 - 23
      d1 = 2;               // calculate Tens hour digit
      d2 = h - 20;          // calculate Ones hour digit 0,1,2
    }
  }
  else      //  12 hour display
  {
    if ((h >= 10) && (h <= 12)) {     // AM hours 10,11,12
      d1 = 1; // calculate Tens hour digit
      d2 = h - 10;  // calculate Ones hour digit 0,1,2
    } else if ( (h >= 22) && (h <= 24)) {   // PM hours 10,11,12
      d1 = 1; // calculate Tens hour digit
      d2 = h - 22;  // calculate Ones hour digit 0,1,2
    } else if ((h <= 9) && (h >= 1)) {  // AM hours below ten
      d1 = 0; // calculate Tens hour digit
      d2 = h;  // calculate Ones hour digit 0,1,2
    } else if ( (h >= 13) && (h <= 21)) { // PM hours below 10
      d1 = 0; // calculate Tens hour digit
      d2 = h - 12;  // calculate Ones hour digit 0,1,2
    } else {
      // If hour is 0
      d1 = 1; // calculate Tens hour digit
      d2 = 2;  // calculate Ones hour digit 0,1,2
    }
  }
  // Calculate minute digit values for time

  if ((e >= 10)) {
    d3 = e / 10 ; // calculate Tens minute digit 1,2,3,4,5
    d4 = e - (d3 * 10); // calculate Ones minute digit 0,1,2
  } else {
    // e is less than 10
    d3 = 0;
    d4 = e;
  }

  if (h >= 12) { // Set
    //  h = h-12; // Work out value
    pm = 1;  // Set PM flag
  }

  // *************************************************************************
  // Print each digit if it has changed to reduce screen impact/flicker

  // Set digit font colour to white

  //  myGLCD.setColor(255, 255, 255);
  //  myGLCD.setBackColor(0, 0, 0);
  //  myGLCD.setFont(SevenSeg_XXXL_Num);

  pushTA();       // push Text Attributes on stack

  myGLCD.setTextColor(TFT_WHITE, TFT_BLACK);
  myGLCD.setTextSize(2);
  //myGLCD.setFreeFont(FF20);
  setGfxFont(FF20);


  // First Digit
  if (((d1 != c1) || (xsetup == true)) && (d1 != 0)) { // Do not print zero in first digit position
    myGLCD.drawNumber(d1, 51, 85); // Printing thisnumber impacts LFH walls so redraw impacted area
    // ---------------- reprint two left wall pillars
    //    myGLCD.setColor(1, 73, 240);

    myGLCD.drawRoundRect(0 , 80  , 27  , 25  , 2 , TFT_BLUE);
    myGLCD.drawRoundRect(2 , 85  , 23  , 15  , 2 , TFT_BLUE);

    myGLCD.drawRoundRect(0 , 140 , 27  , 25  , 2 , TFT_BLUE);
    myGLCD.drawRoundRect(2 , 145 , 23  , 15  , 2 , TFT_BLUE);

    // ---------------- Clear lines on Outside wall
    //    myGLCD.setColor(0,0,0);
    myGLCD.drawRoundRect(1 , 1 , 317 , 237 , 2 , TFT_BLACK);
  }
  //If prevous time 12:59 or 00:59 and change in time then blank First Digit

  if (clockConfig.clock24 == false)   // 12 hour clock display
  {
    if ((c1 == 1) && (c2 == 2) && (c3 == 5) && (c4 == 9) && (d2 != c2) )
    { // Clear the previouis First Digit and redraw wall
      //    myGLCD.setColor(0,0,0);
      myGLCD.fillRect(50  , 70  , 45  , 95, TFT_BLACK);
    }
    if ((c1 == 0) && (c2 == 0) && (c3 == 5) && (c4 == 9) && (d2 != c2) )
    { // Clear the previouis First Digit and redraw wall
      //    myGLCD.setColor(0,0,0);
      myGLCD.fillRect(50  , 70  , 45  , 95, TFT_BLACK);
    }
  }
  else    // 24 hour clock display
  {
    if ((c1 == 2) && (c2 == 3) && (c3 == 5) && (c4 == 9) && (d2 != c2) )
    { // Clear the previouis First Digit and redraw wall
      //    myGLCD.setColor(0,0,0);
      myGLCD.fillRect(50  , 70  , 45  , 95, TFT_BLACK);
    }
  }
  // Reprint the dots that have not been gobbled
  //    myGLCD.setColor(200,200,200);
  // Row 4
  fillDot(32);
  fillDot(34);
  fillDot(36);
  fillDot(38);
  fillDot(40);

  myGLCD.setTextColor(TFT_WHITE, TFT_BLACK); // myGLCD.setTextSize(20);

  if (clockConfig.clock24 == false)   // 12 hour clock display
  {
    // Second Digit - Hour ones digit
    if ((d2 != c2) || (xsetup == true)) {
      myGLCD.drawNumber(d2, 91, 85); // Print 0
    }

    // Third Digit - Minute tens digit
    if ((d3 != c3) || (xsetup == true)) {
      myGLCD.drawNumber(d3, 156, 85); // Was 145
    }

    // Fourth Digit - Minute ones digit
    if ((d4 != c4) || (xsetup == true)) {
      myGLCD.drawNumber(d4, 211, 85); // Was 205
    }

    if (xsetup == true) {
      xsetup = false; // Reset Flag now leaving setup mode
    }
    // Print PM or AM

    popTA();    // Pop Text Attributes from stack

    myGLCD.setTextColor(TFT_WHITE, TFT_BLACK);
    myGLCD.setTextSize(1);

    //myGLCD.setFreeFont(NULL);
    //setGfxFont(NULL);

    if (pm == 0) {
      myGLCD.drawString("AM", 300, 148);
    } else {
      myGLCD.drawString("PM", 300, 148);
    }
    // Round dots (Colon between hour and minute)
    myGLCD.fillCircle(148, 112, 4, TFT_WHITE);
    myGLCD.fillCircle(148, 132, 4, TFT_WHITE);
  }
  else            // 24 hour clock display
  {
    // Second Digit - Hour ones digit
    if ((d2 != c2) || (xsetup == true)) {
      myGLCD.drawNumber(d2, 99, 85); // 91, Print 0
    }

    // Third Digit - Minute tens digit
    if ((d3 != c3) || (xsetup == true)) {
      myGLCD.drawNumber(d3, 163, 85); // 156, Was 145
    }

    // Fourth Digit - Minute ones digit
    if ((d4 != c4) || (xsetup == true)) {
      myGLCD.drawNumber(d4, 215, 85); // 211, Was 205
    }

    if (xsetup == true) {
      xsetup = false; // Reset Flag now leaving setup mode
    }
    popTA();    // Pop Text Attributes from stack

    myGLCD.setTextColor(TFT_WHITE, TFT_BLACK);
    myGLCD.setTextSize(1);
    // Round dots (Colon between hour and minute)
    myGLCD.fillCircle(156, 112, 4, TFT_WHITE);    // 148
    myGLCD.fillCircle(156, 132, 4, TFT_WHITE);    // 148
  }

  // ----------- Alarm Set on LHS lower pillar
  if (clockConfig.alarmStatus == true) { // Print AS on fron screenleft hand side
    myGLCD.drawString("AS", 7, 147);
  }

  // Round dots (Colon between hour and minute)

  //  myGLCD.setColor(255, 255, 255);
  //  myGLCD.setBackColor(0, 0, 0);

  //myGLCD.fillCircle(156, 112, 4, TFT_WHITE);    // 148
  //myGLCD.fillCircle(156, 132, 4, TFT_WHITE);    // 148

  //--------------------- copy exising time digits to global variables so that these can be used to test which digits change in future

  c1 = d1;
  c2 = d2;
  c3 = d3;
  c4 = d4;


}

///////////////////////////////////////////////////////////////
//
// ===== initiateGame - Custom Function

///////////////////////////////////////////////////////////////
//
//  Draw Screen
//
void drawscreen() {

  // test only

  //  myGLCD.fillRect(100, 100, 40, 80, TFT_RED);

  //Draw Background lines

  //      myGLCD.setColor(1, 73, 240);

  //
  // ---------------- Outside wall
  //
  myGLCD.drawRoundRect(0, 0, 319, 239, 2, TFT_BLUE); // X,Y location then X,Y Size
  myGLCD.drawRoundRect(2, 2, 315, 235, 2, TFT_BLUE);
  //
  // ---------------- Four top spacers and wall pillar
  //
  // Top Left spacer wall (horiz)
  myGLCD.drawRoundRect(35 , 35  , 25  , 10 , 2 ,  TFT_BLUE);
  myGLCD.drawRoundRect(37 , 37  , 21  ,  6 , 2, TFT_BLUE);
  //
  // Top Left center spacer wall (horiz)
  myGLCD.drawRoundRect(93 , 35  , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(95 , 37  , 21  , 6 , 2 , TFT_BLUE);
  //
  // Top Right center spacer wall (horiz)
  myGLCD.drawRoundRect(201 , 35  , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(203 , 37  , 21  , 6 , 2 , TFT_BLUE);
  //
  // Top Right spacer wall (horiz)
  myGLCD.drawRoundRect(258 , 35  , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(260 , 37  , 21  , 6 , 2 , TFT_BLUE);
  //
  // Top Center Wall (vert)
  myGLCD.drawRoundRect(155 , 0 , 10  , 45  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(157 , 2 , 6 , 41  , 2 , TFT_BLUE);
  //
  // ---------------- Four bottom spacers and wall pillar
  //
  // Bottom Left spacer wall (horiz)
  myGLCD.drawRoundRect(35 , 196 , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(37 , 198 , 21  , 6 , 2 , TFT_BLUE);
  //
  // Bottom Left center spacer wall (horiz)
  myGLCD.drawRoundRect(93 , 196 , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(95 , 198 , 21  , 6 , 2 , TFT_BLUE);
  //
  // Bottom Right center spacer wall (horiz)
  myGLCD.drawRoundRect(201 , 196 , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(203 , 198 , 21  , 6 , 2 , TFT_BLUE);
  //
  // Bottom Right spacer wall (horiz)
  myGLCD.drawRoundRect(258 , 196 , 25  , 10  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(260 , 198 , 21  , 6 , 2 , TFT_BLUE);
  //
  // Bottom Center Wall (vert)
  myGLCD.drawRoundRect(155 , 196 , 10  , 43  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(157 , 198 , 6 , 39  , 2 , TFT_BLUE);
  //
  // ---------- Four Door Pillars
  //
  // Top Left door pillar
  myGLCD.drawRoundRect(0 , 80  , 27  , 25  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(2 , 85  , 23  , 15  , 2 , TFT_BLUE);
  //
  // Bottom Left door pillar
  myGLCD.drawRoundRect(0 , 140 , 27  , 25  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(2 , 145 , 23  , 15  , 2 , TFT_BLUE);
  //
  // Top Right door pillar
  myGLCD.drawRoundRect(292 , 80  , 27  , 25  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(294 , 85  , 23  , 15  , 2 , TFT_BLUE);
  //
  // Bottom Right door pillar
  myGLCD.drawRoundRect(292 , 140 , 27  , 25  , 2 , TFT_BLUE);
  myGLCD.drawRoundRect(294 , 145 , 23  , 15  , 2 , TFT_BLUE);
  //
  // ---------------- Clear lines on Outside wall
  //
  myGLCD.drawRoundRect(1 , 1 , 317 , 237 , 2 , TFT_BLACK);
  // Clear doorways (between pillars)
  myGLCD.fillRect(0  , 106 , 3 , 33  ,  TFT_BLACK);
  myGLCD.fillRect(316  , 106 , 3 , 33 , TFT_BLACK);

  // Draw Dots
  //  myGLCD.setColor(200, 200, 200);
  //  myGLCD.setBackColor(0, 0, 0);

  // delay(10000);

  for (int i = 1; i < 73; i++)
  {
    if (dots[i].state)  // if true display dot
      myGLCD.fillCircle(dots[i].xPos, dots[i].yPos, dots[i].dotSize, TFT_SILVER);
  }

  // TempTest delay

  // delay(100000);
}

//*****************************************************************************************************
//====== Draws the Pacman - bitmap
//*****************************************************************************************************
void drawPacman(int x, int y, int p, int d, int pd) {

  // Draws the Pacman - bitmap
  //  // Pacman direction d == 0 = right, 1 = down, 2 = left, 3 = up
  //  myGLCD.setColor(0, 0, 0);
  //  myGLCD.setBackColor(0, 0, 0);

  //Serial.println(p);
  if (p == 3) p = 1;      // because p sometimes is 3 ? should only ever be 0 to 2.
  //Serial.println(d);

  switch (pd)     // Previous Pacman direction
  {
    case DIR_RIGHT:
      myGLCD.fillRect(x - 1, y, 2, 28, TFT_BLACK); // Clear LEFT trail off graphic
      break;
    case DIR_DOWN:
      myGLCD.fillRect(x, y - 1, 28, 2, TFT_BLACK); // Clear TOP trail off graphic
      break;
    case DIR_LEFT:
      myGLCD.fillRect(x + 28, y, 2, 28, TFT_BLACK); // Clear RIGHT trail off graphic
      break;
    case DIR_UP:
      myGLCD.fillRect(x, y + 28, 28, 2, TFT_BLACK); // Clear BOTTOM trail off graphic
      break;
    default:
      break;
  }
#ifdef SPRITE
  sprPacman.pushImage(0, 0, 28, 28, ptrPacman[p][d]);
  sprPacman.pushSprite(x, y);
#else
  myGLCD.pushImage(x, y, 28, 28, ptrPacman[p][d]);
#endif
}

//**********************************************************************************************************
//====== Draws the Ghost - bitmap
//**********************************************************************************************************
void drawGhost(int x, int y, int d, int pd)
{
  // Draws the Ghost - bitmap
  //  // Ghost direction d == 0 = right, 1 = down, 2 = left, 3 = up

  switch (pd)
  {
    case DIR_RIGHT:
      myGLCD.fillRect(x - 1, y, 2, 28, TFT_BLACK); // Clear trail off graphic before printing new position
      break;
    case DIR_DOWN:
      myGLCD.fillRect(x, y - 1, 28, 2, TFT_BLACK); // Clear trail off graphic before printing new position
      break;
    case DIR_LEFT:
      myGLCD.fillRect(x + 28, y, 2, 28, TFT_BLACK); // Clear trail off graphic before printing new positin
      break;
    case DIR_UP:
      myGLCD.fillRect(x, y + 28, 28, 2, TFT_BLACK); // Clear trail off graphic before printing new position
      break;
    default:
      break;
  }

  if (fruiteatenpacman == true) {
#ifdef SPRITE
    sprGhost.pushImage(0, 0, 28, 28, blueGhost);
    sprGhost.pushSprite(x, y);
#else
    myGLCD.pushImage(x, y, 28, 28, blueGhost);
#endif
  } else {
#ifdef SPRITE
    sprGhost.pushImage(0, 0, 28, 28, ptrGhost[d]);
    sprGhost.pushSprite(x, y);
#else
    myGLCD.pushImage(x, y, 28, 28, ptrGhost[d]);
#endif
  }
}

///////////////////////////////////////////////////////////
//
// ================= Decimal to BCD converter
//
byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

//////////////////////////////////////////////////////////
//
//  Draw Fruit Icon
//
void drawFruitIcon(void)
{
  if ((fruitdrawn == true) && (fruitgone == false))
  {
    myGLCD.pushImage(146, 168, 28, 28, fruitIcon);
  }
}

////////////////////////////////////////////////////////////
//
//  Print Local Time
//
void printLocalTime()
{
#ifdef DS3231_RTC
  struct tm *timeinfo;
  DateTime now = rtc.now();

  time_t rawTime = now.unixtime();
  timeinfo = localtime(&rawTime);
  clockhour = timeinfo->tm_hour;
  clockminute = timeinfo->tm_min;
  Serial.println(timeinfo, "%A, %B %d %Y %H:%M:%S");
#else
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println(F("Failed to obtain time"));
    //return;
  }
  else
  {
    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    clockhour = timeinfo.tm_hour;
    clockminute = timeinfo.tm_min;
  }
#endif
}


//////////////////////////////////////////////////////////////
//
//  Play Alarm Sound #1
//
void playalarmsound1()
{

  // UNMUTE sound to speaker via pin 5 of PAM8403 using ESP32 pin 32
  digitalWrite(MUTE_PIN, UNMUTE);
  /*
    for (int startsound = 1; startsound < 8500000; startsound++)
    {
      DacAudio.FillBuffer();                // Fill the sound buffer with data
      if (Pacman.Playing == false)    // if not playing,
        DacAudio.Play(&Pacman);
    }
  */
  DacAudio.FillBuffer();
  if (Pacman.Playing == false) DacAudio.Play(&Pacman);
  delay(10);
  while (Pacman.Playing == true) {
    DacAudio.FillBuffer();
    delay(10);
  }

  // MUTE sound to speaker via pin 5 of PAM8403 using ESP32 pin 32
  digitalWrite(MUTE_PIN, MUTE);
}

///////////////////////////////////////////////////////////
//
//  Play Alarm Sound #2
//
void playalarmsound2() {

  // UNMUTE sound to speaker via pin 5 of PAM8403 using ESP32 pin 32
  digitalWrite(MUTE_PIN, UNMUTE);
  /*
    for (int startsound = 1; startsound < 1500000; startsound++)
    {
      DacAudio.FillBuffer();                // Fill the sound buffer with data
      if (pacmangobble.Playing == false)    // if not playing,
        DacAudio.Play(&pacmangobble);
    }
  */
  DacAudio.FillBuffer();
  if (pacmangobble.Playing == false) DacAudio.Play(&pacmangobble);
  delay(10);
  while (pacmangobble.Playing == true) {
    DacAudio.FillBuffer();
    delay(10);
  }

  // MUTE sound to speaker via pin 5 of PAM8403 using ESP32 pin 32
  digitalWrite(MUTE_PIN, MUTE);
}


#ifndef TFT_SPI

//
//  TFT with Parallel Interface
//  Read Touch Screen
//
boolean readtouchscreen(int xr1, int xr2, int yr1, int yr2 ) { // pass parameters to this loop to debounce and validate a screen touch to avoid spurious resutls

  boolean touchresult;  // Holds return value of operation
  int xnum = 0; // find average of x coordinate over samples
  int ynum = 0; // find average of y coordinate over samples
  int numsamples = 5; // Number of samples taken

  // Firstly quickly read the screen value 20 times then take the last value as the correct value

  TSPoint p = ts.getPoint();
  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  delay(2);

  for (int screenread = 0; screenread < numsamples; screenread++) {

    // if sharing pins, you'll need to fix the directions of the touchscreen pins
    delay(2);
    p = ts.getPoint();

    xnum = xnum + p.x;
    ynum = ynum + p.y;

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
  }

  xT = (xnum / numsamples) + 3000;
  yT = (ynum / numsamples) + 3000;


  TOUCH_PRINT("X = "); TOUCH_PRINT(xT);
  TOUCH_PRINT("\tY = "); TOUCH_PRINT(yT);
  TOUCH_PRINTLN("");

  // Calculate result

  if ((xT >= xr1) && (xT <= xr2) && (yT >= yr1) && (yT <= yr2)) { //  if this condition met then retuen a TRUE value
    touchresult = true;
  } else {
    touchresult = false;
  }

  return touchresult;
}
#endif
