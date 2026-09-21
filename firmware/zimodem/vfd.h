// VFD (Noritake CU24025ECPB-W1J, 24x2) direct-parallel driver.
// Ported from vfd-i2c-bridge/vfd-wiring-test/vfd-wiring-test.ino (proven on real
// hardware as a standalone STM32 test sketch), adapted from STM32duino to the
// ESP32 Arduino core. See ESP32S3_PIN_MAP.md at the repo root for wiring.

static void vfdInit();
static void vfdLoop();
static void vfdWriteByte(uint8_t rs, uint8_t value);
static void vfdSetCursor(uint8_t row, uint8_t col);
static void vfdPrint(const char *s);
static void vfdSetBrightness(uint8_t br);
static void vfdSetBrightnessLevel(uint8_t level); // 0=blanked, 1-4 = 25/50/75/100%
