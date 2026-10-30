#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorWAV.h"
#include "AudioGeneratorMP3.h"
#include "SPI.h"
#include "SD.h"
#include <vector>
#include <Preferences.h> // Kalıcı hafıza kütüphanesi

// --- PIN TANIMLAMALARI ---
#define BTN_NEXT      17
#define BTN_PREV      4
#define BTN_PP        16

#define SD_CS         27
#define SPI_MOSI      13
#define SPI_MISO      19
#define SPI_SCK       14
#define I2S_DOUT      25
#define I2S_BCLK      26
#define I2S_LRC       22

AudioGenerator *gen = NULL;
AudioFileSourceSD *source = NULL;
AudioOutputI2S *out = NULL;
Preferences prefs; // Hafıza nesnesi

std::vector<String> trackList;
int currentTrackIndex = 0;
bool isPaused = false;
unsigned long lastButtonPress = 0;

void scanSD() {
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String fileName = String("/") + entry.name();
    if (fileName.endsWith(".wav") || fileName.endsWith(".WAV") || 
        fileName.endsWith(".mp3") || fileName.endsWith(".MP3")) {
      trackList.push_back(fileName);
      Serial.println("Listeye eklendi: " + fileName);
    }
    entry.close();
  }
}

void stopPlayback() {
  if (gen) {
    gen->stop();
    delete gen;
    gen = NULL;
  }
  if (source) {
    source->close();
    delete source;
    source = NULL;
  }
}

void startPlayback(String path) {
  stopPlayback();
  isPaused = false;
  Serial.println("Caliniyor: " + path);
  
  // Hafızaya kaydet: Hangi indekste kaldığımızı yaz
  prefs.putInt("lastTrack", currentTrackIndex); 
  
  source = new AudioFileSourceSD(path.c_str());
  if (path.endsWith(".mp3") || path.endsWith(".MP3")) {
    gen = new AudioGeneratorMP3();
  } else {
    gen = new AudioGeneratorWAV();
  }
  gen->begin(source, out);
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_PP, INPUT_PULLUP);

  
  // 1. Hafızayı başlat ("audio" isimli bir alan aç)
  prefs.begin("audio", false);
  
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD Hatasi!");
    return;
  }

  scanSD();
  if (trackList.size() == 0) return;

  // 2. Kapatmadan önce hangi parçada kaldığımızı oku
  // Eğer hiç kayıt yoksa 0. parçadan başla
  currentTrackIndex = prefs.getInt("lastTrack", 0);
  
  // Güvenlik: Eğer SD karttaki dosya sayısı değişmişse hata olmasın
  if (currentTrackIndex >= trackList.size()) {
    currentTrackIndex = 0;
  }

  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Kaldığı yerden devam et
  startPlayback(trackList[currentTrackIndex]);
  Serial.printf("Cihaz acildi, %d. parcadan devam ediliyor.\n", currentTrackIndex);
  out->SetGain(0.85);
}

void loop() {
  if (!isPaused && gen && gen->isRunning()) {
    if (!gen->loop()) {
      startPlayback(trackList[currentTrackIndex]);
    }
  }

  unsigned long now = millis();
  if (now - lastButtonPress > 400) {
    if (digitalRead(BTN_NEXT) == LOW) {
      currentTrackIndex = (currentTrackIndex + 1) % trackList.size();
      startPlayback(trackList[currentTrackIndex]);
      lastButtonPress = now;
    }
    else if (digitalRead(BTN_PREV) == LOW) {
      currentTrackIndex--;
      if (currentTrackIndex < 0) currentTrackIndex = trackList.size() - 1;
      startPlayback(trackList[currentTrackIndex]);
      lastButtonPress = now;
    }
    else if (digitalRead(BTN_PP) == LOW) {
      isPaused = !isPaused;
      if (isPaused) {
        out->stop(); 
      } else {
        out->begin();
        out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); 
      }
      lastButtonPress = now;
    }
  }
}