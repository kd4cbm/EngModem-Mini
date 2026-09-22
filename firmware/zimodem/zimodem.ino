/*
   Copyright 2016-2026 Bo Zimmerman

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. 
*/
//#define TCP_SND_BUF                     4 * TCP_MSS
#define ZIMODEM_VERSION "4.0.3"
const char compile_date[] = __DATE__ " " __TIME__;
#define DEFAULT_NO_DELAY true
#define null 0


#define INCLUDE_IRCC true
#define INCLUDE_SD_SHELL true  /* ESP32 only, requires SPI SD card interface */
#define INCLUDE_CBMMODEM true  // ESP32 only, 1650, 1660, 1670, and Pulse Dialing support
#define INCLUDE_PING true
#define INCLUDE_SSH true   // ESP32 only, adds SSH client
#define INCLUDE_HOSTCM true // requires sd shell
#define INCLUDE_FTP true
#define INCLUDE_COMET64 true // requires sd shell
#define INCLUDE_OTH_UPDATES true // comment out if you make incompatible firmware
#define INCLUDE_SLIP true
#define INCLUDE_PPP true
#define INCLUDE_VFD true  /* ESP32-S3 only, requires the direct-parallel VFD wiring in ESP32S3_PIN_MAP.md */
#define SUPPORT_LED_PINS true // this board has 3 direct-drive status LEDs wired (AA/HS/OH) - see ENGMODEM_MINI_BOARD below
//#define INCLUDE_CMDRX16 true // enable this if you are David or Kevin
//#define USE_DEVUPDATER true // only enable this if your name is Bo

// EngModem "Mini" custom PCB: reuses the ESP32-S3-16R8 DevKitC-1 pin map
// below (ARDUINO_ESP32S3_DEV block) since the wiring matches it exactly,
// but it is NOT that dev board - it has no onboard addressable RGB LED on
// GPIO38 (that pin is a general-purpose expansion header pin instead), and
// it has 3 direct-drive status LEDs on GPIO10/11/12 that the dev board
// doesn't have at all. Gates the two spots below where "any ESP32-S3-16R8
// DevKitC-1 build" and "this specific custom board" need different behavior.
#define ENGMODEM_MINI_BOARD true

// Figure out whether we are building for ESP8266 or ESP32
#if defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_ARCH_ESP32)
# define ZIMODEM_ESP32
#elif defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32C3_DEV)
# define ZIMODEM_ESP32
#elif defined(ESP32)
# define ZIMODEM_ESP32
#elif defined(ARDUINO_ESP320)
# define ZIMODEM_ESP32
#elif defined(ARDUINO_NANO32)
# define ZIMODEM_ESP32
#elif defined(ARDUINO_LoLin32)
# define ZIMODEM_ESP32
#elif defined(ARDUINO_ESPea32)
# define ZIMODEM_ESP32
#elif defined(ARDUINO_QUANTUM)
# define ZIMODEM_ESP32
#else
# define ZIMODEM_ESP8266
# undef INCLUDE_SSH
# undef INCLUDE_SD_SHELL
# undef INCLUDE_CBMMODEM
#endif

#if SUPPORT_LED_PINS
# if defined(ARDUINO_ESP32S3_DEV) && defined(ENGMODEM_MINI_BOARD)
    // EngModem Mini board's real wiring: 3 direct-drive status LEDs,
    // silkscreened AA/HS/OH (Hayes-style: auto-answer, high-speed, off-hook).
    // Reuses the existing AA/HS/WIFI framework's pin slot for OH - the
    // identifier is still DEFAULT_PIN_WIFI, but on this board it's driven
    // as a true off-hook signal (lit while any connection is open - telnet,
    // SSH, FTP, IRC, print - regardless of protocol), not WiFi association
    // status. See checkOpenConnections() for the actual drive logic.
#   define DEFAULT_PIN_AA GPIO_NUM_10
#   define DEFAULT_PIN_HS GPIO_NUM_11
#   define DEFAULT_PIN_WIFI GPIO_NUM_12
# elif defined(GPIO_NUM_0)
#   define DEFAULT_PIN_AA GPIO_NUM_35
#   define DEFAULT_PIN_HS GPIO_NUM_34
#   define DEFAULT_PIN_WIFI GPIO_NUM_26
# else
#   define DEFAULT_PIN_AA 35
#   define DEFAULT_PIN_HS 34
#   define DEFAULT_PIN_WIFI 26
# endif
# define DEFAULT_HS_BAUD 38400
# ifdef ENGMODEM_MINI_BOARD
   // On the EngModem Mini the AA/HS/OH LEDs are wired GPIO -> 330R -> LED anode, cathode -> GND,
   // so they light when the pin is driven HIGH. The dev-board defaults below are active-LOW
   // and left these three LEDs inverted (lit while idle, dark while "active").
