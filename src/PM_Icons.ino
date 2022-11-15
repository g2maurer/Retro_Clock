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

//#define DEBUG_ICONS
#ifdef DEBUG_ICONS
#define ICONS_PRINT(x) Serial.print(x)
#define ICONS_PRINTLN(x) Serial.println(x)
#define ICONS_PRINTF(x,y) Serial.printf(x,y)
#else
#define ICONS_PRINT(x)
#define ICONS_PRINTLN(x)
#define ICONS_PRINTF(x,y)
#endif

/////////////////////////////////////////////////////////////////
//
//  Make a Mirror image of the source Icon into destination Icon.
//
void mirrorIcon(unsigned short * srcIcon, unsigned short * destIcon)
{
  for (int i = 0; i < 784; i += 28) // row
  {
    for (int j = 0; j < 28; j++) // column
    {
      destIcon[j + i] = srcIcon[(28 - j) + i];
    }
  }
}

//////////////////////////////////////////////////
//
// Convert Left facing Icon to Up facing Icon
//    (rotate clockwise 90 degrees)
//
void rotateIconCW(unsigned short * srcIcon, unsigned short * destIcon)
{
  for (int i = 0; i < 28; i++)    // source row index
  {
    int k = i * 28;
    for (int j = 0; j < 28; j++)    // source column index
    {
      destIcon[27 + (28 * j) - i] = srcIcon[j + k];
    }
  }
}

//////////////////////////////////////////////////////////
//
// Convert Left facing Icon to Down facing Icon
//    rotate counter (anti) clockwise 90 degrees
//
void rotateIconCCW(unsigned short * srcIcon, unsigned short * destIcon)
{
  for (int i = 0; i < 28; i++)
  {
    int k = i * 28;
    for (int j = 0; j < 28; j++)
    {
      destIcon[756 - (28 * j) + i] = srcIcon[j + k];
    }
  }
}

/////////////////////////////////////////////
//
//  Swap bytes in a short (16 bit)
//
#define BYTESWAP16(z) ((unsigned short)((z&0xff)<<8)|((z>>8)&0xff))


//////////////////////////////////////////////////////////
//
//  Load Pac-man ICON files from SPIFFS
//
void loadIcons(void)
{
  if (clockConfig.mspacman) loadMsPacmanIcons();
  else loadPacmanIcons();
}

////////////////////////////////////////////////////////
//
//  Load Ghosts ICON files from SPIFFS
//
void loadGhostsIcons(void)
{
  if (!existsFile(fnBlueGhost))
    ICONS_PRINTLN("Missing Blue Ghost Icon file");
  else if (rdFile(fnBlueGhost, (char *)&blueGhost, ICON_SIZE))
    ICONS_PRINTLN("blueGhost file read");
  else
    ICONS_PRINTLN("Failed to read blueGhost file");

  if (!existsFile(fnRedGhostLeft))
    ICONS_PRINTLN("Missing Red Ghost Eyes Left Icon file");
  else if (rdFile(fnRedGhostLeft, (char *)&redGhostEyesLeft, ICON_SIZE))
    ICONS_PRINTLN("redGhost Eyes Left file read");
  else
    ICONS_PRINTLN("Failed to read redGhostLeft file");

  if (!existsFile(fnRedGhostRight))
    ICONS_PRINTLN("Missing Red Ghost Eyes Right Icon file");
  else if (rdFile(fnRedGhostRight, (char *)&redGhostEyesRight, ICON_SIZE))
    ICONS_PRINTLN("redGhost Eyes Right file read");
  else
    ICONS_PRINTLN("Failed to read redGhostRight file");

  if (!existsFile(fnRedGhostUp))
    ICONS_PRINTLN("Missing Red Ghost Eyes Up Icon file");
  else if (rdFile(fnRedGhostUp, (char *)&redGhostEyesUp, ICON_SIZE))
    ICONS_PRINTLN("redGhost Eyes Up file read");
  else
    ICONS_PRINTLN("Failed to read redGhostUp file");

  if (!existsFile(fnRedGhostDown))
    ICONS_PRINTLN("Missing Red Ghost Eyes Down Icon file");
  else if (rdFile(fnRedGhostDown, (char *)&redGhostEyesDown, ICON_SIZE))
    ICONS_PRINTLN("redGhost Eyes Down file read");
  else
    ICONS_PRINTLN("Failed to read redGhostDown file");

  if (!existsFile(fnFruit))
    ICONS_PRINTLN("Missing Fruit Icon file");
  else if (rdFile(fnFruit, (char *)&fruitIcon, ICON_SIZE))
    ICONS_PRINTLN("Fruit Icon file read");
  else
    ICONS_PRINTLN("Failed to read Fruit Icon file");
}


