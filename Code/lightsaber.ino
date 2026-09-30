#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "SoundData.h"
#include "XT_DAC_Audio.h"
#define DATA_PIN        3
#define PIN_BTN_POWER   27
#define PIN_BTN_COLOR   26
#define PIN_PWR         14
#define DAC_PIN         25
#define NUM_LEDS        30
CRGB leds[NUM_LEDS];
#define DEBOUNCE_MS       150
#define CLASH_DURATION_MS 350
#define SWING_COOLDOWN_MS 500
#define GYRO_THRESH       50
#define GYRO_MAX         450 
#define SWING_THRESH     100 
#define CLASH_ACCEL      22.0f
#define CLASH_DELTA      16.0f
enum PatternMode : uint8_t {
  MODE_RED  = 0,
  MODE_BLUE = 1,
  NUM_MODES  = 2
};

const CRGB BASE_COLORS[NUM_MODES] = {
  CRGB(180,   0,   0),
  CRGB(  0,   0, 200),
};
const CRGB CLASH_COLOR = CRGB(255, 80, 0);

RTC_DATA_ATTR uint8_t g_mode = MODE_BLUE;
XT_DAC_Audio_Class DacAudio(DAC_PIN, 1);
XT_Wav_Class SoundOn   (sound_on);
XT_Wav_Class SoundOff  (sound_off);
XT_Wav_Class SoundIdle (sound_idle);
XT_Wav_Class SoundSwing(sound_swing);
XT_Wav_Class SoundClash(sound_clash);
Adafruit_MPU6050 mpu;
bool     bladeOn      = false;
bool     clashActive  = false;
uint32_t clashStart   = 0;
uint32_t lastSwingMs  = 0;
float    lastAccelMag = 9.81f;   // 1 g at rest
float    pulsePhase   = 0.0f;
struct ToggleSwitch {
  uint8_t  pin;
  bool     raw      = HIGH;
  bool     state    = HIGH;
  uint32_t lastEdge = 0;

  void begin(uint8_t p) {
    pin = p;
    pinMode(p, INPUT_PULLUP);
    raw = state = digitalRead(p);
  }
  int update() {
    bool newRaw = digitalRead(pin);
    if (newRaw != raw) { raw = newRaw; lastEdge = millis(); }
    if ((millis() - lastEdge) > DEBOUNCE_MS) {
      if (raw != state) {
        bool prev = state;
        state = raw;
        if (prev == HIGH && state == LOW)  return  1;
        if (prev == LOW  && state == HIGH) return -1;
      }
    }
    return 0;
  }

  bool isOn() const { return state == LOW; }
};
struct Btn {
  uint8_t  pin;
  bool     raw      = HIGH;
  bool     state    = HIGH;
  uint32_t lastEdge = 0;

  void begin(uint8_t p) {
    pin = p;
    pinMode(p, INPUT_PULLUP);
  }

  bool pressed() {
    bool newRaw = digitalRead(pin);
    if (newRaw != raw) { raw = newRaw; lastEdge = millis(); }
    bool prev = state;
    if ((millis() - lastEdge) > DEBOUNCE_MS) state = raw;
    return (prev == HIGH && state == LOW);
  }
};

ToggleSwitch swPower;
Btn          btnColor;
void ledsOff() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void powerOnAnim() {
  DacAudio.Play(&SoundOn,   true);
  DacAudio.Play(&SoundIdle, true);
  SoundIdle.RepeatForever = true;

  CRGB col = BASE_COLORS[g_mode];
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = col;
    FastLED.show();
    DacAudio.FillBuffer();
    delay(300 / NUM_LEDS);
  }
}