#  define DEFAULT_AA_ACTIVE HIGH
#  define DEFAULT_AA_INACTIVE LOW
#  define DEFAULT_HS_ACTIVE HIGH
#  define DEFAULT_HS_INACTIVE LOW
#  define DEFAULT_WIFI_ACTIVE HIGH
#  define DEFAULT_WIFI_INACTIVE LOW
# else
#  define DEFAULT_AA_ACTIVE LOW
#  define DEFAULT_AA_INACTIVE HIGH
#  define DEFAULT_HS_ACTIVE LOW
#  define DEFAULT_HS_INACTIVE HIGH
#  define DEFAULT_WIFI_ACTIVE LOW
#  define DEFAULT_WIFI_INACTIVE HIGH
# endif
#endif

#define DEFAULT_BAUD_RATE 1200
#define DEFAULT_SERIAL_CONFIG SERIAL_8N1
#define RX_BUFFER_SIZE 4096
/*
 * Unused pins on WROOM32:
 * 2, 20, 21, 22.   36,39 (sensor)
 */

#ifdef ZIMODEM_ESP32
# define PIN_FACTORY_RESET GPIO_NUM_0
# define DEFAULT_FCT FCT_DISABLED
# if INCLUDE_CMDRX16                   /* Configuration for the Commander X16 I/O Card */
#  undef INCLUDE_SD_SHELL
#  undef DEFAULT_FCT
#  define DEFAULT_FCT FCT_RTSCTS
#  undef DEFAULT_BAUD_RATE
#  define DEFAULT_BAUD_RATE 115200
#  define DEFAULT_PIN_DCD GPIO_NUM_22
#  define DEFAULT_PIN_CTS GPIO_NUM_19
#  define DEFAULT_PIN_RTS GPIO_NUM_21
#  define DEFAULT_PIN_RI GPIO_NUM_18
#  define DEFAULT_PIN_DSR GPIO_NUM_23
#  define DEFAULT_PIN_DTR GPIO_NUM_25
//#  define DEFAULT_PIN_OPA 26
#  define DEFAULT_PIN_OPB GPIO_NUM_27
#  define DEFAULT_PIN_OTH GPIO_NUM_26 // stream/command mode flag
#  define DEFAULT_PIN_TXD GPIO_NUM_32
#  define DEFAULT_PIN_RXD GPIO_NUM_33
#  define DEFAULT_PIN_SND GPIO_NUM_4
# elif defined(ARDUINO_ESP32C3_DEV)   /* Configuration for the Esp32C3 4MB Dev Board */
#  undef INCLUDE_SD_SHELL
#  undef PIN_FACTORY_RESET
#  define DEFAULT_PIN_DCD GPIO_NUM_8
#  define DEFAULT_PIN_CTS GPIO_NUM_3 // espdev rts pin
#  define DEFAULT_PIN_RTS GPIO_NUM_2 // espdev cts pin
#  define DEFAULT_PIN_RI GPIO_NUM_1
#  define DEFAULT_PIN_DSR GPIO_NUM_4
#  define DEFAULT_PIN_SND GPIO_NUM_6
#  define DEFAULT_PIN_OTH GPIO_NUM_7 // pulse pin
#  define DEFAULT_PIN_DTR GPIO_NUM_5
# elif defined(ARDUINO_ESP32S3_DEV) /* Configuration for the Esp32S3-16R8 DevKitC-1 -
                                        pin map verified against Espressif's J1/J3
                                        header tables; see ESP32S3_PIN_MAP.md at the
                                        repo root for the full rationale. */
#  define DEFAULT_PIN_DCD GPIO_NUM_5
# ifdef ENGMODEM_MINI_BOARD
   // RTS/CTS are wired DCE-style on the EngModem Mini PCB: GPIO17 is fed by the
   // MAX3237's ROUT2 (the PC's RTS, from DB9 pin 7) and GPIO18 drives DIN3 (out to
   // the PC's CTS, DB9 pin 8). Zimodem's UART flow control needs CTS as an INPUT and
   // RTS as an OUTPUT, so on this board those roles sit on the opposite GPIOs from
   // the dev-board layout in the #else branch. Without this swap the firmware
   // drives GPIO17 (which ROUT2 also drives) and reads a floating GPIO18.
#  define DEFAULT_PIN_CTS GPIO_NUM_17 // input: PC's RTS via MAX3237 ROUT2
#  define DEFAULT_PIN_RTS GPIO_NUM_18 // output: to PC's CTS via MAX3237 DIN3
# else
#  define DEFAULT_PIN_CTS GPIO_NUM_18 // espdev rts pin
#  define DEFAULT_PIN_RTS GPIO_NUM_17 // espdev cts pin
# endif
#  define DEFAULT_PIN_RI GPIO_NUM_6
#  define DEFAULT_PIN_DSR GPIO_NUM_7  // moved from GPIO9 to close the gap in the J1 run
#  define DEFAULT_PIN_SND -1
#  define DEFAULT_PIN_OTH -1 // pulse pin
#  define DEFAULT_PIN_DTR GPIO_NUM_4
#  define DEFAULT_PIN_TXD GPIO_NUM_16
#  define DEFAULT_PIN_RXD GPIO_NUM_15
#  define DEFAULT_PIN_SD_CS   GPIO_NUM_13 // moved from GPIO14 - GPIO14 is now SD MOSI
#  define DEFAULT_PIN_SD_MOSI GPIO_NUM_14
#  define DEFAULT_PIN_SD_MISO GPIO_NUM_1
#  define DEFAULT_PIN_SD_SCK  GPIO_NUM_2
   // VFD driven directly over its 4-bit parallel bus - no I2C bridge.
   // R/W is hardwired to GND on the VFD board itself, so it's not an MCU pin.
