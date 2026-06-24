// settings.h - 場所設定（画面なし版）
// 場所はすべて緯度経度(lat/lon)で扱う。都道府県プリセット or 地図でのカスタム指定。

#ifndef SETTINGS_H
#define SETTINGS_H

#include <M5Unified.h>
#include <Preferences.h>
#include "config.h"

// 設定保存用（このTUで一度だけ定義。他ヘッダは extern で参照）
Preferences preferences;

// ===== 都道府県プリセット（代表＝県庁所在地の座標）=====
struct Pref { const char* name; const char* lat; const char* lon; };
const Pref PREFS[] = {
    {"北海道",   "43.064", "141.347"},
    {"青森県",   "40.824", "140.740"},
    {"岩手県",   "39.704", "141.153"},
    {"宮城県",   "38.269", "140.872"},
    {"秋田県",   "39.719", "140.102"},
    {"山形県",   "38.240", "140.364"},
    {"福島県",   "37.750", "140.468"},
    {"茨城県",   "36.342", "140.447"},
    {"栃木県",   "36.566", "139.884"},
    {"群馬県",   "36.391", "139.061"},
    {"埼玉県",   "35.857", "139.649"},
    {"千葉県",   "35.605", "140.123"},
    {"東京都",   "35.690", "139.692"},
    {"神奈川県", "35.448", "139.643"},
    {"新潟県",   "37.902", "139.023"},
    {"富山県",   "36.695", "137.211"},
    {"石川県",   "36.595", "136.626"},
    {"福井県",   "36.065", "136.222"},
    {"山梨県",   "35.664", "138.568"},
    {"長野県",   "36.651", "138.181"},
    {"岐阜県",   "35.391", "136.722"},
    {"静岡県",   "34.977", "138.383"},
    {"愛知県",   "35.180", "136.907"},
    {"三重県",   "34.730", "136.509"},
    {"滋賀県",   "35.005", "135.869"},
    {"京都府",   "35.021", "135.756"},
    {"大阪府",   "34.694", "135.502"},
    {"兵庫県",   "34.691", "135.183"},
    {"奈良県",   "34.685", "135.833"},
    {"和歌山県", "34.226", "135.168"},
    {"鳥取県",   "35.504", "134.238"},
    {"島根県",   "35.472", "133.051"},
    {"岡山県",   "34.662", "133.935"},
    {"広島県",   "34.385", "132.455"},
    {"山口県",   "34.186", "131.471"},
    {"徳島県",   "34.066", "134.559"},
    {"香川県",   "34.340", "134.043"},
    {"愛媛県",   "33.842", "132.766"},
    {"高知県",   "33.560", "133.531"},
    {"福岡県",   "33.607", "130.418"},
    {"佐賀県",   "33.249", "130.299"},
    {"長崎県",   "32.745", "129.874"},
    {"熊本県",   "32.790", "130.742"},
    {"大分県",   "33.238", "131.613"},
    {"宮崎県",   "31.911", "131.424"},
    {"鹿児島県", "31.560", "130.558"},
    {"沖縄県",   "26.212", "127.681"}
};
const int PREF_COUNT = sizeof(PREFS) / sizeof(PREFS[0]);

// ===== 現在の場所（lat/lon/name/tz）。未設定時は東京を既定に =====
String customLat  = "";
String customLon  = "";
String customName = "";
long   customTz   = DEFAULT_TIMEZONE_OFFSET;

// 雨の通知: 直近何時間先までの予報で傘判定するか（1〜24h。既定はconfigのFORECAST_CHECK_HOURS）
int    notifyHours = FORECAST_CHECK_HOURS;

bool hasCustomLocation() { return customLat.length() > 0 && customLon.length() > 0; }

// 設定のロード（未設定なら東京を既定に）
void loadSettings() {
    preferences.begin("weather_app", false);
    customLat  = preferences.getString("lat", "");
    customLon  = preferences.getString("lon", "");
    customName = preferences.getString("loc_name", "");
    customTz   = preferences.getLong("tz_offset", DEFAULT_TIMEZONE_OFFSET);
    notifyHours = preferences.getInt("notify_hrs", FORECAST_CHECK_HOURS);
    if (notifyHours < 1 || notifyHours > 24) notifyHours = FORECAST_CHECK_HOURS;
    preferences.end();
    if (!hasCustomLocation()) {  // 初回など未設定 → 東京
        customLat = "35.690"; customLon = "139.692"; customName = "東京都"; customTz = DEFAULT_TIMEZONE_OFFSET;
    }
}

long   getCurrentTimezoneOffset() { return customTz; }
String getLocationName() { return customName.length() > 0 ? customName : String("東京都"); }

// 場所を保存（地図ページの /save、都道府県選択の両方から使う）
void setCustomLocation(const String& lat, const String& lon, const String& name, long tz) {
    preferences.begin("weather_app", false);
    preferences.putString("lat", lat);
    preferences.putString("lon", lon);
    preferences.putString("loc_name", name);
    preferences.putLong("tz_offset", tz);
    preferences.end();
    customLat = lat; customLon = lon; customName = name; customTz = tz;
}

// 都道府県を選択（その県庁所在地の座標で保存）
void setPrefecture(int idx) {
    if (idx < 0 || idx >= PREF_COUNT) return;
    setCustomLocation(PREFS[idx].lat, PREFS[idx].lon, PREFS[idx].name, DEFAULT_TIMEZONE_OFFSET);
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

// 設定ページの都道府県ドロップダウン用 <option> 群（現在地と一致するものをselected）
String prefOptionsHtml() {
    String s = "";
    for (int i = 0; i < PREF_COUNT; i++) {
        s += "<option value='" + String(i) + "'";
        if (customName == PREFS[i].name) s += " selected";
        s += ">" + String(PREFS[i].name) + "</option>";
    }
    return s;
}

#endif // SETTINGS_H
