// config.h - ユーザー設定ファイル（M5 NanoC6 / 画面なし・LEDのみ版）

#ifndef CONFIG_H
#define CONFIG_H

// ===== 基本設定 =====
#define APP_TITLE "Umbrella Alert"                     // アプリケーションタイトル
#define APP_VERSION "v0.0.1"                           // アプリケーションバージョン
#define AP_SSID "Umbrella-Alert"                       // アクセスポイント名（設定モード）
#define AP_PASS "12345678"                             // アクセスポイントパスワード
#define AUTHOR_NAME "@kohack_v"                         // 作者名
#define AUTHOR_URL "https://x.com/kohack_v"            // 作者URL

// デバイスの動作モード（画面なしのためScreenModeは無し）
enum DeviceMode {
    SETUP_MODE,   // 設定モード（アクセスポイント＋設定ページ）
    APP_MODE      // 通常動作（天気取得＋LED表示）
};

// ===== シリアル通信設定 =====
#define SERIAL_BAUD_RATE 115200

// ===== ネットワーク設定 =====
#define AP_IP_ADDR IPAddress(192, 168, 4, 1)           // アクセスポイントIP（固定）
#define WEB_SERVER_PORT 80
#define DNS_SERVER_PORT 53
#define DNS_DOMAIN "umbrella"                            // mDNS: http://umbrella.local
#define WIFI_CONNECTION_TIMEOUT 20                      // 接続タイムアウト回数（500ms/回）

// 外部の場所設定ページ（GitHub Pages）。設定ページからこのURL?ip=<本体IP>へ誘導
#define SETUP_PAGE_URL "https://tsubokulab.github.io/umbrella_alert/"

// ===== Wi-Fi / APIキー（secrets.h、リポジトリ非公開）=====
#include "secrets.h"

// ===== OpenWeatherMap API設定 =====
#define UNITS "metric"           // 温度単位 (metric=摂氏)
#define LANGUAGE "ja"            // 言語設定 (ja=日本語)

// ===== 天気チェック設定 =====
#define UPDATE_INTERVAL 1800000  // 更新間隔: 30分 (ミリ秒)
#define RAIN_THRESHOLD 40        // 降水確率のしきい値 (%)
#define FORECAST_CHECK_HOURS 12  // 何時間先までの雨をチェックするか(3時間単位)
#define WEATHER_MAX_RETRIES 3
#define WEATHER_RETRY_DELAY 60000

// タイムゾーン（カスタム未設定時のフォールバック）
#define DEFAULT_TIMEZONE_OFFSET (9 * 3600)

// ===== 都市設定 =====
#define CITY_AUTO "auto"
#define CITY_SAPPORO "2128295"   // 札幌
#define CITY_TOKYO "1850147"     // 東京
#define CITY_OSAKA "1853909"     // 大阪
#define CITY_FUKUOKA "1863967"   // 福岡
#define CITY_NAHA "1856035"      // 那覇
#define DEFAULT_CITY_ID CITY_TOKYO

// ===== ハードウェア設定（NeoPixel LED）=====
#define LED_USE_GROVE_PIN 1      // 1: GroveのSDAピンを自動使用 / 0: LED_PINを使用
#define LED_PIN 2                // NeoPixel LEDのピン番号（LED_USE_GROVE_PIN=0のとき）
#define LED_COUNT 24             // LEDの数
#define LED_BRIGHTNESS 255       // LED明るさ (0-255)
#define LED_UPDATE_INTERVAL 10   // LED更新の間隔(ms)。10ms=100fps
#define LED_ROTATION_PERIOD 1500 // 雨アニメが1周する時間(ms)。fpsとは独立

// ===== 本体内蔵RGB LED（インジケーター。リング未接続でも状態が分かる）=====
// M5 NanoC6 の内蔵RGB LED: データ=GPIO20 / 電源イネーブル=GPIO19(HIGHで点灯可)
// ※点灯しない場合はピン番号を確認して調整
#define ONBOARD_LED_PIN 20
#define ONBOARD_LED_POWER_PIN 19
#define ONBOARD_LED_BRIGHTNESS 60   // インジケーターは控えめに (0-255)

// ===== 設定リセット =====
#define RESET_HOLD_MS 3000       // ボタン長押しでWiFi設定リセット（ミリ秒）

// ===== Web UI（設定ページ）スタイル =====
#define CONTAINER_MAX_WIDTH "480px"
#define BORDER_RADIUS "20px"
#define BUTTON_PADDING "16px"
#define INPUT_PADDING "16px"
#define THEME_PRIMARY_START "#667eea"
#define THEME_PRIMARY_END "#764ba2"
#define THEME_SECONDARY_START "#667eea"
#define THEME_SECONDARY_END "#764ba2"
#define THEME_DANGER_START "#ef4444"
#define THEME_DANGER_END "#dc2626"

#endif // CONFIG_H