#  define DEFAULT_PIN_VFD_RS  GPIO_NUM_42
#  define DEFAULT_PIN_VFD_E   GPIO_NUM_41
#  define DEFAULT_PIN_VFD_DB4 GPIO_NUM_40
#  define DEFAULT_PIN_VFD_DB5 GPIO_NUM_39
   // GPIO47/48 are NOT general-purpose on this N16R8 (octal PSRAM) module -
   // they're the SUBSPI differential clock pair used to reach the in-package
   // flash/PSRAM at its 1.8V domain, confirmed reserved in Espressif's own
   // docs. DB7 was there originally and caused a hard no-boot the moment it
   // was wired up; DB6 landed there in the last revision while fixing an
   // unrelated GPIO38/LED conflict, which was an equally bad landing spot
   // that just hadn't been wired up yet to fail visibly. Both now moved to
   // J1's GPIO8/GPIO9 (freed earlier when the VFD I2C bridge was removed) -
   // this means DB6/DB7 physically move to J1, separate from RS/E/DB4/DB5
   // on J3, since J3 has no remaining safe pins after this correction.
#  define DEFAULT_PIN_VFD_DB6 GPIO_NUM_8
#  define DEFAULT_PIN_VFD_DB7 GPIO_NUM_9
# else                                    /* Configuration for standard ESP32 4 & 8MB boards */
#  define DEFAULT_PIN_DCD GPIO_NUM_14
#  define DEFAULT_PIN_CTS GPIO_NUM_13
#  define DEFAULT_PIN_RTS GPIO_NUM_15 // unused?
#  define DEFAULT_PIN_RI GPIO_NUM_32
#  define DEFAULT_PIN_DSR GPIO_NUM_12
#  define DEFAULT_PIN_SND GPIO_NUM_25
#  define DEFAULT_PIN_OTH GPIO_NUM_4 // pulse pin
#  define DEFAULT_PIN_DTR GPIO_NUM_27
# endif
# ifndef DEFAULT_PIN_VFD_RS
#  undef INCLUDE_VFD  // only the ESP32-S3-16R8 DevKitC-1 pin block above defines VFD pins
# endif
# define debugPrintf DBSerial.printf
# define SerialConfig uint32_t
# define UART_CONFIG_MASK 0x8000000
# define UART_NB_BIT_MASK      0B00001100 | UART_CONFIG_MASK
# define UART_NB_BIT_5         0B00000000 | UART_CONFIG_MASK
# define UART_NB_BIT_6         0B00000100 | UART_CONFIG_MASK
# define UART_NB_BIT_7         0B00001000 | UART_CONFIG_MASK
# define UART_NB_BIT_8         0B00001100 | UART_CONFIG_MASK
# define UART_PARITY_MASK      0B00000011
# define UART_PARITY_NONE      0B00000000
# define UART_NB_STOP_BIT_MASK 0B00110000
# define UART_NB_STOP_BIT_0    0B00000000
# define UART_NB_STOP_BIT_1    0B00010000
# define UART_NB_STOP_BIT_15   0B00100000
# define UART_NB_STOP_BIT_2    0B00110000
# define preEOLN(...)
//# define preEOLN serial.prints
# define echoEOLN(...) serial.prints(EOLN)
//# define echoEOLN serial.write
//# define HARD_DCD_HIGH 1
//# define HARD_DCD_LOW 1
#else  // ESP-8266, e.g. ESP-01, ESP-12E
# define DEFAULT_PIN_DSR 13
# define DEFAULT_PIN_DTR 12
# define DEFAULT_PIN_RI 14
# define DEFAULT_PIN_RTS 4
# define DEFAULT_PIN_CTS 5 // is 0 for ESP-01
# define DEFAULT_PIN_DCD 2
# define DEFAULT_PIN_OTH -1 // pulse pin
# define DEFAULT_FCT FCT_DISABLED
# define debugPrintf doNothing //Serial.printf
# define preEOLN(...)
# define echoEOLN(...) serial.prints(EOLN)
#endif

