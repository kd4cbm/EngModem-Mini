// VFD (Noritake CU24025ECPB-W1J, 24x2) direct-parallel driver.
// R/W is hardwired to GND on the VFD board itself - this driver is write-only
// and never polls the busy flag, matching the VFD board's own design.
#if INCLUDE_VFD

#define VFD_ROWS 2
#define VFD_COLS 24

static void vfdPulseEnable()
{
  digitalWrite(DEFAULT_PIN_VFD_E, HIGH);
  delayMicroseconds(1);
  digitalWrite(DEFAULT_PIN_VFD_E, LOW);
  delayMicroseconds(1);
}

// nibble bit3->DB7 .. bit0->DB4
static void vfdWriteNibble(uint8_t nibble)
{
  digitalWrite(DEFAULT_PIN_VFD_DB7, (nibble >> 3) & 0x01);
  digitalWrite(DEFAULT_PIN_VFD_DB6, (nibble >> 2) & 0x01);
  digitalWrite(DEFAULT_PIN_VFD_DB5, (nibble >> 1) & 0x01);
  digitalWrite(DEFAULT_PIN_VFD_DB4, (nibble >> 0) & 0x01);
  vfdPulseEnable();
}

// rs=0 instruction, rs=1 data (character write). High nibble first.
static void vfdWriteByte(uint8_t rs, uint8_t value)
{
  digitalWrite(DEFAULT_PIN_VFD_RS, rs ? HIGH : LOW);
  delayMicroseconds(1);          // RS/data setup before E, covers the 20ns min
  vfdWriteNibble((value >> 4) & 0x0F);
  vfdWriteNibble(value & 0x0F);

  if(rs == 0 && value <= 0x03)
    delayMicroseconds(100);      // Display Clear (01H) / Cursor Home (02H-03H)
  else
    delayMicroseconds(1);        // 666ns typ settle for everything else
}

// BR: 0=100% (default) 1=75% 2=50% 3=25%
static void vfdSetBrightness(uint8_t br)
{
  vfdWriteByte(0, 0x20);         // Function Set (IF=0) arms brightness-control
  vfdWriteByte(1, br & 0x03);    // next RS=1 byte is brightness data, not a char
}

// AT$vfdb= scale: 0=blanked, 1=25%, 2=50%, 3=75%, 4=100%. The hardware only
// has these 4 real brightness levels (no finer control exists), so this maps
// 1:1 onto them rather than approximating a finer scale. 0 sends Display OFF
// rather than a brightness value - besides blanking the screen, the
// datasheet notes this also inhibits the VFD's internal power converter, so
// it's a real power saving, not just a dark display.
static void vfdSetBrightnessLevel(uint8_t level)
{
  static const uint8_t brCodes[4] = { 0x03, 0x02, 0x01, 0x00 }; // level 1-4 -> 25/50/75/100%

  if(level == 0)
  {
    vfdWriteByte(0, 0x08);        // Display OFF, cursor off, blink off
  }
  else if(level <= 4)
  {
    vfdWriteByte(0, 0x0C);        // Display ON, cursor off, blink off (idempotent if already on)
    vfdSetBrightness(brCodes[level - 1]);
  }
}

static void vfdSetCursor(uint8_t row, uint8_t col)
{
  uint8_t base = row ? 0xC0 : 0x80;
  vfdWriteByte(0, base + col);
}

static void vfdPrint(const char *s)
{
  while(*s)
    vfdWriteByte(1, (uint8_t)*s++);
}

// CGRAM custom characters for the status line's TX/RX activity arrows - the
// VFD's built-in font has no arrow glyphs, so these are defined once at init
// as codes 1 and 2 (not 0 - code 0 can't be embedded as a value the way the
// status line composes its byte buffer). 8 rows/glyph even though only 7
// are visible - the 8th is the cursor row and is left blank.
static const uint8_t vfdGlyphUp[8]   = {0b00100,0b01110,0b11111,0b00100,0b00100,0b00100,0b00000,0b00000};
static const uint8_t vfdGlyphDown[8] = {0b00000,0b00100,0b00100,0b00100,0b11111,0b01110,0b00100,0b00000};

static void vfdDefineChar(uint8_t code, const uint8_t *pattern)
{
  vfdWriteByte(0, 0x40 | ((code & 0x07) << 3));  // Set CGRAM address to code*8
  for(uint8_t i = 0; i < 8; i++)
    vfdWriteByte(1, pattern[i]);
  vfdWriteByte(0, 0x80);                         // back to DDRAM home, not left pointed at CGRAM
}

