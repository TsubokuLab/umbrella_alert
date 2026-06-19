// config.h - ユーザー設定ファイル

#ifndef CONFIG_H
#define CONFIG_H

// ===== 画面設定 =====
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_BRIGHTNESS 255
#define FONT_SCALE 1.0

// ===== Wi-Fi設定 =====
#define WIFI_SSID "telnet"         // Wi-FiのSSID
#define WIFI_PASSWORD "19871222" // Wi-Fiのパスワード

// ===== OpenWeatherMap API設定 =====
#define API_KEY "e46d15cf638864ba0b4e5dee3b873f2d"   // OpenWeatherMapのAPIキー
#define CITY_ID "1850147"        // 都市ID (例: 1850147=東京)
#define UNITS "metric"           // 温度単位 (metric=摂氏)
#define LANGUAGE "ja"            // 言語設定 (ja=日本語)

// ===== 天気チェック設定 =====
#define UPDATE_INTERVAL 1800000  // 更新間隔: 30分 (ミリ秒)
#define RAIN_THRESHOLD 40        // 降水確率のしきい値 (%)

// ===== 都市設定 =====
// 都市ID（OpenWeatherMapのcity ID）
#define CITY_AUTO "auto"         // 自動（IPアドレスから判定）
#define CITY_SAPPORO "2128295"   // 札幌
#define CITY_TOKYO "1850147"     // 東京
#define CITY_NAGOYA "1856057"    // 名古屋
#define CITY_OSAKA "1853909"     // 大阪
#define CITY_FUKUOKA "1863967"   // 福岡
#define CITY_NAHA "1856035"      // 那覇

// デフォルト設定
#define DEFAULT_CITY_ID CITY_TOKYO

// ===== ハードウェア設定 =====
#define LED_PIN 32               // NeoPixel LEDのピン番号 (Core2: 32)
#define LED_COUNT 24             // LEDの数
#define LED_BRIGHTNESS 255        // LED明るさ (0-255)

// ===== 色設定 =====
#define SUNNY_COLOR M5.Display.color565(240, 240, 210)  // 晴れの背景色
#define RAINY_COLOR M5.Display.color565(40, 62, 81)  // 雨の背景色
#define MAX_COLOR M5.Display.color565(255,115,60)  // 最高気温色
#define MIN_COLOR M5.Display.color565(100,170,255)  // 最低気温色
#define DARK_COLOR M5.Display.color565(55, 71, 79)  // ダークグレー
#define NAVY_COLOR M5.Display.color565(0, 51, 102)  // ネイビー
#define WARNING_COLOR M5.Display.color565(0,220,255)  // 警告色

// ===== 画像設定 =====
constexpr auto SUN_PATH = "/sun-icon.png";
constexpr auto MOON_PATH = "/moon-icon.png";
constexpr auto UMBRELLA_PATH = "/umbrella-icon.png";
constexpr auto CLOUD_PATH = "/cloud-icon.png";
constexpr auto DAY_CLOUD_PATH = "/day-cloud-icon.png";
constexpr auto NIGHT_CLOUD_PATH = "/night-cloud-icon.png";
// Designed by Anindyanfitri / Freepik - http://www.freepik.com
// OpenWeatherMapのアイコンパス : https://openweathermap.org/img/wn/10d.png
constexpr auto WEATHER_ICON_PATH = "https://openweathermap.org/img/wn/";

#endif // CONFIG_H