# define DEFAULT_DCD_ACTIVE  LOW
# define DEFAULT_DCD_INACTIVE  HIGH
# define DEFAULT_CTS_ACTIVE  LOW
# define DEFAULT_CTS_INACTIVE  HIGH
# define DEFAULT_RTS_ACTIVE  LOW
# define DEFAULT_RTS_INACTIVE  HIGH
# define DEFAULT_RI_ACTIVE  LOW
# define DEFAULT_RI_INACTIVE  HIGH
# define DEFAULT_DSR_ACTIVE  LOW
# define DEFAULT_DSR_INACTIVE  HIGH
# define DEFAULT_DTR_ACTIVE  LOW
# define DEFAULT_DTR_INACTIVE  HIGH
# define DEFAULT_OTH_ACTIVE  LOW
# define DEFAULT_OTH_INACTIVE  HIGH

#define MAX_PIN_NO 50
#define INTERNAL_FLOW_CONTROL_DIV 380
#define DEFAULT_RECONNECT_DELAY 60000
#define MAX_RECONNECT_DELAY 1800000

class ZMode
{
  public:
    virtual void serialIncoming();
    virtual void loop();
};

#include "pet2asc.h"
#include "rt_clock.h"
#include "filelog.h"
#include "serout.h"
#include "connSettings.h"
#include "wificlientnode.h"
#include "stringstream.h"
#include "phonebook.h"
#include "wifiservernode.h"
#include "zstream.h"
#include "proto_http.h"
#include "proto_ftp.h"
#include "vfd.h"
#include "zconfigmode.h"
#include "zcommand.h"
#include "zprint.h"

#if INCLUDE_SD_SHELL
#  if INCLUDE_HOSTCM
#    include "zhostcmmode.h"
#  endif
#  if INCLUDE_FTP
#    include "zcomet64mode.h"
#  endif
#  include "proto_xmodem.h"
#  include "proto_zmodem.h"
#  include "proto_punter.h"
#  include "proto_kermit.h"
#  include "zbrowser.h"
#endif
#if INCLUDE_SLIP
#  include "zslipmode.h"
#endif
#if INCLUDE_PPP
#  include "zpppmode.h"
#endif
#if INCLUDE_IRCC
#  include "zircmode.h"
#endif

static WiFiClientNode *conns = null;
static WiFiServerNode *servs = null;
static PhoneBookEntry *phonebook = null;
static bool pinSupport[MAX_PIN_NO];
static int pinCache[MAX_PIN_NO];
static String termType = DEFAULT_TERMTYPE;
static String busyMsg = DEFAULT_BUSYMSG;
static bool debugUart = false;

static OpModes altOpMode = OPMODE_NONE;
static ZMode *currMode = null;
static ZStream streamMode;
static ZCommand commandMode;
static ZPrint printMode;
static ZConfig configMode;
static RealTimeClock zclock(0);
#if INCLUDE_SD_SHELL
#  if INCLUDE_HOSTCM
     static ZHostCMMode hostcmMode;
#  endif
#  if INCLUDE_FTP
     static ZComet64Mode comet64Mode;
#  endif
   static ZBrowser browseMode;
#endif
#if INCLUDE_SLIP
   static ZSLIPMode slipMode;
#endif
#if INCLUDE_PPP
   static ZPPPMode pppMode;
#endif
#if INCLUDE_IRCC
   static ZIRCMode ircMode;
#endif

enum BaudState
{
  BS_NORMAL,
  BS_SWITCH_TEMP_NEXT,
  BS_SWITCHED_TEMP,
  BS_SWITCH_NORMAL_NEXT
};

