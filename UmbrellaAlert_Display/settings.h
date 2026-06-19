// settings.h - 設定画面関連の関数

#ifndef SETTINGS_H
#define SETTINGS_H

#include <M5Unified.h>
#include <Preferences.h>
#include "config.h"

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
    long tzOffset;    // タイムゾーン補正(秒)。例: 日本=+9時間=32400
};

// 都市リスト（将来Web設定から土地を選ぶ際の唯一の情報源。天気IDとTZをまとめて保持）
const CityInfo CITIES[CITY_COUNT] = {
    // {CITY_AUTO, "自動検出", DEFAULT_TIMEZONE_OFFSET},
    {CITY_SAPPORO, "札幌",   9 * 3600},
    {CITY_TOKYO,   "東京",   9 * 3600},
    {CITY_NAGOYA,  "名古屋", 9 * 3600},
    {CITY_OSAKA,   "大阪",   9 * 3600},
    {CITY_FUKUOKA, "福岡",   9 * 3600},
    {CITY_NAHA,    "那覇",   9 * 3600}
};

// 現在選択されている都市インデックス
int currentCityIndex = INDEX_TOKYO;  // デフォルトは東京
int nextCityIndex = INDEX_TOKYO;  // デフォルトは東京
extern M5Canvas canvas;
extern ScreenMode currentScreen;
extern void drawVirtualButtons(String btnA, String btnB, String btnC);

// ===== 場所モード（ハイブリッド: プリセット都市 or カスタム緯度経度）=====
// locMode: 0=PRESET（都市リスト） / 1=CUSTOM（緯度経度）
int    locMode    = 0;
String customLat  = "";
String customLon  = "";
String customName = "";
long   customTz   = DEFAULT_TIMEZONE_OFFSET;

bool isCustomLocation() {
    return locMode == 1 && customLat.length() > 0 && customLon.length() > 0;
}

// ===== ビープ音量（プリセットをタップで循環・保存）=====
const int BEEP_PRESETS[] = { BEEP_SMALL, BEEP_MEDIUM, BEEP_LARGE, BEEP_OFF };
const char* const BEEP_LABELS[] = { "小", "中", "大", "OFF" };
#define BEEP_PRESET_COUNT 4
int beepVolIndex = 0;  // 既定はloadSettingsでconfigのBEEP_VOLUMEから決定

// config の BEEP_VOLUME に一致するプリセット番号（未一致なら0=小）
int defaultBeepIndex() {
    for (int i = 0; i < BEEP_PRESET_COUNT; i++) {
        if (BEEP_PRESETS[i] == BEEP_VOLUME) return i;
    }
    return 0;
}
int         currentBeepVolume() { return BEEP_PRESETS[beepVolIndex]; }
const char* currentBeepLabel()  { return BEEP_LABELS[beepVolIndex]; }
void        applyBeepVolume()   { M5.Speaker.setVolume(currentBeepVolume()); }

// 設定のロード
void loadSettings() {
    preferences.begin("weather_app", false);
    currentCityIndex = preferences.getInt("city_index", INDEX_TOKYO);
    if (currentCityIndex < 0 || currentCityIndex >= CITY_COUNT) {
        currentCityIndex = INDEX_TOKYO;  // 範囲外の場合はデフォルトに戻す
    }
    nextCityIndex = currentCityIndex;
    // 場所モード・カスタム座標
    locMode    = preferences.getInt("loc_mode", 0);
    customLat  = preferences.getString("lat", "");
    customLon  = preferences.getString("lon", "");
    customName = preferences.getString("loc_name", "");
    customTz   = preferences.getLong("tz_offset", DEFAULT_TIMEZONE_OFFSET);
    // ビープ音量
    beepVolIndex = preferences.getInt("beep_idx", defaultBeepIndex());
    if (beepVolIndex < 0 || beepVolIndex >= BEEP_PRESET_COUNT) beepVolIndex = defaultBeepIndex();
    preferences.end();
}

