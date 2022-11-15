/*
  Licensed as Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
  You are free to:
  Share — copy and redistribute the material in any medium or format
  Adapt — remix, transform, and build upon the material
  Under the following terms:
  Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
  NonCommercial — You may not use the material for commercial purposes.
  ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original
*/
// #############################################################
//
//      TTTTTTT   OOOOO    U     U    CCCCC    H     H
//         T     O     O   U     U   C     C   H     H
//         T     O     O   U     U   C         HHHHHHH
//         T     O     O   U     U   C     C   H     H
//         T      ooooo     UUUUU     CCCCC    H     H
//
// #############################################################

#ifdef TFT_SPI   ///////////////////////////////////////

//
//  TFT SPI only
//  Read Touch Screen position (x,y)
//
bool touchCheck(void)
{
  uint16_t x, y;
  // Check to verify previous TFT touch was processed
  //  if it hasn't been processed return true.
  //  if (touchData.touchedFlag)
  //  {
  //    return (true);
  //  }
  // See if there's any new touch data
  if (myGLCD.getTouch(&x, &y))
  {
    touchData.x = x;
    touchData.y = y;
    touchData.row = 0;
    touchData.column = 0;
    if (x <= 100) //  Left Column
    {
      touchData.button = 1;
    }
    else if (x >= 220)  // Right Column
    {
      touchData.button = 3;
    }
    else    // Center column
      touchData.button = 2;
    if (y >= 180)   // Row #4 (bottom)
    {
      touchData.button += 9;
    }
    else if (y >= 120) // Row #3
    {
      touchData.button += 6;
    }
    else if (y >= 60) // Row #2
    {
      touchData.button += 3;
    }
#ifdef DEBUG_TOUCH
    Serial.printf("Button = %d,  x = %d, y = %d\n\r", touchData.button, x, y);
#endif
    touchData.row = (y - pgStart) / pgLineHeight;
    TOUCH_PRINTF("row = %d\n\r", touchData.row);
    //
    // determine what column (button) was touched
    //
    if ( x < col_1_Start )
    {
      touchData.column = 0;
      TOUCH_PRINTLN("Touch column = 0");
    }
    else if ( x < col_2_Start )
    {
      touchData.column = 1;
      TOUCH_PRINTLN("Touch column = 1");
    }
    else if ( x < col_3_Start )
    {
      touchData.column = 2;
      TOUCH_PRINTLN("Touch column = 2");
    }
    else
    {
      touchData.column = 3;
      TOUCH_PRINTLN("Touch column = 3");
    }
    delay(200);
    return (true);
  }
  return (false);
}


//
//
//
void setGfxFont(const GFXfont *font)
{
  gfxFont = font;
  myGLCD.setFreeFont(font);
}

typedef struct ATTR_STRUCT {
  uint8_t textSize;
  uint8_t textFont;
  uint8_t textDatum;
  uint32_t textColor;
  uint32_t textBgColor;
  const GFXfont *freeFont;
} TEXT_ATTR;

#define ATTR_STACK_SIZE 8
TEXT_ATTR *textAttr;
TEXT_ATTR attrStack[ATTR_STACK_SIZE];
int attrStkPtr = 0;

//
//  Push Text Attributes
//
void pushTA(void)
{
  if (attrStkPtr < ATTR_STACK_SIZE)
  {
#ifdef PUSH_POP_DEBUG
    Serial.printf("pre-PUSH %x\n\r", attrStkPtr);

    Serial.printf("textfont = %d\n\r", myGLCD.textfont);
    Serial.printf("textsize = %d\n\r", myGLCD.textsize);
    Serial.printf("textdatum = %d\n\r", myGLCD.textdatum);
    Serial.printf("freeFont = %x\n\r", gfxFont);
#endif
    attrStack[attrStkPtr].textFont = myGLCD.textfont;
    attrStack[attrStkPtr].textSize = myGLCD.textsize;
    attrStack[attrStkPtr].textDatum = myGLCD.textdatum;
    attrStack[attrStkPtr].freeFont = gfxFont;
    attrStack[attrStkPtr].textColor = myGLCD.textcolor;
    attrStack[attrStkPtr].textBgColor = myGLCD.textbgcolor;
    attrStkPtr++;
#ifdef PUSH_POP_DEBUG
    Serial.printf("post-PUSH %x\n\r", attrStkPtr);
#endif
  }
  else
  {
    Serial.println("Attribute stack overflow");
    //sysLog("Attribute stack overflow");
    // Dump Stack
    for (int i = 0; i < ATTR_STACK_SIZE; i++)
    {
      Serial.printf("%d = %d\r\n", i, attrStack[i]);
    }
    delay(2000);
    ESP.restart();
    while (1);
  }
}