static String wifiSSI;
static String wifiPW;
static String hostname;
static IPAddress *staticIP = null;
static IPAddress *staticDNS = null;
static IPAddress *staticGW = null;
static IPAddress *staticSN = null;
static unsigned long lastConnectAttempt = 0;
static unsigned long nextReconnectDelay = 0; // zero means don't attempt reconnects
static SerialConfig serialConfig = DEFAULT_SERIAL_CONFIG;
static int baudRate=DEFAULT_BAUD_RATE;
// Exposed read-only for the VFD status display (vfd.ino) - reflects whether
// the SD shell is actually usable, decided once at startup by initSDShell()
// (zbrowser.ino), not a live-polled card presence check.
static bool sdShellAvailable = false;
static int calcDequeSize(int forBaudRate)
{
  // Unclamped, this grows past SER_BUFSIZE at high baud rates (e.g. 304 at
  // 115200 vs SER_BUFSIZE=127/128), making the "(SER_BUFSIZE -
  // availableForWrite()) < dequeSize" throttle in serialOutDeque() trivially
  // always-true -- i.e. a silent no-op -- for every baud rate this project
  // has ever offered above ~48260 baud. That let one serialOutDeque() call
  // drain the whole TX ring buffer in a single pass instead of trickling it,
  // which under real hardware RTS/CTS flow control (see the MAX3237/generic-
  // level-shifter bug report) queued far more data at once than the far end
  // could keep up with, producing exactly the delayed/misaligned multi-
  // response bursts observed in testing. Clamping keeps the throttle
  // meaningful at any baud rate.
  int size = 1+(forBaudRate/INTERNAL_FLOW_CONTROL_DIV);
  if(size > SER_BUFSIZE-1)
    size = SER_BUFSIZE-1;
  return size;
}
static int dequeSize=calcDequeSize(DEFAULT_BAUD_RATE);
static BaudState baudState = BS_NORMAL; 
static unsigned long resetPushTimer=0;
static int tempBaud = -1; // -1 do nothing
static unsigned int plussesInARow = 0;
static unsigned long lastInputTimeMs = 0;
static int dcdStatus = DEFAULT_DCD_INACTIVE;
static int pinDCD = DEFAULT_PIN_DCD;
static int pinCTS = DEFAULT_PIN_CTS;
static int pinRTS = DEFAULT_PIN_RTS;
static int pinDSR = DEFAULT_PIN_DSR;
static int pinDTR = DEFAULT_PIN_DTR;
static int pinOTH = DEFAULT_PIN_OTH;
static int pinRI = DEFAULT_PIN_RI;
static int dcdActive = DEFAULT_DCD_ACTIVE;
static int dcdInactive = DEFAULT_DCD_INACTIVE;
static int ctsActive = DEFAULT_CTS_ACTIVE;
static int ctsInactive = DEFAULT_CTS_INACTIVE;
static int rtsActive = DEFAULT_RTS_ACTIVE;
static int rtsInactive = DEFAULT_RTS_INACTIVE;
static int riActive = DEFAULT_RI_ACTIVE;
static int riInactive = DEFAULT_RI_INACTIVE;
static int dtrActive = DEFAULT_DTR_ACTIVE;
static int dtrInactive = DEFAULT_DTR_INACTIVE;
static int dsrActive = DEFAULT_DSR_ACTIVE;
static int dsrInactive = DEFAULT_DSR_INACTIVE;
static int othActive = DEFAULT_OTH_ACTIVE;
static int othInactive = DEFAULT_OTH_INACTIVE;

static int getDefaultCtsPin()
{
  return DEFAULT_PIN_CTS;
}

static void doNothing(const char* format, ...) 
{
}

static void s_pinWrite(uint8_t pinNo, uint8_t value)
{
  if(pinSupport[pinNo])
  {
    pinCache[pinNo] = value;
    digitalWrite(pinNo, value);
  }
}

static void setHostName(const char *hname)
{
#ifdef ZIMODEM_ESP32
  #ifdef TCPIP_ADAPTER_IF_STA
      tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hname);
  #else
      WiFi.setHostname(hname);
  #endif
#else
      WiFi.hostname(hname);
#endif
}

static void setNewStaticIPs(IPAddress *ip, IPAddress *dns, IPAddress *gateWay, IPAddress *subNet)
{
  if(staticIP != null)
    free(staticIP);
  staticIP = ip;
  if(staticDNS != null)
    free(staticDNS);
  staticDNS = dns;
  if(staticGW != null)
    free(staticGW);
  staticGW = gateWay;
  if(staticSN != null)
    free(staticSN);
  staticSN = subNet;
}

static bool connectWifi(const char* ssid, const char* password, IPAddress *ip, IPAddress *dns, IPAddress *gateWay, IPAddress *subNet)
{
  while(WiFi.status() == WL_CONNECTED)
  {
    WiFi.disconnect();
    delay(100);
    yield();
  }
#ifndef ZIMODEM_ESP32
  if(hostname.length() > 0)
    setHostName(hostname.c_str());
#endif
  WiFi.mode(WIFI_STA);
  if((ip != null)&&(gateWay != null)&&(dns != null)&&(subNet!=null))
  {
    if(!WiFi.config(*ip,*gateWay,*subNet,*dns))
      return false;
  }
  WiFi.begin(ssid, password);
#if defined(ARDUINO_MAKERGO_C3_SUPERMINI) || defined(ARDUINO_NOLOGO_ESP32C3_SUPER_MINI)
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
  if(hostname.length() > 0)
    setHostName(hostname.c_str());
  bool amConnected = (WiFi.status() == WL_CONNECTED) && (strcmp(WiFi.localIP().toString().c_str(), "0.0.0.0")!=0);
  int WiFiCounter = 0;
  while ((!amConnected) && (WiFiCounter < 20))
  {
    WiFiCounter++;
    if(!amConnected)
      delay(500);
    amConnected = (WiFi.status() == WL_CONNECTED) && (strcmp(WiFi.localIP().toString().c_str(), "0.0.0.0")!=0);
  }
  lastConnectAttempt = millis();
  if(lastConnectAttempt == 0)  // it IS possible for millis() to be 0, but we need to ignore it.
    lastConnectAttempt = 1; // 0 is a special case, so skip it

  if(!amConnected)
  {
    nextReconnectDelay = 0; // assume no retry is desired.. let the caller set it up, as it could be bad PW
    WiFi.disconnect();
  }
  else
    nextReconnectDelay = DEFAULT_RECONNECT_DELAY; // if connected, we always want to try reconns in the future

#if SUPPORT_LED_PINS
# ifndef ENGMODEM_MINI_BOARD
  s_pinWrite(DEFAULT_PIN_WIFI,(WiFi.status() == WL_CONNECTED)?DEFAULT_WIFI_ACTIVE:DEFAULT_WIFI_INACTIVE);
# endif
  // On ENGMODEM_MINI_BOARD, DEFAULT_PIN_WIFI is the board's "OH" (off-hook)
  // LED, not a WiFi-association indicator - left untouched here and driven
  // instead by checkOpenConnections() based on actual open connections
  // (telnet/SSH/FTP/etc, not WiFi association). See that function.
#endif
  if(WiFi.status() == WL_CONNECTED)
    debugPrintf("Connected to %s with IP %s.\r\n",ssid,WiFi.localIP().toString().c_str());
  return (WiFi.status() == WL_CONNECTED);
}

