#include <U8g2lib.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- Display Settings ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// --- Pairing Settings ---
// Change to your own random UUID (uuidgenerator.net). Both keychains use the same one.
#define PAIR_UUID "6f1c2a3e-8b4d-4e57-9a10-3c5d7e9b1f42"

const unsigned long SCAN_SECONDS = 1;
const unsigned long SCAN_REST_MS = 59000;  // 1s scan + 59s rest = once per minute

const int TOGETHER_ENTER = -60;  // stronger than this = together
const int TOGETHER_EXIT = -70;   // weaker than this = leaves "together" (hysteresis)

// --- Easing and Animation Settings ---
const float EASING = 0.25;
const float ROUNDNESS = 0.45;
const unsigned long FRAME_MS = 33;  // ~30 fps
const float EYE_CY = 26;
const float MOUTH_CY = 55;

bool isBlinking = false;

// --- Emotions and Moods (defined early so all functions can use them) ---
enum Emotion { NEUTRAL,
               HAPPY,
               SAD,
               SURPRISED,
               SLEEPY,
               ANGRY,
               LOVE };
Emotion currentEmotion = NEUTRAL;

enum Mood { M_APART,
            M_NEAR,
            M_TOGETHER };
Mood mood = M_NEAR;  // neutral-ish until the first scan finishes

// --- BLE shared state (written by BLE task, read by loop) ---
volatile bool scanFound = false;     // during a scan: did we see the partner?
volatile int scanBestRssi = -127;    // during a scan: strongest RSSI seen
volatile bool newScanResult = false; // set when a scan finishes
volatile bool resultFound = false;   // published result
volatile int resultRssi = -127;

class PartnerCB : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) {
    if (dev.isAdvertisingService(BLEUUID(PAIR_UUID))) {
      int rssi = dev.getRSSI();
      if (!scanFound || rssi > scanBestRssi) scanBestRssi = rssi;
      scanFound = true;
    }
  }
};

// Runs in its own task so scanning never stalls the animation
void bleTask(void *) {
  BLEDevice::init("kc");
  BLEDevice::createServer();

  // Always advertise so the partner can find us during its scan
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(PAIR_UUID);
  adv->setMinInterval(160);  // 100 ms
  adv->setMaxInterval(160);
  adv->start();

  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new PartnerCB(), false);
  scan->setActiveScan(false);
  scan->setInterval(100);
  scan->setWindow(99);

  for (;;) {
    scanFound = false;
    scanBestRssi = -127;

    scan->start(SCAN_SECONDS, false);  // blocks for the scan duration
    scan->clearResults();

    resultFound = scanFound;
    resultRssi = scanBestRssi;
    newScanResult = true;

    Serial.printf("Scan done: found=%d rssi=%d\n", (int)resultFound, (int)resultRssi);

    vTaskDelay(SCAN_REST_MS / portTICK_PERIOD_MS);
  }
}

// Only called when a NEW scan result arrives, so mood is held between scans
Mood moodFromScan(bool found, int rssi) {
  if (!found) return M_APART;
  if (mood == M_TOGETHER) return (rssi < TOGETHER_EXIT) ? M_NEAR : M_TOGETHER;
  return (rssi > TOGETHER_ENTER) ? M_TOGETHER : M_NEAR;
}

Emotion pickEmotionForMood() {
  int r = random(100);
  switch (mood) {
    case M_TOGETHER: return r < 60 ? LOVE : HAPPY;
    case M_NEAR: return r < 50 ? NEUTRAL : (r < 80 ? HAPPY : SURPRISED);
    default: return r < 50 ? SAD : (r < 80 ? SLEEPY : NEUTRAL);
  }
}

// --- Shape Structure (used for eyes and mouth) ---
struct Eye {
  float x, y, w, h;      // Current rendering values
  float tx, ty, tw, th;  // Target values (emotion base shape)
  float ox, oy;          // Saccade offset
  bool blinks = true;    // Mouth sets this to false

  void snap() {
    x = tx + ox;
    y = ty + oy;
    w = tw;
    h = th;
  }

  void update() {
    float eh = (blinks && isBlinking) ? 2 : th;
    x += (tx + ox - x) * EASING;
    y += (ty + oy - y) * EASING;
    w += (tw - w) * EASING;
    h += (eh - h) * EASING;
  }

  void draw() {
    int ww = max(2, (int)roundf(w));
    int hh = max(2, (int)roundf(h));
    int left = (int)roundf(x - w / 2.0f);
    int top = (int)roundf(y - h / 2.0f);

    left = constrain(left, 0, SCREEN_WIDTH - ww);
    top = constrain(top, 0, SCREEN_HEIGHT - hh);

    int minSide = min(ww, hh);
    int r = (int)(minSide * ROUNDNESS);
    r = min(r, minSide / 2 - 1);
    if (r < 0) r = 0;

    u8g2.drawRBox(left, top, ww, hh, r);
  }
};

Eye leftEye, rightEye, mouth;

void drawHeart(int cx, int cy, int s) {  // s = size
  int r = s / 3;
  u8g2.drawDisc(cx - r, cy - r / 2, r);
  u8g2.drawDisc(cx + r, cy - r / 2, r);
  u8g2.drawTriangle(cx - 2 * r, cy - r / 4, cx + 2 * r, cy - r / 4, cx, cy + s / 2 + r / 2);
}

