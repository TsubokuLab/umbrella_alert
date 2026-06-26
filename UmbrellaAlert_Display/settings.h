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

// ===== 都道府県プリセット（代表＝県庁所在地の座標）。Web本体ページの場所選択用（NanoC6と共通仕様）=====
struct Pref { const char* name; const char* lat; const char* lon; };
const Pref PREFS[] = {
    {"北海道",   "43.064", "141.347"}, {"青森県",   "40.824", "140.740"},
    {"岩手県",   "39.704", "141.153"}, {"宮城県",   "38.269", "140.872"},
    {"秋田県",   "39.719", "140.102"}, {"山形県",   "38.240", "140.364"},
    {"福島県",   "37.750", "140.468"}, {"茨城県",   "36.342", "140.447"},
    {"栃木県",   "36.566", "139.884"}, {"群馬県",   "36.391", "139.061"},
    {"埼玉県",   "35.857", "139.649"}, {"千葉県",   "35.605", "140.123"},
    {"東京都",   "35.690", "139.692"}, {"神奈川県", "35.448", "139.643"},
    {"新潟県",   "37.902", "139.023"}, {"富山県",   "36.695", "137.211"},
    {"石川県",   "36.595", "136.626"}, {"福井県",   "36.065", "136.222"},
    {"山梨県",   "35.664", "138.568"}, {"長野県",   "36.651", "138.181"},
    {"岐阜県",   "35.391", "136.722"}, {"静岡県",   "34.977", "138.383"},
    {"愛知県",   "35.180", "136.907"}, {"三重県",   "34.730", "136.509"},
    {"滋賀県",   "35.005", "135.869"}, {"京都府",   "35.021", "135.756"},
    {"大阪府",   "34.694", "135.502"}, {"兵庫県",   "34.691", "135.183"},
    {"奈良県",   "34.685", "135.833"}, {"和歌山県", "34.226", "135.168"},
    {"鳥取県",   "35.504", "134.238"}, {"島根県",   "35.472", "133.051"},
    {"岡山県",   "34.662", "133.935"}, {"広島県",   "34.385", "132.455"},
    {"山口県",   "34.186", "131.471"}, {"徳島県",   "34.066", "134.559"},
    {"香川県",   "34.340", "134.043"}, {"愛媛県",   "33.842", "132.766"},
    {"高知県",   "33.560", "133.531"}, {"福岡県",   "33.607", "130.418"},
    {"佐賀県",   "33.249", "130.299"}, {"長崎県",   "32.745", "129.874"},
    {"熊本県",   "32.790", "130.742"}, {"大分県",   "33.238", "131.613"},
    {"宮崎県",   "31.911", "131.424"}, {"鹿児島県", "31.560", "130.558"},
    {"沖縄県",   "26.212", "127.681"}
};
const int PREF_COUNT = sizeof(PREFS) / sizeof(PREFS[0]);

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

// ===== カスタム場所スロット（地図で指定した場所を都道府県選択とは別に保持。NanoC6と共通仕様）=====
// 都道府県に切り替えてもこのスロットは消さず、「カスタム」を選べば復元できる。
String custSlotLat  = "";
String custSlotLon  = "";
String custSlotName = "";
long   custSlotTz   = DEFAULT_TIMEZONE_OFFSET;

// 雨の通知: 直近何時間先までの予報で傘判定するか（1〜24h。既定はconfigのFORECAST_CHECK_HOURS）
int    notifyHours = FORECAST_CHECK_HOURS;

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
    // カスタム場所スロット（都道府県選択とは別保持）
    custSlotLat  = preferences.getString("cust_lat", "");
    custSlotLon  = preferences.getString("cust_lon", "");
    custSlotName = preferences.getString("cust_name", "");
    custSlotTz   = preferences.getLong("cust_tz", DEFAULT_TIMEZONE_OFFSET);
    // ビープ音量
    beepVolIndex = preferences.getInt("beep_idx", defaultBeepIndex());
    if (beepVolIndex < 0 || beepVolIndex >= BEEP_PRESET_COUNT) beepVolIndex = defaultBeepIndex();
    // 雨の通知（チェック時間）
    notifyHours = preferences.getInt("notify_hrs", FORECAST_CHECK_HOURS);
    if (notifyHours < 1 || notifyHours > 24) notifyHours = FORECAST_CHECK_HOURS;
    preferences.end();
    nextSel = currentSelectionIndex();
}

