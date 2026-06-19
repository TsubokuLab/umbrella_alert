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

// 現在の都市のタイムゾーン補正(秒)を取得
long getCurrentTimezoneOffset() {
    return CITIES[currentCityIndex].tzOffset;
}

// 都市をインデックスで確定・保存する（UI/Webどちらからでも使える共通セッター）
// 将来、WiFi設定画面のプルダウンから土地を選ぶ際にこの関数を呼ぶ想定。
void setCityByIndex(int idx) {
    if (idx < 0 || idx >= CITY_COUNT) return;
    nextCityIndex = idx;
    currentCityIndex = idx;
    saveSettings();
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
    saveSettings();
    showSettingsScreen();  // 画面を再描画
    M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);
}

#endif // SETTINGS_H