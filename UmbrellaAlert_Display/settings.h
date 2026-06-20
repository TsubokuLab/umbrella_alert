// settings.h - 設定画面関連の関数

#ifndef SETTINGS_H
#define SETTINGS_H

#include <M5Unified.h>
#include <Preferences.h>
#include "config.h"

// 都市選択インデックス（画面の行数都合で名古屋は削除。カスタム地点を1行追加するため）
enum CityIndex {
//    INDEX_AUTO = 0,
    INDEX_SAPPORO,
    INDEX_TOKYO,
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
    {CITY_OSAKA,   "大阪",   9 * 3600},
    {CITY_FUKUOKA, "福岡",   9 * 3600},
    {CITY_NAHA,    "那覇",   9 * 3600}
};

// 現在選択されている都市インデックス（プリセット内）
int currentCityIndex = INDEX_TOKYO;  // デフォルトは東京
// 都市設定画面で仮選択中の項目（結合リスト上のインデックス。0=カスタムが存在する場合）
int nextSel = 0;
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

// カスタム場所(緯度経度)が保存されているか（locModeに依らず、座標が存在するか）
bool hasCustomLocation() {
    return customLat.length() > 0 && customLon.length() > 0;
}
bool isCustomLocation() {
    return locMode == 1 && hasCustomLocation();
}

// ===== 都市設定画面の選択リスト（先頭にカスタム地点、その後にプリセット都市） =====
int  selectionOffset() { return hasCustomLocation() ? 1 : 0; }       // カスタム分のずれ
int  selectionCount()  { return selectionOffset() + CITY_COUNT; }    // 表示行数
bool selIsCustom(int sel) { return hasCustomLocation() && sel == 0; }
String selectionName(int sel) {
    if (selIsCustom(sel)) return customName.length() > 0 ? customName : String("カスタム地点");
    return String(CITIES[sel - selectionOffset()].name);
}
// 現在の実際の選択を結合インデックスへ変換
int currentSelectionIndex() {
    if (isCustomLocation()) return 0;  // hasCustom前提でlocMode==1
    return selectionOffset() + currentCityIndex;
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
    nextSel = currentSelectionIndex();
}

// 設定の保存（都市インデックスと場所モード）
void saveSettings() {
    preferences.begin("weather_app", false);
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

// プリセット都市をインデックスで確定・保存する（UI/Webどちらからでも使える共通セッター）
// プリセット都市を選んでも、保存済みのカスタム座標は消さない（選択肢として残す）。
void setCityByIndex(int idx) {
    if (idx < 0 || idx >= CITY_COUNT) return;
    currentCityIndex = idx;
    locMode = 0;  // PRESETへ戻す（customLat/Lonは保持）
    saveSettings();
    nextSel = currentSelectionIndex();
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
    
    canvas.fillScreen(RAINY_COLOR);
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
    
    // 結合リスト（先頭にカスタム地点があればそれ、その後にプリセット都市）を描画
    for (int i = 0; i < selectionCount(); i++) {
        int itemY = startY + i * itemHeight;

        // 選択中のアイテムをハイライト
        uint16_t markerColor;
        if (i == nextSel) {
            canvas.fillRoundRect(40, itemY - 5, width - 80, itemHeight, 5, TFT_DARKGREY);
            canvas.setTextColor(TFT_YELLOW);
            markerColor = TFT_YELLOW;
        } else {
            canvas.setTextColor(selIsCustom(i) ? TFT_CYAN : TFT_WHITE);
            markerColor = TFT_CYAN;
        }

        // カスタム地点には丸マーカーを描画（フォント記号に依存しない）
        if (selIsCustom(i)) {
            canvas.fillCircle(46, itemY + 8, 3, markerColor);
            canvas.drawString(selectionName(i), 56, itemY);
        } else {
            canvas.drawString(selectionName(i), 50, itemY);
        }
    }
    
    // 操作ガイド（左下=戻る / 右下=↓で循環。選択は戻る時に変更があれば自動確定）
    canvas.setTextColor(TFT_WHITE);
    drawVirtualButtons("戻る", "", "↓");
    canvas.pushSprite(&M5.Display, 0, 0);
}

// 設定画面のタッチ処理
void handleSettingsTouch(int x, int y) {
    int startY = 50;
    int itemHeight = 25;
    int width = M5.Display.width();
    
    // リストのタッチ判定
    if (x >= 40 && x <= width - 40) {
        for (int i = 0; i < selectionCount(); i++) {
            int itemY = startY + i * itemHeight;
            if (y >= itemY - 5 && y <= itemY + itemHeight - 5) {
                nextSel = i;
                showSettingsScreen();  // 画面を再描画
                return;
            }
        }
    }
}

// 次の項目を選択（ボタンC押下時）
void selectNextCity() {
    nextSel = (nextSel + 1) % selectionCount();
    showSettingsScreen();  // 画面を再描画
    M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);  // 非ブロッキング。delayは入れない（フリーズ防止）
}

// 仮選択(nextSel)を確定・保存する（ボタンB押下時）
void setCurrentCity() {
    if (selIsCustom(nextSel)) {
        locMode = 1;  // カスタム地点を選択（座標は保存済み）
    } else {
        currentCityIndex = nextSel - selectionOffset();
        locMode = 0;  // プリセット都市（customLat/Lonは保持＝選択肢として残す）
    }
    saveSettings();
    showSettingsScreen();  // 画面を再描画
    M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);
}

#endif // SETTINGS_H