unsigned long lastActionTime = 0;
unsigned long actionInterval = 2000;
unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 3000;
unsigned long lastFrameTime = 0;

void setup() {
  Serial.begin(115200);
  delay(250);

  u8g2.begin();
  Wire.setClock(400000);

  mouth.blinks = false;

  setEmotion(NEUTRAL);
  leftEye.snap();
  rightEye.snap();
  mouth.snap();

  xTaskCreate(bleTask, "ble", 8192, NULL, 1, NULL);
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Spontaneous blinking
  if (!isBlinking && currentMillis - lastBlinkTime > blinkInterval) {
    isBlinking = true;
    lastBlinkTime = currentMillis;
    blinkInterval = random(2000, 6000);
  } else if (isBlinking && currentMillis - lastBlinkTime > 150) {
    isBlinking = false;
    lastBlinkTime = currentMillis;
  }

  // 2. Mood only changes when a new scan result arrives
  bool moodChanged = false;
  if (newScanResult) {
    newScanResult = false;
    Mood newMood = moodFromScan(resultFound, resultRssi);
    if (newMood != mood) {
      mood = newMood;
      moodChanged = true;
    }
  }

  if (moodChanged) {
    lastActionTime = currentMillis;
    if (mood == M_TOGETHER) {
      setEmotion(SURPRISED);  // "oh! you're here!" then love/happy
      actionInterval = 1200;
    } else if (mood == M_APART) {
      setEmotion(SAD);
      actionInterval = 2500;
    } else {
      setEmotion(NEUTRAL);
      actionInterval = 2000;
    }
  } else if (currentMillis - lastActionTime > actionInterval) {
    // Normal idle behaviour, drawn from the current (held) mood
    lastActionTime = currentMillis;
    actionInterval = random(1500, 4000);
    if (random(100) < 40) saccade();
    else setEmotion(pickEmotionForMood());
  }

  // 3. Frame-limited update + render
  if (currentMillis - lastFrameTime >= FRAME_MS) {
    lastFrameTime = currentMillis;

    leftEye.update();
    rightEye.update();
    mouth.update();

    u8g2.clearBuffer();
    if (currentEmotion == LOVE) {
      int pulse = 24 + (int)(4 * sin(millis() / 150.0f));
      drawHeart((int)leftEye.x, (int)leftEye.y, pulse);
      drawHeart((int)rightEye.x, (int)rightEye.y, pulse);
    } else {
      leftEye.draw();
      rightEye.draw();
    }
    mouth.draw();
    u8g2.sendBuffer();
  }
}

// --- Animation Helpers ---

void setEmotion(Emotion emo) {
  currentEmotion = emo;
  float cy = EYE_CY;
  float lx = (SCREEN_WIDTH / 2) - 24;
  float rx = (SCREEN_WIDTH / 2) + 24;
  float mx = SCREEN_WIDTH / 2;
  float my = MOUTH_CY;

  leftEye.ox = leftEye.oy = 0;
  rightEye.ox = rightEye.oy = 0;
  mouth.ox = mouth.oy = 0;

  switch (emo) {
    case NEUTRAL:
      setEyeTargets(lx, cy, 24, 40, rx, cy, 24, 40);
      setMouthTargets(mx, my, 16, 4);
      break;
    case HAPPY:
      setEyeTargets(lx, cy - 6, 28, 20, rx, cy - 6, 28, 20);
      setMouthTargets(mx, my, 30, 9);
      break;
    case SAD:
      setEyeTargets(lx + 4, cy + 8, 20, 24, rx - 4, cy + 8, 20, 24);
      setMouthTargets(mx, my + 1, 12, 4);
      break;
    case SURPRISED:
      setEyeTargets(lx, cy - 6, 20, 44, rx, cy - 6, 20, 44);
      setMouthTargets(mx, my + 1, 10, 11);
      break;
    case SLEEPY:
      setEyeTargets(lx, cy + 12, 24, 8, rx, cy + 12, 24, 8);
      setMouthTargets(mx, my + 1, 8, 3);
      break;
    case ANGRY:
      setEyeTargets(lx + 8, cy + 4, 22, 22, rx - 8, cy + 4, 22, 22);
      setMouthTargets(mx, my, 22, 3);
      break;
    case LOVE:
      setEyeTargets(lx, cy - 4, 28, 28, rx, cy - 4, 28, 28);
      setMouthTargets(mx, my, 26, 8);
      break;
  }
}

// Saccade: sets (not accumulates) a small offset; mouth follows at half strength
void saccade() {
  float offsetX = random(-12, 13);
  float offsetY = random(-8, 9);

  leftEye.ox = rightEye.ox = offsetX;
  leftEye.oy = rightEye.oy = offsetY;
  mouth.ox = offsetX * 0.5f;
  mouth.oy = 0;
}

void setEyeTargets(float lx, float ly, float lw, float lh,
                   float rx, float ry, float rw, float rh) {
  leftEye.tx = lx;
  leftEye.ty = ly;
  leftEye.tw = lw;
  leftEye.th = lh;
  rightEye.tx = rx;
  rightEye.ty = ry;
  rightEye.tw = rw;
  rightEye.th = rh;
}

void setMouthTargets(float x, float y, float w, float h) {
  mouth.tx = x;
  mouth.ty = y;
  mouth.tw = w;
  mouth.th = h;
}