static void checkBaudChange()
{
  switch(baudState)
  {
    case BS_SWITCH_TEMP_NEXT:
      changeBaudRate(tempBaud);
      baudState = BS_SWITCHED_TEMP;
      break;
    case BS_SWITCH_NORMAL_NEXT:
      changeBaudRate(baudRate);
      baudState = BS_NORMAL;
      break;
    default:
      break;
  }
}

static void changeBaudRate(int baudRate)
{
  flushSerial(); // blocking, but very very necessary
  delay(500); // give the client half a sec to catch up
  logPrintfln("Baud change to %d.",baudRate);
  dequeSize=calcDequeSize(baudRate);
  debugPrintf("Baud %d, Deque constant now: %d\r\n",baudRate,dequeSize);
#ifdef ZIMODEM_ESP32
  HWSerial.updateBaudRate(baudRate);
#else
  HWSerial.begin(baudRate, serialConfig);  //Change baud rate
#endif
#if SUPPORT_LED_PINS
  s_pinWrite(DEFAULT_PIN_HS,(baudRate>=DEFAULT_HS_BAUD)?DEFAULT_HS_ACTIVE:DEFAULT_HS_INACTIVE);
#endif  
}

static void changeSerialConfig(SerialConfig conf)
{
  flushSerial(); // blocking, but very very necessary
  delay(500); // give the client half a sec to catch up
  debugPrintf("Config changing to %dbps, %d.\r\n",baudRate,(int)conf);
  dequeSize=calcDequeSize(baudRate);
  debugPrintf("Deque constant now: %d\r\n",dequeSize);
# ifdef DEFAULT_PIN_RXD
    debugPrintf("Using BPS %d, RXD %d, TXD %d\r\n",baudRate,DEFAULT_PIN_RXD, DEFAULT_PIN_TXD);
    HWSerial.begin(baudRate, conf, DEFAULT_PIN_RXD, DEFAULT_PIN_TXD);
# else
    HWSerial.begin(baudRate, conf);  //Change baud rate
# endif
  debugPrintf("Config changed.\r\n");
}

static int checkOpenConnections()
{
  int num=WiFiClientNode::getNumOpenWiFiConnections();
  if(num == 0)
  {
    if((dcdStatus == dcdActive)
    &&(dcdStatus != dcdInactive))
    {
      dcdStatus = dcdInactive;
      s_pinWrite(pinDCD,dcdStatus);
      if(baudState == BS_SWITCHED_TEMP)
        baudState = BS_SWITCH_NORMAL_NEXT;
      if(currMode == &commandMode)
        clearSerialOutBuffer();
    }
#if SUPPORT_LED_PINS && defined(ENGMODEM_MINI_BOARD)
    // Off-hook LED: no open connections of any kind (telnet/SSH/FTP/IRC/print).
    s_pinWrite(DEFAULT_PIN_WIFI,DEFAULT_WIFI_INACTIVE);
#endif
  }
  else
  {
    if((dcdStatus == dcdInactive)
    &&(dcdStatus != dcdActive))
    {
      dcdStatus = dcdActive;
      s_pinWrite(pinDCD,dcdStatus);
      if((tempBaud > 0) && (baudState == BS_NORMAL))
        baudState = BS_SWITCH_TEMP_NEXT;
    }
#if SUPPORT_LED_PINS && defined(ENGMODEM_MINI_BOARD)
    // Off-hook LED: at least one open connection - protocol-agnostic, since
    // every connection type (telnet/SSH/FTP/IRC/print) registers as a plain
    // WiFiClientNode and is counted by getNumOpenWiFiConnections() above.
    s_pinWrite(DEFAULT_PIN_WIFI,DEFAULT_WIFI_ACTIVE);
#endif
  }
  return num;
}