// Generous on purpose: if the VFD's own internal supply is still stabilizing
// when vfdInit() first writes the splash, that one-shot write can be lost on
// a controller that isn't fully awake yet - vfdLoop() re-sends the splash
// every VFD_SPLASH_REFRESH_MS during this window specifically to catch that
// case, rather than relying on the first attempt landing.
// Total hold, not visible time: testing showed the VFD's own warm-up eats
// roughly the first ~1.5s (writes during that window get sent but don't
// land), so the budget here is that ~1.5s plus the 3s of actual visible
// splash time we want, plus some margin for run-to-run variance. Bumped
// 5000->7000 on 2026-08-22 after the DB6/DB7 rewiring: visible splash time
// dropped to roughly half of the intended 3s again, suggesting the warm-up
// window grew - same fix pattern as before, more total budget.
#define VFD_SPLASH_HOLD_MS    7000UL
#define VFD_SPLASH_REFRESH_MS 250UL

static uint32_t vfdSplashUntil = 0;

static void vfdInit()
{
  pinMode(DEFAULT_PIN_VFD_RS, OUTPUT);
  pinMode(DEFAULT_PIN_VFD_E, OUTPUT);
  pinMode(DEFAULT_PIN_VFD_DB4, OUTPUT);
  pinMode(DEFAULT_PIN_VFD_DB5, OUTPUT);
  pinMode(DEFAULT_PIN_VFD_DB6, OUTPUT);
  pinMode(DEFAULT_PIN_VFD_DB7, OUTPUT);
  digitalWrite(DEFAULT_PIN_VFD_E, LOW);
  digitalWrite(DEFAULT_PIN_VFD_RS, LOW);

  // Let the module's own POR / supply rise settle. The datasheet only
  // requires <50ms rise time, but bulk capacitance on the 5V rail can
  // stretch the real-world settle time well past that - and unlike the
  // splash below, this delay only runs once, so it's worth erring generous.
  delay(250);

  // Wake-up nibble: the controller powers up expecting 8-bit-wide
  // instructions, but DB3-DB0 are "don't care" for Function Set, so one
  // lone upper-nibble write is already a complete, valid instruction. Sets
  // IF=0 and switches into 4-bit mode for every instruction after - the
  // same trick classic HD44780 4-bit init sequences use.
  vfdWriteNibble(0b0010);        // DB7=0 DB6=0 DB5=1 DB4=0 (IF=0)
  delayMicroseconds(100);

  vfdWriteByte(0, 0x20);         // Function Set, confirm IF=0 (4-bit)
  vfdWriteByte(0, 0x0C);         // Display ON, cursor off, blink off
  vfdWriteByte(0, 0x06);         // Entry mode: increment, no shift
  vfdWriteByte(0, 0x01);         // Clear display

  vfdDefineChar(1, vfdGlyphUp);
  vfdDefineChar(2, vfdGlyphDown);

  vfdPrintCentered(0, "Zimodem");
  vfdPrintCentered(1, "Version " ZIMODEM_VERSION);
  vfdSplashUntil = millis() + VFD_SPLASH_HOLD_MS;   // vfdLoop() holds off until this passes

  debugPrintf("VFD initialized: RS=%d E=%d DB4-7=%d,%d,%d,%d\r\n",
    (int)DEFAULT_PIN_VFD_RS, (int)DEFAULT_PIN_VFD_E,
    (int)DEFAULT_PIN_VFD_DB4, (int)DEFAULT_PIN_VFD_DB5,
    (int)DEFAULT_PIN_VFD_DB6, (int)DEFAULT_PIN_VFD_DB7);
}

#define VFD_PAGE_HOLD_MS     3000UL
#define VFD_ACTIVITY_HOLD_MS 200UL
#define VFD_SCROLL_STEP_MS   300UL   // ms per column when a page's content is too long to fit

// Left-pads/truncates to exactly VFD_COLS and writes one row. Plain-ASCII
// only - see vfdBottomRow() for the row that carries the two custom glyphs.
static void vfdPrintRow(uint8_t row, const char *text)
{
  char buf[VFD_COLS + 1];
  size_t n = strlen(text);
  if(n > VFD_COLS)
    n = VFD_COLS;
  memcpy(buf, text, n);
  for(size_t i = n; i < VFD_COLS; i++)
    buf[i] = ' ';
  buf[VFD_COLS] = 0;
  vfdSetCursor(row, 0);
  vfdPrint(buf);
}

