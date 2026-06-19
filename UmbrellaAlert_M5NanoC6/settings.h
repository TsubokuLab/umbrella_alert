// settings.h - 設定画面関連の関数

#ifndef SETTINGS_H
#define SETTINGS_H

#include <M5Unified.h>
#include <Preferences.h>
#include "config.h"

// 設定保存用
Preferences preferences;

// 都市選択インデックス
enum CityIndex {
//    INDEX_AUTO = 0,
    INDEX_SAPPORO,
    INDEX_TOKYO,
    INDEX_NAGOYA,
    INDEX_OSAKA,
    INDEX_FUKUOKA,
    INDEX_NAHA,
    CITY_COUNT
};

// 都市情報
struct CityInfo {
    const char* id;   // OpenWeatherMap ID
    const char* name; // 表示名
};

// 都市リスト
const CityInfo CITIES[CITY_COUNT] = {
    // {CITY_AUTO, "自動検出"},
    {CITY_SAPPORO, "札幌"},
    {CITY_TOKYO, "東京"},
    {CITY_NAGOYA, "名古屋"},
    {CITY_OSAKA, "大阪"},
    {CITY_FUKUOKA, "福岡"},
    {CITY_NAHA, "那覇"}
};

// 現在選択されている都市インデックス
int currentCityIndex = INDEX_TOKYO;  // デフォルトは東京
int nextCityIndex = INDEX_TOKYO;  // デフォルトは東京

// 設定のロード
void loadSettings() {
    preferences.begin("weather_app", false);
    currentCityIndex = preferences.getInt("city_index", INDEX_TOKYO);
    if (currentCityIndex < 0 || currentCityIndex >= CITY_COUNT) {
        currentCityIndex = INDEX_TOKYO;  // 範囲外の場合はデフォルトに戻す
    }
    nextCityIndex = currentCityIndex;
    preferences.end();
}

// 設定の保存
void saveSettings() {
    preferences.begin("weather_app", false);
    currentCityIndex = nextCityIndex;
    preferences.putInt("city_index", currentCityIndex);
    preferences.end();
}

// 現在の都市IDを取得
const char* getCurrentCityId() {
    return CITIES[currentCityIndex].id;
}

// 次の都市を選択（ボタンC押下時）
void selectNextCity() {
  nextCityIndex = (nextCityIndex + 1) % CITY_COUNT;
}

void setCurrentCity() {
  currentCityIndex = nextCityIndex;
  saveSettings();
}

#endif // SETTINGS_H