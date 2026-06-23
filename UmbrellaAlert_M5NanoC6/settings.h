// settings.h - 場所設定（都市プリセット / カスタム緯度経度）。画面なし版。

#ifndef SETTINGS_H
#define SETTINGS_H

#include <M5Unified.h>
#include <Preferences.h>
#include "config.h"

// 設定保存用（このTUで一度だけ定義。他ヘッダは extern で参照）
Preferences preferences;

// ===== 都市プリセット =====
enum CityIndex {
    INDEX_SAPPORO,
    INDEX_TOKYO,
    INDEX_OSAKA,
    INDEX_FUKUOKA,
    INDEX_NAHA,
    CITY_COUNT
};

struct CityInfo {
    const char* id;    // OpenWeatherMap ID
    const char* name;  // 表示名
    long tzOffset;     // タイムゾーン補正(秒)
};

const CityInfo CITIES[CITY_COUNT] = {
    {CITY_SAPPORO, "札幌", 9 * 3600},
    {CITY_TOKYO,   "東京", 9 * 3600},
    {CITY_OSAKA,   "大阪", 9 * 3600},
    {CITY_FUKUOKA, "福岡", 9 * 3600},
    {CITY_NAHA,    "那覇", 9 * 3600}
};

// ===== 場所モード（PRESET / CUSTOM）=====
int    currentCityIndex = INDEX_TOKYO;
int    locMode    = 0;   // 0=PRESET, 1=CUSTOM
String customLat  = "";
String customLon  = "";
String customName = "";
long   customTz   = DEFAULT_TIMEZONE_OFFSET;

bool hasCustomLocation() { return customLat.length() > 0 && customLon.length() > 0; }
bool isCustomLocation()  { return locMode == 1 && hasCustomLocation(); }

// 設定のロード
void loadSettings() {
    preferences.begin("weather_app", false);
    currentCityIndex = preferences.getInt("city_index", INDEX_TOKYO);
    if (currentCityIndex < 0 || currentCityIndex >= CITY_COUNT) currentCityIndex = INDEX_TOKYO;
    locMode    = preferences.getInt("loc_mode", 0);
    customLat  = preferences.getString("lat", "");
    customLon  = preferences.getString("lon", "");
    customName = preferences.getString("loc_name", "");
    customTz   = preferences.getLong("tz_offset", DEFAULT_TIMEZONE_OFFSET);
    preferences.end();
}

// 都市インデックスとモードを保存
void saveSettings() {
    preferences.begin("weather_app", false);
    preferences.putInt("city_index", currentCityIndex);
    preferences.putInt("loc_mode", locMode);
    preferences.end();
}

const char* getCurrentCityId() { return CITIES[currentCityIndex].id; }
long getCurrentTimezoneOffset() {
    return isCustomLocation() ? customTz : CITIES[currentCityIndex].tzOffset;
}

// 表示用の場所名（カスタムは入力名、なければ都市名）
String getLocationName() {
    if (isCustomLocation() && customName.length() > 0) return customName;
    return String(CITIES[currentCityIndex].name);
}

// プリセット都市を確定（Web /setcity から呼ぶ）。カスタム座標は消さない。
void setCityByIndex(int idx) {
    if (idx < 0 || idx >= CITY_COUNT) return;
    currentCityIndex = idx;
    locMode = 0;
    saveSettings();
}

// カスタム場所を保存（Web /save から呼ぶ）
void setCustomLocation(const String& lat, const String& lon, const String& name, long tz) {
    preferences.begin("weather_app", false);
    preferences.putInt("loc_mode", 1);
    preferences.putString("lat", lat);
    preferences.putString("lon", lon);
    preferences.putString("loc_name", name);
    preferences.putLong("tz_offset", tz);
    preferences.end();
    locMode = 1; customLat = lat; customLon = lon; customName = name; customTz = tz;
}

// 設定ページの都市ドロップダウン用 <option> 群（現在選択中をselected）
String cityOptionsHtml() {
    String s = "";
    for (int i = 0; i < CITY_COUNT; i++) {
        s += "<option value='" + String(i) + "'";
        if (!isCustomLocation() && i == currentCityIndex) s += " selected";
        s += ">" + String(CITIES[i].name) + "</option>";
    }
    return s;
}

#endif // SETTINGS_H