//
//  Pop Text Attributes
//
void popTA(void)
{
  if (attrStkPtr > 0)
  {
#ifdef PUSH_POP_DEBUG
    Serial.printf("pre-POP %x\n\r", attrStkPtr);
#endif
    --attrStkPtr;
#ifdef PUSH_POP_DEBUG
    Serial.printf("post-POP %x\n\r", attrStkPtr);
    Serial.printf("textfont = %d\n\r", attrStack[attrStkPtr].textFont);
    Serial.printf("textsize = %d\n\r", attrStack[attrStkPtr].textSize);
    Serial.printf("textdatum = %d\n\r", attrStack[attrStkPtr].textDatum);
    Serial.printf("freeFont = %x\n\r", attrStack[attrStkPtr].freeFont);
#endif
    myGLCD.setTextDatum(attrStack[attrStkPtr].textDatum);
    myGLCD.setTextSize(attrStack[attrStkPtr].textSize);
    myGLCD.setTextFont(attrStack[attrStkPtr].textFont);
    gfxFont = attrStack[attrStkPtr].freeFont;
    myGLCD.setFreeFont(gfxFont);
    //myGLCD.setTextColor(attrStack[attrStkPtr].textColor);
    myGLCD.textcolor = attrStack[attrStkPtr].textColor;
    myGLCD.textbgcolor = attrStack[attrStkPtr].textBgColor;
  }
  else
  {
    Serial.println("Attribute stack underflow");
    //sysLog("Attribute stack underflow");
    // Dump Stack
    for (int i = 0; i < ATTR_STACK_SIZE; i++)
    {
      Serial.printf("%d = %d\r\n", i, attrStack[i]);
    }
    delay(2000);
    ESP.restart();
    while (1);
  }

}