// 設定の保存（都市インデックスと場所モード）
void saveSettings() {
    preferences.begin("weather_app", false);
    currentCityIndex = nextCityIndex;
    preferences.putInt("city_index", currentCityIndex);
    preferences.putInt("loc_mode", locMode);
    preferences.end();
}

// 現在の都市IDを取得
const char* getCurrentCityId() {
    return CITIES[currentCityIndex].id;
}

// 現在のタイムゾーン補正(秒)を取得（カスタム優先）
long getCurrentTimezoneOffset() {
    return isCustomLocation() ? customTz : CITIES[currentCityIndex].tzOffset;
}

// 都市をインデックスで確定・保存する（UI/Webどちらからでも使える共通セッター）
// プリセット都市を選ぶとカスタムモードは解除される。
void setCityByIndex(int idx) {
    if (idx < 0 || idx >= CITY_COUNT) return;
    nextCityIndex = idx;
    currentCityIndex = idx;
    locMode = 0;  // PRESETへ戻す
    saveSettings();
}

// カスタム場所（緯度経度・地名・TZ）を保存する（Web /save から呼ぶ）
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

// ビープ音量を循環（小→中→大→OFF→…）して保存・反映する
void cycleBeepVolume() {
    beepVolIndex = (beepVolIndex + 1) % BEEP_PRESET_COUNT;
    preferences.begin("weather_app", false);
    preferences.putInt("beep_idx", beepVolIndex);
    preferences.end();
    applyBeepVolume();
    if (currentBeepVolume() > 0) M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);  // 確認音
}

// 設定画面表示
void showSettingsScreen() {
    currentScreen = SETTINGS_PAGE;
    int width = M5.Display.width();
    int height = M5.Display.height();
    
    canvas.fillScreen(TFT_NAVY);
    canvas.setTextColor(TFT_WHITE);
    
    // ヘッダー
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(TC_DATUM);
    canvas.setFont(&fonts::lgfxJapanGothicP_24);
    canvas.drawString("都市設定", width/2, 10);
    
    // 都市リスト表示
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(TL_DATUM);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    
    int startY = 50;
    int itemHeight = 25;
    
    for (int i = 0; i < CITY_COUNT; i++) {
        int itemY = startY + i * itemHeight;
        
        // 選択中のアイテムをハイライト
        if (i == nextCityIndex) {
            canvas.fillRoundRect(40, itemY - 5, width - 80, itemHeight, 5, TFT_DARKGREY);
            canvas.setTextColor(TFT_YELLOW);
        } else {
            canvas.setTextColor(TFT_WHITE);
        }
        
        // 都市名表示
        canvas.drawString(CITIES[i].name, 50, itemY);
    }
    
    // 操作ガイド
    canvas.setTextColor(TFT_WHITE);
    drawVirtualButtons("戻る", "選択", "↓");
    canvas.pushSprite(&M5.Display, 0, 0);
}

// 設定画面のタッチ処理
void handleSettingsTouch(int x, int y) {
    int startY = 50;
    int itemHeight = 25;
    int width = M5.Display.width();
    
    // 都市リストのタッチ判定
    if (x >= 40 && x <= width - 40) {
        for (int i = 0; i < CITY_COUNT; i++) {
            int itemY = startY + i * itemHeight;
            if (y >= itemY - 5 && y <= itemY + itemHeight - 5) {
                // 都市を選択
                nextCityIndex = i;
                showSettingsScreen();  // 画面を再描画
                return;
            }
        }
    }
}

// 次の都市を選択（ボタンC押下時）
void selectNextCity() {
    nextCityIndex = (nextCityIndex + 1) % CITY_COUNT;
    showSettingsScreen();  // 画面を再描画
    M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);  // 非ブロッキング。delayは入れない（フリーズ防止）
}

void setCurrentCity() {
    currentCityIndex = nextCityIndex;
    locMode = 0;  // プリセット都市を確定 → カスタムモード解除
    saveSettings();
    showSettingsScreen();  // 画面を再描画
    M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);
}

#endif // SETTINGS_H