static int processPlusPlusPlus(uint8_t c)
{
  if(c<0)
    return 0;
  int plusOut = 0;
  if(c == commandMode.EC)
  {
    bool timeout = (millis()-lastInputTimeMs)>900;
    if(plussesInARow==0)
    {
      if(timeout)
         plussesInARow=1; // it begins!
      // else got a +, but too quick after last char, so keep at 0
    }
    else
    if(!timeout) // quick PLUS
    {
      if(plussesInARow<3)
        plussesInARow++;
      else
      {
        plusOut = plussesInARow; // sur-plus, so reject
        plussesInARow=0; // spamming plusses clears!
      }
    }
    else // plus long after timeout
    {
      plusOut = plussesInARow;
      plussesInARow=1;
    }
  }
  else
  if(plussesInARow>0)
  {
      plusOut = plussesInARow;
      plussesInARow=0;
  }
  lastInputTimeMs = millis();
  return plusOut;
}

static bool checkPlusPlusPlusEscape()
{
  if((plussesInARow == 3) && ((millis()-lastInputTimeMs)>900))
  {
    plussesInARow = 0;
    return true;
  }
  return false;
}

void setup()
{
#if defined(ARDUINO_ESP32S3_DEV) && !defined(ENGMODEM_MINI_BOARD)
  // This board's onboard addressable RGB LED is GPIO38 (Espressif DevKitC-1
  // v1.1 J3 header docs - not the same pin the Arduino core's generic
  // esp32s3 variant assumes for RGB_BUILTIN, which is why neopixelWrite()
  // is called with the literal pin number here instead of that constant).
  // It used to be double-booked with VFD DB6; explicitly clear it now that
  // DB6 has moved, since a stray leftover color can persist indefinitely -
  // an addressable LED holds whatever it last received until a new frame
  // is clocked in, it doesn't reset itself just because the pin goes idle.
  // Skipped entirely on ENGMODEM_MINI_BOARD: that board has no RGB LED on
  // GPIO38 at all - it's a general expansion header pin there instead, and
  // this WS2812 protocol write would just be a spurious pulse train on it.
  neopixelWrite(38, 0, 0, 0);
#endif

  for(int i=0;i<MAX_PIN_NO;i++)
    pinSupport[i]=false;
#ifdef ZIMODEM_ESP32
  DBSerial.begin(115200); //the debug port
  DBSerial.setDebugOutput(true);
  Serial.begin(115200); //TEMP diagnostic -- DBSerial's UART0 pins don't reach COM14 on this board, native USB CDC does
# ifdef ARDUINO_ESP32S3_DEV
  pinSupport[1]=true;
# ifdef ENGMODEM_MINI_BOARD
  // DTR is on GPIO4 on this board, which the dev-board list below skips. Without
  // this, pinMode(pinDTR,INPUT) is never called and the pin reads a constant level.
  pinSupport[4]=true;
# endif
  for(int i=5;i<=21;i++)
    pinSupport[i]=true;
  for(int i=36;i<=38;i++)
    pinSupport[i]=true;
  pinSupport[47]=true;
  pinSupport[48]=true;
# else
   pinSupport[2]=true;
   pinSupport[4]=true;
   pinSupport[5]=true;
   for(int i=12;i<=23;i++)
     pinSupport[i]=true;
   for(int i=25;i<=27;i++)
     pinSupport[i]=true;
   for(int i=32;i<=36;i++)
     pinSupport[i]=true;
   pinSupport[39]=true;
# endif
#else
  pinSupport[0]=true;
  pinSupport[2]=true;
  if((ESP.getFlashChipRealSize()/1024)>=4096) // assume this is a strykelink/esp12e
  {
    pinSupport[4]=true;
    pinSupport[5]=true;
    for(int i=9;i<=16;i++)
      pinSupport[i]=true;
    pinSupport[11]=false;
  }
#endif
#ifdef DEFAULT_PIN_OPB
  static bool OPB_stat=0;
  pinMode(DEFAULT_PIN_OPB, INPUT);
  OPB_stat=digitalRead(DEFAULT_PIN_OPB);
#endif

  debugPrintf("Zimodem %s firmware starting initialization\r\n",ZIMODEM_VERSION);
  initSDShell();
  vfdInit();
  currMode = &commandMode;
  if(!SPIFFS.begin())
  {
    SPIFFS.format();
    SPIFFS.begin();
    debugPrintf("SPIFFS Formatted.\r\n");
  }
#ifdef ENGMODEM_MINI_BOARD
  // The Arduino-ESP32 core only accepts setRxBufferSize() BEFORE begin() ("RX Buffer can't be
  // resized when Serial is already running"). The original call further down came after begin(), so
  // it was silently rejected and the modem UART ran with the core's 256-byte default receive buffer.
  // With flow control off, a sustained full-speed stream then overflowed it (about 13% of a 20 KB
  // stream lost at 115200 on the built board). Set it first so RX_BUFFER_SIZE actually applies.
  HWSerial.setRxBufferSize(RX_BUFFER_SIZE);
#endif
# ifdef DEFAULT_PIN_RXD
    debugPrintf("Using BPS %d, RXD %d, TXD %d\r\n",DEFAULT_BAUD_RATE,DEFAULT_PIN_RXD, DEFAULT_PIN_TXD);
    HWSerial.begin(DEFAULT_BAUD_RATE, DEFAULT_SERIAL_CONFIG, DEFAULT_PIN_RXD, DEFAULT_PIN_TXD);
# else
    HWSerial.begin(DEFAULT_BAUD_RATE, DEFAULT_SERIAL_CONFIG);  //Start Serial
# endif
#ifndef ENGMODEM_MINI_BOARD
  HWSerial.setRxBufferSize(RX_BUFFER_SIZE);   // no effect on ESP32 (called after begin); kept for other builds
#endif
  commandMode.loadConfig();
#ifdef ENGMODEM_MINI_BOARD
  // Boot-time confirmation of the RTS/CTS pin roles actually in effect after
  // any saved config has been applied - see the pin block comment.
  debugPrintf("Flow control pins: CTS(in)=%d RTS(out)=%d\r\n",(int)pinCTS,(int)pinRTS);
#endif
  PhoneBookEntry::loadPhonebook();
  dcdStatus = dcdInactive;
  s_pinWrite(pinDCD,dcdStatus);
  flushSerial();
#if SUPPORT_LED_PINS
# ifndef ENGMODEM_MINI_BOARD
  s_pinWrite(DEFAULT_PIN_WIFI,(WiFi.status() == WL_CONNECTED)?DEFAULT_WIFI_ACTIVE:DEFAULT_WIFI_INACTIVE);
# else
  // Off-hook LED: starts inactive (no connections yet); checkOpenConnections()
  // drives it from here on based on actual open telnet/SSH/FTP/etc connections.
  s_pinWrite(DEFAULT_PIN_WIFI,DEFAULT_WIFI_INACTIVE);
# endif
  s_pinWrite(DEFAULT_PIN_HS,(baudRate>=DEFAULT_HS_BAUD)?DEFAULT_HS_ACTIVE:DEFAULT_HS_INACTIVE);
#endif
}