// Centers text within VFD_COLS (leading spaces only - vfdPrintRow's own
// right-pad handles the trailing side), truncating if it's somehow too long.
static void vfdPrintCentered(uint8_t row, const char *text)
{
  char buf[VFD_COLS + 1];
  size_t n = strlen(text);
  if(n > VFD_COLS)
    n = VFD_COLS;
  size_t lead = (VFD_COLS - n) / 2;
  for(size_t i = 0; i < lead; i++)
    buf[i] = ' ';
  memcpy(buf + lead, text, n);
  buf[lead + n] = 0;
  vfdPrintRow(row, buf);
}

// Full number through 9600 (widest case is "9600", 4 chars); only abbreviate
// above that, e.g. 19200 -> "19.2k", 115200 -> "115.2k". The middle field is
// at least 6 chars wide even in the worst case (XON/XOFF shown, "921.6k" is
// also 6 chars), so there's no truncation risk either way - this is a
// readability choice, not a fit one.
static void vfdFormatBaud(int baud, char *out, size_t outSize)
{
  if(baud > 9600)
    snprintf(out, outSize, "%.1fk", baud / 1000.0);
  else
    snprintf(out, outSize, "%d", baud);
}

// TX<arrow> RX<arrow> [Sd], baud centered in whatever's left, flow control
// mode right-aligned - truncating the baud field rather than colliding with
// its neighbors if it doesn't fit (in practice it always fits, even in the
// worst case of 921.6k baud with XON/XOFF shown - the "Sd" slot is reserved
// whether or not it's actually shown, so the rest of the row never shifts).
static void vfdBottomRow(bool txActive, bool rxActive)
{
  uint8_t row[VFD_COLS];
  for(int i = 0; i < VFD_COLS; i++)
    row[i] = ' ';

  uint8_t left[10] = { 'T', 'X', (uint8_t)(txActive ? 1 : '.'),
                        ' ', 'R', 'X', (uint8_t)(rxActive ? 2 : '.'),
                        ' ', (uint8_t)(sdShellAvailable ? 'S' : ' '),
                             (uint8_t)(sdShellAvailable ? 'd' : ' ') };
  memcpy(row, left, sizeof left);

  FlowControlType fct = streamMode.getCurrentFlowControl();
  const char *flow = (fct == FCT_RTSCTS) ? "RTS/CTS" : (fct == FCT_NORMAL) ? "XON/XOFF" : "NONE";
  uint8_t flowLen = strlen(flow);
  memcpy(row + (VFD_COLS - flowLen), flow, flowLen);

  char baud[8];
  vfdFormatBaud(baudRate, baud, sizeof baud);
  uint8_t baudLen = strlen(baud);
  uint8_t midStart = sizeof left;
  uint8_t midWidth = (VFD_COLS - flowLen) - midStart;
  uint8_t shown = baudLen > midWidth ? midWidth : baudLen;
  uint8_t offset = midStart + (midWidth - shown) / 2;
  memcpy(row + offset, baud, shown);

  vfdSetCursor(1, 0);
  for(int i = 0; i < VFD_COLS; i++)
    vfdWriteByte(1, row[i]);
}