// ===== 雨の通知（チェック時間） =====
int  getNotifyHours() { return notifyHours; }
// 予報は3時間刻みなので、通知時間→チェック枠数（切り上げ・最低1枠）
int  notifyForecastSlots() { int s = (notifyHours + 2) / 3; return s < 1 ? 1 : s; }
void setNotifyHours(int h) {
    if (h < 1) h = 1; if (h > 24) h = 24;
    preferences.begin("weather_app", false);
    preferences.putInt("notify_hrs", h);
    preferences.end();
    notifyHours = h;
}
// 設定ページの「雨の通知」ドロップダウン用 <option> 群（現在値をselected）
String notifyHoursOptionsHtml() {
    String s = "";
    for (int h = 1; h <= 24; h++) {
        s += "<option value='" + String(h) + "'";
        if (h == notifyHours) s += " selected";
        s += ">" + String(h) + " 時間</option>";
    }
    return s;
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

// アクティブな場所をCUSTOM(緯度経度)として保存（内部用。都道府県/カスタム復元の共通処理）
void setActiveCustomLocation(const String& lat, const String& lon, const String& name, long tz) {
    preferences.begin("weather_app", false);
    preferences.putInt("loc_mode", 1);
    preferences.putString("lat", lat);
    preferences.putString("lon", lon);
    preferences.putString("loc_name", name);
    preferences.putLong("tz_offset", tz);
    preferences.end();
    locMode = 1; customLat = lat; customLon = lon; customName = name; customTz = tz;
}

// カスタム場所を保存（地図ページの /save から）。アクティブ＋カスタムスロットの両方に保存。
void setCustomLocation(const String& lat, const String& lon, const String& name, long tz) {
    preferences.begin("weather_app", false);
    preferences.putString("cust_lat", lat);
    preferences.putString("cust_lon", lon);
    preferences.putString("cust_name", name);
    preferences.putLong("cust_tz", tz);
    preferences.end();
    custSlotLat = lat; custSlotLon = lon; custSlotName = name; custSlotTz = tz;
    setActiveCustomLocation(lat, lon, name, tz);
}

// ===== Web本体ページの場所選択（都道府県47＋カスタム復元。NanoC6と共通仕様）=====
String getLocationName() {
    if (isCustomLocation()) return customName.length() > 0 ? customName : String("カスタム地点");
    return String(CITIES[currentCityIndex].name);
}
bool hasCustomSlot()  { return custSlotLat.length() > 0 && custSlotLon.length() > 0; }
bool isCustomActive() { return isCustomLocation() && hasCustomSlot() && customLat == custSlotLat && customLon == custSlotLon; }

// 保存済みカスタムスロットをアクティブに復元（プルダウンで「カスタム」を選んだとき）
void applyCustomSlot() {
    if (!hasCustomSlot()) return;
    setActiveCustomLocation(custSlotLat, custSlotLon, custSlotName, custSlotTz);
}
// 都道府県を選択（県庁所在地の座標をアクティブに。カスタムスロットは残す）
void setPrefecture(int idx) {
    if (idx < 0 || idx >= PREF_COUNT) return;
    setActiveCustomLocation(PREFS[idx].lat, PREFS[idx].lon, PREFS[idx].name, DEFAULT_TIMEZONE_OFFSET);
}
// 現在のアクティブ場所が都道府県プリセットと一致するindex（カスタム/無一致は-1）
int currentPrefIndex() {
    if (isCustomActive()) return -1;
    for (int i = 0; i < PREF_COUNT; i++) if (isCustomLocation() && customName == PREFS[i].name) return i;
    return -1;
}
// プルダウンの「現在値」(JSの初期値比較用)。カスタム時は "-1"
String currentPrefValue() { return isCustomActive() ? String("-1") : String(currentPrefIndex()); }
// 都道府県ドロップダウン用 <option> 群。カスタムスロットがあれば先頭に「カスタム（地名）」
String prefOptionsHtml() {
    String s = "";
    if (hasCustomSlot()) {
        String cn = custSlotName.length() > 0 ? custSlotName : String("地図設定");
        s += "<option value='-1'";
        if (isCustomActive()) s += " selected";
        s += ">カスタム（" + cn + "）</option>";
    }
    int cur = currentPrefIndex();
    for (int i = 0; i < PREF_COUNT; i++) {
        s += "<option value='" + String(i) + "'";
        if (i == cur) s += " selected";
        s += ">" + String(PREFS[i].name) + "</option>";
    }
    return s;
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