//#include <WiFi.h>

#define DEBUG_WIFI
#ifdef DEBUG_WIFI
#define WIFI_PRINT(x) Serial.print(x)
#define WIFI_PRINTLN(x) Serial.println(x)
#define WIFI_PRINTF(x,y) Serial.printf(x,y)
#else
#define WIFI_PRINT(x)
#define WIFI_PRINTLN(x)
#define WIFI_PRINTF(x,y)
#endif

#define DEFAULT_SSID "ssid"
#define DEFAULT_PASSWORD "password"

const char* hostName = "Pac-manClock";

//GMT Time Zone with sign
#define GMT_TIME_ZONE -8

struct WIFI_CONFIG_STRUCT {
  char ssID[32];
  char passWord[32];
} WiFiConfig;


#define MAX_NETWORK_CNT 8

struct WIFI_INFO_STRUCT {
  char  localIP[20];
  char  subnetMask[20];
  char  gatewayIP[20];
  char  macAddress[20];
} WiFiInfo;

struct SSID_Struct {
  char ssid[64];
  int rssi;
  int encryptType;
};
struct NETWORK_Struct {
  int count;
  SSID_Struct Tbl[MAX_NETWORK_CNT+2];
} WifiNetworks;



void init_WiFi(void);
void connectWiFi(void);
void disconnectWiFi(void);
String  statusWiFi(void);
int getWiFiQuality(void);
