#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioPlaySdWav           playSdWav1;
AudioOutputI2S           i2s1;
AudioFilterBiquad        biquad1;
AudioConnection          patchCord1(playSdWav1, 0, biquad1, 0);
AudioConnection          patchCord2(biquad1, 0, i2s1, 0);
AudioConnection          patchCord3(biquad1, 0, i2s1, 1);
AudioControlSGTL5000     audioShield;

// Uncomment for debugging
//#define BENCH_MODE

#define SDCARD_CS_PIN 10  // Audio Shield
#define POWER_LED     22

const double bq0[] = {
  0.9947972407097025,
  -1.9029341844395047,
  0.9433521183861597,
  -1.9029341844395047,
  0.9381493590958622
};
const double bq1[] = {
  0.9848101258174138,
  -1.7904043280826825,
  0.9049441818439214,
  -1.7904043280826825,
  0.8897543076613353
};
const double bq2[] = {
  0.9978206281684215,
  -1.9730691877025888,
  0.9826992042579441,
  -1.9730691877025888,
  0.9805198324263658
};

// Use real pin numbers (printed on the board).
const int outs[] = { 0, 1, 2, 3, 4, 5, 9 };
const int ins[] = { 14, 15, 16, 17, 24, 25, 27 };

#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

void setup() {
  AudioMemory(10);
#ifndef BENCH_MODE
  audioShield.enable();
  audioShield.muteLineout();
  audioShield.muteHeadphone();
  audioShield.lineOutLevel(15);
#endif

  pinMode(POWER_LED, OUTPUT);
  digitalWrite(POWER_LED, HIGH);

  biquad1.setCoefficients(0, bq0);
  biquad1.setCoefficients(1, bq1);
  biquad1.setCoefficients(2, bq2);

#ifndef BENCH_MODE
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      digitalWrite(POWER_LED, LOW);
      delay(250);
      digitalWrite(POWER_LED, HIGH);
      delay(250);
    }
  }
#endif

  for (unsigned int out = 0; out < ARRAY_SIZE(outs); ++out) {
    pinMode(outs[out], OUTPUT);
    digitalWrite(outs[out], LOW);
  }
  for (unsigned int in = 0; in < ARRAY_SIZE(ins); ++in) {
    pinMode(ins[in], INPUT);
  }

#ifndef BENCH_MODE
  audioShield.unmuteLineout();
  playSdWav1.play("hello.wav");
#endif
}

enum { MODE_SOUNDS, MODE_LETTERS };
int mode = MODE_SOUNDS;
unsigned int last_x = 1024, last_y = 1024;

#define MODE_CHANGE_X         6
#define MODE_CHANGE_SOUNDS_Y  5
#define MODE_CHANGE_LETTERS_Y 6

void playSound(int x, int y);

void loop() {
  if (!playSdWav1.isPlaying()) {
    last_x = last_y = 1024;
  }
  int delayAmount = 0;
  for (unsigned int out = 0; out < ARRAY_SIZE(outs); ++out) {
    digitalWrite(outs[out], HIGH);
    delay(10);
    delayAmount += 10;
    for (unsigned int in = 0; in < ARRAY_SIZE(ins); ++in) {
      if (digitalRead(ins[in])) {
        Serial.printf("out: %d, in %d\n", out, in);
        if (last_x != out || last_y != in) {
          playSound(out, in);
          last_x = out;
          last_y = in;
        }
        if (out == MODE_CHANGE_X) {
          if (in == MODE_CHANGE_SOUNDS_Y) {
            mode = MODE_SOUNDS;
          } else if (in == MODE_CHANGE_LETTERS_Y) {
            mode = MODE_LETTERS;
          }
        }
        break;
      }
    }
    digitalWrite(outs[out], LOW);
    delay(10);
    delayAmount += 10;
  }
  delay(250 - delayAmount);
}

const int numbers_map[2][7] = {
  { 4, 5, 7, 9, 3, 2, 1 },
  { 0, 6, 8, 10, 0, 0, 0 }
};
#define NUMBERS_XS ARRAY_SIZE(numbers_map)
const char* sounds_map[5][7] = {
  { NULL, "YE", "YU", "YA", "Y", NULL, "SCH" },
  { "KH", "F", "U", "T", "C", "TCH", "SH" },
  { "O", "P", "R", "S", "N", "M", "L" },
  { "Z", "ZH", "YO", "E", "I", "YI", "K" },
  { "B", "V", "G", "D", "A", NULL, NULL }
};
const char* letters_map[5][7] = {
  { "SOFT", "YE", "YU", "YA", "Y", "HARD", "SCH" },
  { "KH", "F", "U", "T", "C", "TCH", "SH" },
  { "O", "P", "R", "S", "N", "M", "L" },
  { "Z", "ZH", "YO", "E", "I", "YI", "K" },
  { "B", "V", "G", "D", "A", NULL, NULL }
};

void playSound(unsigned int x, unsigned int y) {
  char sound_buf[64];
  const char* sound = NULL;
  if (x < NUMBERS_XS) {
    int snd = numbers_map[x][y];
    if (snd != 0) {
      snprintf(sound_buf, ARRAY_SIZE(sound_buf), "num_%d.wav", snd);
      sound = sound_buf;
    }
  } else {
    if (x == MODE_CHANGE_X) {
      if (y == MODE_CHANGE_SOUNDS_Y) {
        sound = "m_sounds.wav";
      } else if (y == MODE_CHANGE_LETTERS_Y) {
        sound = "m_lettrs.wav";
      }
    }
    if (sound == NULL) {
      x -= NUMBERS_XS;
      const char* snd;
      const char* fmt;
      if (mode == MODE_SOUNDS) {
        snd = sounds_map[x][y];
        fmt = "snd_%s.wav";
      } else {
        snd = letters_map[x][y];
        fmt = "ltr_%s.wav";
      }
      if (snd != NULL) {
        snprintf(sound_buf, ARRAY_SIZE(sound_buf), fmt, snd);
        sound = sound_buf;
      }
    }
  }
  if (sound != NULL) {
    playSdWav1.stop();
    Serial.printf("playing \"%s\"\n", sound);
#ifndef BENCH_MODE
    playSdWav1.play(sound);
    delay(5);
#endif
  }
}