///////////////////////////////////////////////////////
//
//  Load Mr. Pac-man Icons (from files into memory)
//
void loadPacmanIcons(void)
{
  if (!existsFile(fnPacClosed))
    ICONS_PRINTLN("Missing Pacman Closed Mouth Icon file");
  else if (rdFile(fnPacClosed, (char *)&pacmanClosedLeft, ICON_SIZE))
    ICONS_PRINTLN("Pacman Closed Mouth Icon file read");
  else
    ICONS_PRINTLN("Failed to read Pacman Closed Mouth Icon file");
  rotateIconCCW(pacmanClosedLeft, pacmanClosedDown);    // Left to Down
  rotateIconCW(pacmanClosedLeft, pacmanClosedUp);      // Left to Up
  mirrorIcon(pacmanClosedLeft, pacmanClosedRight);      // Left to Right

  if (!existsFile(fnPacHalf))
    ICONS_PRINTLN("Missing Pacman Half-Open Mouth Icon file");
  else if (rdFile(fnPacHalf, (char *)&pacmanHalfOpenLeft, ICON_SIZE))
    ICONS_PRINTLN("Pacman Half-Open Mouth Icon file read");
  else
    ICONS_PRINTLN("Failed to read Pacman Half-Open Mouth Icon file");
  rotateIconCCW(pacmanHalfOpenLeft, pacmanHalfOpenDown);    // Left to Down
  rotateIconCW(pacmanHalfOpenLeft, pacmanHalfOpenUp);      // Left to Up
  mirrorIcon(pacmanHalfOpenLeft, pacmanHalfOpenRight);      // Left to Right


  if (!existsFile(fnPacOpen))
    ICONS_PRINTLN("Missing Pacman Open Mouth Icon file");
  else if (rdFile(fnPacOpen, (char *)&pacmanOpenLeft, ICON_SIZE))
    ICONS_PRINTLN("Pacman Open Mouth Icon file read");
  else
    ICONS_PRINTLN("Failed to read Pacman Open Mouth Icon file");
  rotateIconCCW(pacmanOpenLeft, pacmanOpenDown);    // Left to Down
  rotateIconCW(pacmanOpenLeft, pacmanOpenUp);      // Left to Up
  mirrorIcon(pacmanOpenLeft, pacmanOpenRight);      // Left to Right
}


//////////////////////////////////////////////////////////
//
//  Load Ms Pac-man Icons (from files into memory)
//
void loadMsPacmanIcons(void)
{
  if (!existsFile(fnMsPacClosed))
    ICONS_PRINTLN("Missing Ms Pacman Closed Mouth Icon file");
  else if (rdFile(fnMsPacClosed, (char *)&pacmanClosedLeft, ICON_SIZE))
    ICONS_PRINTLN("Ms Pacman Closed Mouth Icon file read");
  else
    ICONS_PRINTLN("Failed to read  Ms Pacman Closed Mouth Icon file");
  rotateIconCCW(pacmanClosedLeft, pacmanClosedDown);    // Left to Down
  rotateIconCW(pacmanClosedLeft, pacmanClosedUp);      // Left to Up
  mirrorIcon(pacmanClosedLeft, pacmanClosedRight);      // Left to Right

  if (!existsFile(fnMsPacHalf))
    ICONS_PRINTLN("Missing Ms Pacman Half-Open Mouth Icon file");
  else if (rdFile(fnMsPacHalf, (char *)&pacmanHalfOpenLeft, ICON_SIZE))
    ICONS_PRINTLN("Ms Pacman Half-Open Mouth Icon file read");
  else
    ICONS_PRINTLN("Failed to read  ms Pacman Half-Open Mouth Icon file");
  rotateIconCCW(pacmanHalfOpenLeft, pacmanHalfOpenDown);    // Left to Down
  rotateIconCW(pacmanHalfOpenLeft, pacmanHalfOpenUp);      // Left to Up
  mirrorIcon(pacmanHalfOpenLeft, pacmanHalfOpenRight);      // Left to Right

  if (!existsFile(fnMsPacOpen))
    ICONS_PRINTLN("Missing Ms Pacman Open Mouth Icon file");
  else if (rdFile(fnMsPacOpen, (char *)&pacmanOpenLeft, ICON_SIZE))
    ICONS_PRINTLN("Ms Pacman Open Mouth Icon file read");
  else
    ICONS_PRINTLN("Failed to read  ms Pacman Open Mouth Icon file");
  rotateIconCCW(pacmanOpenLeft, pacmanOpenDown);    // Left to Down
  rotateIconCW(pacmanOpenLeft, pacmanOpenUp);      // Left to Up
  mirrorIcon(pacmanOpenLeft, pacmanOpenRight);      // Left to Right
}
