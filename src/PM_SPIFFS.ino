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

//#define DEBUG_SPIFFS
#ifdef DEBUG_SPIFFS
#define SPIFFS_PRINT(x) Serial.print(x)
#define SPIFFS_PRINTLN(x) Serial.println(x)
#define SPIFFS_PRINTF(x,y) Serial.printf(x,y)
#else
#define SPIFFS_PRINT(x)
#define SPIFFS_PRINTLN(x)
#define SPIFFS_PRINTF(x,y)
#endif

//////////////////////////////////////////////////////
//
// Initialize SPIFFS - SPI Flash File System (ESP32)
//
//
void initSPIFFS(void) {
  if (!SPIFFS.begin(true)) {
    SPIFFS_PRINTLN("An error has occurred while mounting SPIFFS");
  }
  else
  {
    SPIFFS_PRINTLN("SPIFFS mounted successfully");
    get_spiffsInfo();
    display_SPIFFS_usage();
  }
}


////////////////////////////////////////////////////////////
//
//  Display SPIFFS Usage (only if SPIFFS debug print enabled)
//
void  display_SPIFFS_usage(void)
{
  // Get all information of SPIFFS

  unsigned int totalBytes = SPIFFS.totalBytes();
  unsigned int usedBytes = SPIFFS.usedBytes();

  SPIFFS_PRINTLN(F("\r\n=== File system info ==="));
  SPIFFS_PRINTF("\tFile count: %d\r\n", spiffsInfo.entryCount);
  SPIFFS_PRINTF("\tTotal space: %d bytes\r\n", spiffsInfo.totalBytes);
  SPIFFS_PRINTF("\tUsed space: %d bytes\r\n", spiffsInfo.usedBytes);
  SPIFFS_PRINTF("\tAvailable space: %d bytes\r\n", spiffsInfo.availBytes);
}

/////////////////////////////////////////////////////////////
//
//  Get SPIFFS Information (Usage Numbers)
//
void get_spiffsInfo(void)
{
  int entryIndex = 0;
  File dir = SPIFFS.open("/");
  spiffsInfo.totalBytes = SPIFFS.totalBytes();
  spiffsInfo.usedBytes = SPIFFS.usedBytes();
  spiffsInfo.availBytes = spiffsInfo.totalBytes - spiffsInfo.usedBytes;

  while (true)
  {
    File entry =  dir.openNextFile();
    if (! entry)
    {
      spiffsInfo.entryCount = entryIndex;
      // no more files in the folder
      break;
    }
    //   spiffsInfo.dirEntry[entryIndex].name = entry.name();
    sprintf(spiffsInfo.dirEntry[entryIndex].name, "%s", entry.name());
    spiffsInfo.dirEntry[entryIndex].size = entry.size();

    SPIFFS_PRINTF("\t%s", spiffsInfo.dirEntry[entryIndex].name);
    SPIFFS_PRINTF("\t%d\n\r", spiffsInfo.dirEntry[entryIndex].size);

    //    if ( entry.isDirectory() ) spiffsInfo.dirEntry[entryIndex].isDir = true;
    //    else spiffsInfo.dirEntry[entryIndex].isDir = false;
    spiffsInfo.dirEntry[entryIndex].isDir = entry.isDirectory();
    entryIndex++;
  }
  SPIFFS_PRINTF("Entry count   %d\n\r", spiffsInfo.entryCount);
}

//////////////////////////////////////////////////////////////
//
//  Read (binary) File from SPIFFS
//
bool rdFile(const String & filename, char *data, int length)
{
  SPIFFS_PRINT("Reading file: ");
  SPIFFS_PRINTLN(filename);
  bool result = false;
  File f = SPIFFS.open(filename, "r");
  if (f) {
    if (f.readBytes(data, length) == length) result = true;
    f.close();
  }
  return (result);
}

///////////////////////////////////////////////////////////
//
//  Write (binary) File to SPIFFS
//
bool wrtFile(const String & filename, char *data, int length)
{
  bool result = false;
  SPIFFS_PRINTF("Writing file: %s\n\r", filename);
  File f = SPIFFS.open(filename, "w");
  if (f)
  {
    result = f.write((const unsigned char *)data, length);
    f.close();
  }
  return result;
}

/////////////////////////////////////////////////////////////
//
//  Delete File
//
bool deleteFile(const String & filename)
{
  return (SPIFFS.remove(filename));
  SPIFFS_PRINTF("Deleting file: %s\n\r", filename);
}

//////////////////////////////////////////////////////////////
//
//  Check if File Exists
//
bool existsFile(const String & filename)
{
  return (SPIFFS.exists(filename));
}


/////////////////////////////////////////////////////////////
//
//  List Files in SPIFFS
//
void listFilesInDir(File dir, int numTabs)
{
  while (true)
  {
    File entry =  dir.openNextFile();
    if (! entry)
    {
      // no more files in the folder
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
      SPIFFS_PRINT('\t');
    }
    SPIFFS_PRINT(entry.name());
    if (entry.isDirectory())
    {
      SPIFFS_PRINTLN("/");
      listFilesInDir(entry, numTabs + 1);
    }
    else
    {
      // display size for file, nothing for directory
      SPIFFS_PRINT("\t\t");
      SPIFFS_PRINTLN(entry.size());   //, DEC);
    }
    entry.close();
  }
}
