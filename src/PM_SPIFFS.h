#include "SPIFFS.h"

struct SPIFFS_DIR_STRUCT {
  int size;
  bool  isDir;
  char  name[32];
};

struct SPIFFS_INFO_STRUCT {
  unsigned int totalBytes;
  unsigned int usedBytes;
  unsigned int availBytes;
  int entryCount;
  struct SPIFFS_DIR_STRUCT dirEntry[32];
} spiffsInfo;