//
//  Draw Button (1 - 12)
//
//    Four ROWS by Three COLUMNS (12 Buttons)
//      Top row (left to right) is Buttons #1 #2 #3
//      Second row (left to right) is Buttons #4 #5 #6
//      Third row (left to right) is Buttons #7 #8 #9
//      Bottom row (left to right) is Buttons #10 #11 #12
//
//      There is no Button #0 (ZERO)
//
#define B_RADIUS 10
#define B_HEIGHT 45
#define B_WIDTH 90
//
//    Location of text within Button (x and y axis)
//
int xPosTblText[3] = {55, 160, 265};        // index = (buttonNbr - 1) % 3
int yPosTblText1[4] = {31, 89, 149, 207};   // index = (buttonNbr - 1) / 3
int yPosTblText2[4] = {22, 80, 140, 198};   // index = (buttonNbr - 1) / 3
//
//    Draw Button
//        Input:  buttonNbr = Button Number
//                color = Color of Button Text
//                text = Button Text (first line)
//                text2 = Second of Button Text (if not blank, "")
//
void drawBtn(int buttonNbr, int color, String text, String text2)
{
  int yPos1 = 0;
  int yPos2 = 0;
  //  First validate Button Number
  if ((buttonNbr < 1) || (buttonNbr > 12)) return;
  // Push Text Attributes on attribute stack
  pushTA();
  int xPos = xPosTblText[(buttonNbr - 1) % 3];
  setGfxFont(FSS9);
  myGLCD.setTextSize(1);
  myGLCD.setTextColor(color, TFT_BLACK);
  myGLCD.setTextDatum(MC_DATUM);         // Middle-Center Text
  //  Determine if Button contains one or two lines of text
  if (text2.length() == 0)  // if one line of text use table #1
  {
    yPos1 = yPosTblText1[(buttonNbr - 1) / 3];
  }
  else  // if two lines of text use table #2 for first line
  {
    yPos1 = yPosTblText2[(buttonNbr - 1) / 3];
    yPos2 = yPos1 + 16;             // Second line of text is at +16
  }
  //  Draw blank Button
  switch (buttonNbr)
  {
    case 1:
      myGLCD.fillRoundRect(10 , 10 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(10 , 10 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #1
      break;
    case 2:
      myGLCD.fillRoundRect(115 , 10 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(115 , 10 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW);  // Button #2
      break;
    case 3:
      myGLCD.fillRoundRect(220 , 10 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(220 , 10 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW);  // Button #3
      break;
    case 4:
      myGLCD.fillRoundRect(10 , 68 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(10 , 68 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #4
      break;
    case 5:
      myGLCD.fillRoundRect(115 , 68 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(115 , 68 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #5
      break;
    case 6:
      myGLCD.fillRoundRect(220 , 68 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(220 , 68 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #6
      break;
    case 7:
      myGLCD.fillRoundRect(10 , 127 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(10 , 127 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #7
      break;
    case 8:
      myGLCD.fillRoundRect(115 , 127 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(115 , 127 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #8
      break;
    case 9:
      myGLCD.fillRoundRect(220 , 127 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(220 , 127 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #9
      break;
    case 10:
      myGLCD.fillRoundRect(10 , 185 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(10 , 185 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #10
      break;
    case 11:
      myGLCD.fillRoundRect(115 , 185 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(115 , 185 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #11
      break;
    case 12:
      myGLCD.fillRoundRect(220 , 185 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_BLACK);
      myGLCD.drawRoundRect(220 , 185 , B_WIDTH  , B_HEIGHT  , B_RADIUS , TFT_YELLOW); // Button #12
      break;
    default:
      break;
  }
  // Put text into Button
  myGLCD.drawString(text, xPos, yPos1);
  if (yPos2) myGLCD.drawString(text2, xPos, yPos2);
  // Pop Text Attributes from attribute stack
  popTA();
}


//
//
//  Touch Calibrate, TFT SPI only
//
//
void touch_calibrate()
{
  uint16_t calData[5];
  bool calDataOK = false;

  // check file system exists

  //  tftLedOn();
  // Open dir folder
  File dir = SPIFFS.open("/");
  // List file at root
  //  listFilesInDir(dir, 1);


  // check if calibration file exists and size is correct
  //
  if (existsFile(fnTftCal))
  {
    if (rdFile(fnTftCal, (char *)calData, 14))
      calDataOK = true;
    TOUCH_PRINTLN(F("\r\n=== TFT Touch Calibration Data ==="));
    for (int z = 0; z < 5; z++)
    {
      TOUCH_PRINT("\t");
      TOUCH_PRINTLN(calData[z]);
    }
  }
  else calibrateTftTouch = true;
  if (calDataOK && !calibrateTftTouch)
  {
    // calibration data valid
    myGLCD.setTouch(calData);
  }
  else
  {
    // data not valid, calibrate TFT Touch
    myGLCD.fillScreen(TFT_BLACK);
    myGLCD.setCursor(20, 0);
    myGLCD.setTextFont(2);
    myGLCD.setTextSize(1);
    myGLCD.setTextColor(TFT_WHITE, TFT_BLACK);
    myGLCD.println("Touch corners as indicated");
    myGLCD.setTextFont(1);
    myGLCD.println();
    myGLCD.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    for (int d = 0 ; d < 5; d++)
    {
      TOUCH_PRINTLN(calData[d]);
    }
    // Save TFT Touch calibration data
    wrtFile(fnTftCal, (char *)calData, 14);
    calibrateTftTouch = false;
    myGLCD.setTextSize(3);
    myGLCD.setTextColor(TFT_GREEN, TFT_BLACK);
    myGLCD.setTextDatum(TC_DATUM);         // Center Text
    myGLCD.drawString("Calibration", 160, 90);
    myGLCD.drawString("complete!", 160, 120);
    delay(2000);
  }
}
#endif    // TFT_SPI