void checkReconnect()
{
  if((WiFi.status() != WL_CONNECTED)
  &&(nextReconnectDelay>0)
  &&(lastConnectAttempt>0)
  &&(wifiSSI.length()>0))
  {
     unsigned long now=millis();
     if(lastConnectAttempt > now)
       lastConnectAttempt=1;
     if(now > lastConnectAttempt + nextReconnectDelay)
     {
        debugPrintf("Attempting Reconnect to %s\r\n",wifiSSI.c_str());
        unsigned long oldReconnectDelay = nextReconnectDelay;
        if(!connectWifi(wifiSSI.c_str(),wifiPW.c_str(),staticIP,staticDNS,staticGW,staticSN))
          debugPrintf("Unable to reconnect to %s.\r\n",wifiSSI.c_str());
        nextReconnectDelay = oldReconnectDelay * 2;
        if(nextReconnectDelay > MAX_RECONNECT_DELAY)
          nextReconnectDelay = DEFAULT_RECONNECT_DELAY;
     }
  }
}

void checkFactoryReset()
{
#if defined(ZIMODEM_ESP32) && defined(PIN_FACTORY_RESET)
    if(!digitalRead(PIN_FACTORY_RESET))
    {
      if(resetPushTimer != 1)
      {
        if(resetPushTimer==0)
        {
          resetPushTimer=millis();
          if(resetPushTimer==1)
            resetPushTimer++;
        }
        else
        if((millis() - resetPushTimer) > 5000)
        {
          SPIFFS.remove(CONFIG_FILE);
          SPIFFS.remove(CONFIG_FILE_OLD);
          SPIFFS.remove("/zphonebook.txt");
          SPIFFS.remove("/zlisteners.txt");
          SPIFFS.remove("/znick.txt");
          PhoneBookEntry::clearPhonebook();
          SPIFFS.end();
          SPIFFS.format();
          SPIFFS.begin();
          PhoneBookEntry::clearPhonebook();
          if(WiFi.status() == WL_CONNECTED)
            WiFi.disconnect();
          baudRate = DEFAULT_BAUD_RATE;
          commandMode.loadConfig();
          PhoneBookEntry::loadPhonebook();
          dcdStatus = dcdInactive;
          s_pinWrite(pinDCD,dcdStatus);
          wifiSSI="";
          wifiPW="";
          hostname="";
          staticIP = null;
          staticDNS = null;
          staticGW = null;
          staticSN = null;
          delay(500);
          zclock.reset();
          commandMode.reset();
          resetPushTimer=1;
        }
      }
    }
    else
    if(resetPushTimer != 0)
      resetPushTimer=0;
#endif
}

void loop() 
{
  checkFactoryReset();
  checkReconnect();
  vfdLoop();
  if(HWSerial.available())
  {
    currMode->serialIncoming();
  }
  currMode->loop();
  zclock.tick();
}
