#include "FS.h"
#include <LittleFS.h>
#include <M5Unified.h>

constexpr auto SUN_PATH = "/sun-icon.png";
constexpr auto MOON_PATH = "/moon-icon.png";
constexpr auto UMBRELLA_PATH = "/umbrella-icon.png";
constexpr auto CLOUD_PATH = "/cloud-icon.png";
constexpr auto DAY_CLOUD_PATH = "/day-cloud-icon.png";
constexpr auto NIGHT_CLOUD_PATH = "/night-cloud-icon.png";

void listFiles() {
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  
  while(file) {
    Serial.print("File: ");
    Serial.print(file.name());
    Serial.print(" Size: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  M5.begin();  // M5Unifiedの初期化
  
  if(!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }
  
  Serial.println("LittleFS mounted successfully");
  
  // LittleFSに保存されているファイル一覧を表示
  listFiles();
}

void loop() {
  M5.update();
}