void powerOffAnim() {
  DacAudio.Play(&SoundOff, false);
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    leds[i] = CRGB::Black;
    FastLED.show();
    DacAudio.FillBuffer();
    delay(300 / NUM_LEDS);
  }
  DacAudio.StopAllSounds();
  delay(80);
}
void colorChangeBlink() {
  fill_solid(leds, NUM_LEDS, BASE_COLORS[g_mode]);
  FastLED.show();
  delay(60);
}
void goToSleep() {
  ledsOff();
  DacAudio.StopAllSounds();
  delay(100);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
  esp_deep_sleep_start();
}
void setup() {
  Serial.begin(115200);

  pinMode(PIN_PWR, OUTPUT);
  digitalWrite(PIN_PWR, HIGH);

  swPower.begin(PIN_BTN_POWER);
  btnColor.begin(PIN_BTN_COLOR);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  ledsOff();

  SoundOn.Volume    = 30;
  SoundOff.Volume   = 30;
  SoundIdle.Volume  = 15;
  SoundSwing.Volume = 30;
  SoundClash.Volume = 30;

  Wire.begin(21, 22);
  if (!mpu.begin()) {
    Serial.println("[ERR] MPU6050 not detected — clash/swing disabled.");
  } else {
    mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
    Serial.println("[OK] MPU6050 ready.");
  }

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    if (swPower.isOn()) {
      bladeOn = true;
      powerOnAnim();
    } else {
      goToSleep();
    }
  } else if (swPower.isOn()) {
    bladeOn = true;
    powerOnAnim();
  }
}
void loop() {
  DacAudio.FillBuffer();
  int  swChange = swPower.update();
  bool colorBtn = btnColor.pressed();

  if (swChange == 1 && !bladeOn) {
    bladeOn = true;
    powerOnAnim();
  } else if (swChange == -1 && bladeOn) {
    bladeOn = false;
    powerOffAnim();
    goToSleep();
  }
  if (colorBtn && bladeOn) {
    g_mode = (g_mode + 1) % NUM_MODES;
    colorChangeBlink();
    Serial.printf("[COLOR] mode=%s\n", g_mode == MODE_RED ? "RED" : "BLUE");
  }

  if (!bladeOn) {
    delay(10);
    return;
  }
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float gx = g.gyro.x * RAD_TO_DEG;
  float gy = g.gyro.y * RAD_TO_DEG;
  float gz = g.gyro.z * RAD_TO_DEG;

  float accelMag   = sqrtf(ax*ax + ay*ay + az*az);
  float gyroMag    = sqrtf(gx*gx + gy*gy + gz*gz);
  float accelDelta = fabsf(accelMag - lastAccelMag);
  lastAccelMag     = accelMag;
  if (!clashActive && accelDelta > CLASH_DELTA && accelMag > CLASH_ACCEL) {
    clashActive = true;
    clashStart  = millis();
    fill_solid(leds, NUM_LEDS, CLASH_COLOR);
    FastLED.show();
    DacAudio.StopAllSounds();
    DacAudio.Play(&SoundClash, true);
    Serial.printf("[CLASH] accel=%.1f  delta=%.1f\n", accelMag, accelDelta);
  }
  if (clashActive && (millis() - clashStart) > CLASH_DURATION_MS) {
    clashActive = false;
    DacAudio.Play(&SoundIdle, true);
    SoundIdle.RepeatForever = true;
  }
  if (!clashActive &&
      gyroMag > SWING_THRESH &&
      !SoundSwing.Playing &&
      (millis() - lastSwingMs) > SWING_COOLDOWN_MS) {
    DacAudio.Play(&SoundSwing, true);
    lastSwingMs = millis();
  }
  if (clashActive) {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (random8() < 170) {
        leds[i] = CLASH_COLOR;
      } else {
        leds[i] = BASE_COLORS[g_mode];
        leds[i].nscale8(random8(80, 255));
      }
    }

  } else {
    pulsePhase += 0.05f;
    if (pulsePhase > TWO_PI) pulsePhase -= TWO_PI;
    float pulse = 1.0f + 0.15f * sinf(pulsePhase);
    uint8_t swingBoost = 0;
    if (gyroMag > GYRO_THRESH) {
      swingBoost = (uint8_t)constrain(
        map((long)gyroMag, GYRO_THRESH, GYRO_MAX, 0, 90), 0L, 90L);
    }
    CRGB base = BASE_COLORS[g_mode];
    CRGB col(
      constrain((int)(base.r * pulse) + swingBoost, 0, 255),
      constrain((int)(base.g * pulse) + swingBoost, 0, 255),
      constrain((int)(base.b * pulse) + swingBoost, 0, 255)
    );
    fill_solid(leds, NUM_LEDS, col);
  }
  FastLED.show();
  delay(10);
}