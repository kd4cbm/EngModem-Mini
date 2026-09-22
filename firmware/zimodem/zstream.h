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

#define ZSTREAM_ESC_BUF_MAX 10
#define ZSTREAM_TX_BUF_SIZE 250   // must stay <= 255: socketWrite(buf,len) takes a uint8_t length
#define ZSTREAM_RX_PASS_MAX 250   // max serial bytes serialIncoming() handles per main-loop pass (Mini board)
#define ZSTREAM_TX_IDLE_MS 2      // flush once no new serial byte has arrived for this long
#define ZSTREAM_TX_MAX_AGE_MS 10  // ...or once the oldest queued byte is this old

enum HangupType
{
  HANGUP_NONE,
  HANGUP_PPPHARD,
  HANGUP_DTR,
  HANGUP_PDP
};

class ZStream : public ZMode
{
  private:
    WiFiClientNode *current = null;
    unsigned long nextFlushMs = 0;
    ZSerial serial;
    HangupType hangupType = HANGUP_NONE;
    int lastDTR = 0;
    bool defaultEcho=false;
    int lastPDP = 0;
    uint8_t escBuf[ZSTREAM_ESC_BUF_MAX];
    unsigned long switchAlarm = millis() + 5000;
    // Serial->socket coalescing buffer. Only ever filled on ENGMODEM_MINI_BOARD
    // builds (see ZStream::txQueue); empty and inert elsewhere.
    uint8_t txBuf[ZSTREAM_TX_BUF_SIZE];
    uint8_t txLen = 0;
    unsigned long txFirstMs = 0;
    unsigned long txLastMs = 0;

  public:
    // Exposed read-only for the VFD status display (vfd.ino) - not used
    // internally beyond what already existed as private state/methods above.
    uint32_t vfdBytesOut = 0;
    uint32_t vfdBytesIn = 0;
    WiFiClientNode *getCurrentConnection() { return current; }
    FlowControlType getCurrentFlowControl() { return serial.getFlowControlType(); }

  private:

    void switchBackToCommandMode(bool pppMode);
    void socketWrite(uint8_t c);
    void socketWrite(uint8_t *buf, uint8_t len);
    void txQueue(uint8_t c);
    void txFlush();
    void txFlushIfDue();
    void baudDelay();

    bool isPETSCII();
    bool isEcho();
    FlowControlType getFlowControl();
    bool isTelnet();
    bool isDefaultEcho();
    bool isDisconnectedOnStreamExit();
    void doHangupChecks();

  public:
    void setDefaultEcho(bool tf);
    void switchTo(WiFiClientNode *conn);
    void setHangupType(HangupType type);
    HangupType getHangupType();
    
    void serialIncoming();
    void loop();
};