static void vfdLoop()
{
  static uint32_t lastUpdate = 0;
  static uint32_t lastBytesIn = 0, lastBytesOut = 0;
  static uint32_t rxHoldUntil = 0, txHoldUntil = 0;
  static uint8_t  page = 0;
  static uint32_t pageStart = 0;
  static uint32_t scrollOffset = 0;   // current window position, in the content+gap loop
  static uint32_t scrollSteps = 0;    // columns advanced on this page so far, for detecting one full pass
  static uint32_t lastScrollStep = 0;
  char top[VFD_COLS + 40];   // extra room for "Dest: " + host + ":" + port before truncation

  if(millis() < vfdSplashUntil)
  {
    // Re-send the splash periodically rather than writing it once and
    // hoping - see the comment on VFD_SPLASH_HOLD_MS for why.
    static uint32_t lastSplashWrite = 0;
    if(millis() - lastSplashWrite >= VFD_SPLASH_REFRESH_MS)
    {
      lastSplashWrite = millis();
      // CGRAM is exactly as vulnerable to the same "VFD not awake yet" loss
      // as the splash text was - it just wasn't being retried before, which
      // is why the arrows were showing as solid blocks (undefined CGRAM
      // content, not a bad bitmap). Once a write actually lands, CGRAM holds
      // it permanently, so retrying here for the same window is sufficient.
      vfdDefineChar(1, vfdGlyphUp);
      vfdDefineChar(2, vfdGlyphDown);
      vfdPrintCentered(0, "Zimodem");
      vfdPrintCentered(1, "Version " ZIMODEM_VERSION);
    }
    return;
  }
  if(millis() - lastUpdate < 100)
    return;
  lastUpdate = millis();

  bool isStreaming = (currMode == &streamMode);

  if(streamMode.vfdBytesOut != lastBytesOut)
  {
    lastBytesOut = streamMode.vfdBytesOut;
    txHoldUntil = millis() + VFD_ACTIVITY_HOLD_MS;
  }
  if(streamMode.vfdBytesIn != lastBytesIn)
  {
    lastBytesIn = streamMode.vfdBytesIn;
    rxHoldUntil = millis() + VFD_ACTIVITY_HOLD_MS;
  }
  bool txActive = isStreaming && (millis() < txHoldUntil);
  bool rxActive = isStreaming && (millis() < rxHoldUntil);

  WiFiClientNode *conn = streamMode.getCurrentConnection();
  bool hasConn = (conn != null) && conn->isConnected();

  if(WiFi.status() != WL_CONNECTED)
  {
    snprintf(top, sizeof top, "No WiFi");
    page = 0; pageStart = millis(); scrollOffset = 0; scrollSteps = 0;
  }
  else if(!hasConn)
  {
    // IP left-aligned, "Ready" right-aligned so its 'y' always lands in the
    // last column regardless of IP address length - same right-justify
    // approach as the flow-control field in vfdBottomRow().
    memset(top, ' ', VFD_COLS);
    top[VFD_COLS] = 0;
    String ip = WiFi.localIP().toString();
    size_t ipLen = ip.length();
    if(ipLen > VFD_COLS)
      ipLen = VFD_COLS;
    memcpy(top, ip.c_str(), ipLen);
    const char *ready = "Ready";
    size_t readyLen = strlen(ready);
    memcpy(top + (VFD_COLS - readyLen), ready, readyLen);
    page = 0; pageStart = millis(); scrollOffset = 0; scrollSteps = 0;
  }
  else
  {
    char pageContent[VFD_COLS + 40];
    if(page == 0)
      snprintf(pageContent, sizeof pageContent, "Modem: %s", WiFi.localIP().toString().c_str());
    else
      // conn->host is the actual dialed name/address, not just a resolved
      // numeric IP - an advantage over RetroWiFiModem, which only ever had
      // the numeric remote IP available at this point (see VFD_DISPLAY.md).
      snprintf(pageContent, sizeof pageContent, "Dest: %s:%d", (conn->host != null) ? conn->host : "?", conn->port);

    size_t contentLen = strlen(pageContent);

    if(contentLen <= VFD_COLS)
    {
      // fits as-is - fixed hold, same as always
      strncpy(top, pageContent, sizeof top);
      if(millis() - pageStart >= VFD_PAGE_HOLD_MS)
      {
        page = page ? 0 : 1;
        pageStart = millis(); scrollOffset = 0; scrollSteps = 0;
      }
    }
    else
    {
      // Too long to fit (this only happens on the Dest page, for a long
      // hostname) - scroll it instead of clipping, and hold this page
      // until one full pass has been shown rather than a fixed duration,
      // so the whole destination is always readable regardless of length.
      char loopBuf[sizeof(pageContent) + 6];
      snprintf(loopBuf, sizeof loopBuf, "%s     ", pageContent);   // trailing gap before it repeats
      size_t loopLen = strlen(loopBuf);

      if(millis() - lastScrollStep >= VFD_SCROLL_STEP_MS)
      {
        lastScrollStep = millis();
        scrollOffset = (scrollOffset + 1) % loopLen;
        scrollSteps++;
      }
      for(uint8_t i = 0; i < VFD_COLS; i++)
        top[i] = loopBuf[(scrollOffset + i) % loopLen];
      top[VFD_COLS] = 0;

      if(scrollSteps >= loopLen)
      {
        page = page ? 0 : 1;
        pageStart = millis(); scrollOffset = 0; scrollSteps = 0;
      }
    }
  }

  vfdPrintRow(0, top);
  vfdBottomRow(txActive, rxActive);
}

#else

static void vfdInit() {}
static void vfdLoop() {}

#endif // INCLUDE_VFD
