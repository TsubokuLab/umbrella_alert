// config.h - ユーザー設定ファイル

#ifndef CONFIG_H
#define CONFIG_H

// ===== 基本設定 =====
#define APP_TITLE "傘通知アプリ"                    // アプリケーションタイトル
#define APP_VERSION "v0.0.1"                           // アプリケーションバージョン
#define AP_SSID "Umbrella-Alert"                         // アクセスポイント名
#define AP_PASS "12345678"                             // アクセスポイントパスワード
#define AUTHOR_NAME "@kohack_v"                         // 作者名
#define AUTHOR_URL "https://x.com/kohack_v"            // 作者URL

enum DeviceMode {
    SETUP_MODE,      // セットアップモード（アクセスポイント）
    APP_MODE,        // アプリモード（通常動作）
    SETTING_MODE     // 設定モード（状態確認・管理）
};
enum ScreenMode {
    MAIN_PAGE,
    DETAIL_PAGE,
    SETTINGS_PAGE,
    FORECAST_PAGE
};

// ===== シリアル通信設定 =====
#define SERIAL_BAUD_RATE 115200                      // シリアル通信速度

// ===== ネットワーク設定 =====
#define AP_IP_ADDR IPAddress(192, 168, 4, 1)           // アクセスポイントIP
#define WEB_SERVER_PORT 80                             // Webサーバーポート
#define DNS_SERVER_PORT 53                             // DNSサーバーポート
#define DNS_DOMAIN "umbrella"                            // ローカルドメイン(http://device.local等でアクセスできる)
#define WIFI_CONNECTION_TIMEOUT 20                     // WiFi接続タイムアウト回数（500ms/回）
#define WIFI_CHECK_INTERVAL 5000                       // WiFi接続確認間隔（ミリ秒）
#define WIFI_RECONNECT_ATTEMPTS 10                     // 再接続試行回数

// ===== UI設定 =====
#define CONTAINER_MAX_WIDTH "480px"                     // 設定画面最大幅
#define BORDER_RADIUS "20px"                            // 角の丸み
#define BUTTON_PADDING "16px"                           // ボタンパディング
#define INPUT_PADDING "16px"                            // 入力フィールドパディング

// ===== M5Stack LCD設定 =====
#define SCREEN_BRIGHTNESS 255                                 // 画面の明るさ 0-255
#define FONT_SCALE 1.0                                        // フォントサイズ調整用
#define LCD_TEXT_COLOR M5.Display.color565(0, 0, 0)           // LCD文字色（黒）
#define LCD_BG_COLOR M5.Display.color565(255, 255, 255)       // LCD背景色（白）
#define BTN_BG_COLOR M5.Display.color565(70, 93, 106)         // ボタン背景色（ネイビー）
#define BTN_TEXT_COLOR M5.Display.color565(255, 255, 255)     // ボタンテキスト色（白）
#define SETUP_COLOR M5.Display.color565(195, 87, 87)  	      // セットアップ画面ヘッダー色（赤）
#define APP_COLOR M5.Display.color565(52, 200, 190)  	        // アプリ画面ヘッダー色（緑）
#define SETTING_COLOR M5.Display.color565(116, 86, 177)  	    // 設定画面ヘッダー色（紫）
#define CONNECT_COLOR M5.Display.color565(68, 220, 140)  	    // 接続時（緑）
#define DISCONNECT_COLOR M5.Display.color565(195, 87, 87)  	  // 切断時（赤）

// ===== カラーテーマ設定 =====
// テーマ1: ブルー・パープル（デフォルト）
#define THEME_PRIMARY_START "#667eea"
#define THEME_PRIMARY_END "#764ba2"
#define THEME_SECONDARY_START "#667eea"
#define THEME_SECONDARY_END "#764ba2"
#define THEME_DANGER_START "#ef4444"
#define THEME_DANGER_END "#dc2626"

// ===== OpenWeatherMap API設定 =====
#define API_KEY "e46d15cf638864ba0b4e5dee3b873f2d"   // OpenWeatherMapのAPIキー
#define CITY_ID "1850147"        // 都市ID (例: 1850147=東京)
#define UNITS "metric"           // 温度単位 (metric=摂氏)
#define LANGUAGE "ja"            // 言語設定 (ja=日本語)

// ===== 天気チェック設定 =====
#define UPDATE_INTERVAL 1800000  // 更新間隔: 30分 (ミリ秒)
#define RAIN_THRESHOLD 40        // 降水確率のしきい値 (%)
#define FORECAST_CHECK_HOURS 12  // 何時間先までの雨をチェックするか(3時間単位)。12=直近4枠
#define FORECAST_DISPLAY_COUNT 6 // 予報画面に表示する行数

// 天気取得リトライ
#define WEATHER_MAX_RETRIES 3        // 取得失敗時の最大リトライ回数
#define WEATHER_RETRY_DELAY 60000    // リトライ間隔 (ミリ秒)

// タイムゾーン（都市未設定時のフォールバック。通常は都市ごとの設定を使用）
#define DEFAULT_TIMEZONE_OFFSET (9 * 3600)  // 既定のUTC補正(秒)。日本=+9時間

// ===== ビープ音設定 =====
// 音量プリセット（大・中・小・OFF）。M5.Speaker の音量(0-255)に対応。
#define BEEP_OFF    0
#define BEEP_SMALL  32
#define BEEP_MEDIUM 96
#define BEEP_LARGE  200
// 使用する音量をここで選択（BEEP_OFF / BEEP_SMALL / BEEP_MEDIUM / BEEP_LARGE）
#define BEEP_VOLUME BEEP_MEDIUM
#define BEEP_FREQ 3000           // 周波数 (Hz)
#define BEEP_DURATION 100        // 長さ (ミリ秒)

// ===== ループ/描画タイミング設定 =====
// 重い全画面再描画を毎ループ行うとタッチ判定が間引かれて反応が鈍くなるため、
// 入力サンプリング(LOOP_DELAY)と描画(DRAW_INTERVAL)・LED更新(LED_UPDATE_INTERVAL)を分離する。
#define LOOP_DELAY 5             // メインループの遅延(ms)。小さいほどタッチ反応が良い

// 画面再描画: 静止画面では再描画せず、アニメ(雨時の縁点滅・設定画面)時のみ
// DRAW_INTERVAL間隔で再描画する。縁点滅を表示し切れる必要があるため、
// WARNING_BLINK_INTERVAL より十分小さい値にすること（推奨: 半分以下）。
#define DRAW_INTERVAL 125            // 画面再描画の最小間隔(ms)。大きいほど省電力
#define WARNING_BLINK_INTERVAL 250   // 雨時の画面縁点滅の点灯/消灯の切替間隔(ms)
#define STATUS_BLINK_INTERVAL 500    // 設定画面の接続インジケータ点滅の切替間隔(ms)

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

// ===== ハードウェア設定（NeoPixel LED）=====
// LED_PINは既定で M5.Ex_I2C.getSDA()（Groveポート）を使う。
// 固定ピンを使いたい場合は LED_USE_GROVE_PIN を 0 にして LED_PIN を有効化する。
#define LED_USE_GROVE_PIN 1      // 1: GroveのSDAピンを自動使用 / 0: LED_PINを使用
#define LED_PIN 32               // NeoPixel LEDのピン番号 (LED_USE_GROVE_PIN=0 のとき有効)
#define LED_COUNT 24             // LEDの数（リング/テープの実装数に合わせる）
#define LED_BRIGHTNESS 255       // LED明るさ (0-255)

// LED更新と雨アニメの回転速度（fpsと回転速度を分離）
#define LED_UPDATE_INTERVAL 10   // LED更新の間隔(ms)。10ms=100fps（90fps以上を維持）
#define LED_ROTATION_PERIOD 1500 // 雨アニメが1周する時間(ms)。更新fpsとは無関係（小=速い回転）

// ===== 色設定 =====
#define SUNNY_COLOR M5.Display.color565(240, 240, 210)  // 晴れの背景色
#define RAINY_COLOR M5.Display.color565(40, 62, 81)  // 雨の背景色
#define MAX_COLOR M5.Display.color565(255,115,60)  // 最高気温色
#define MIN_COLOR M5.Display.color565(100,170,255)  // 最低気温色
#define DARK_COLOR M5.Display.color565(55, 71, 79)  // ダークグレー
#define NAVY_COLOR M5.Display.color565(0, 51, 102)  // ネイビー
#define WARNING_COLOR M5.Display.color565(0,220,255)  // 警告色

// ===== 画像設定 =====
//constexpr auto SUN_PATH = "/sun-icon.png";
//constexpr auto MOON_PATH = "/moon-icon.png";
//constexpr auto UMBRELLA_PATH = "/umbrella-icon.png";
//constexpr auto CLOUD_PATH = "/cloud-icon.png";
//constexpr auto DAY_CLOUD_PATH = "/day-cloud-icon.png";
//constexpr auto NIGHT_CLOUD_PATH = "/night-cloud-icon.png";
// Designed by Anindyanfitri / Freepik - http://www.freepik.com
// OpenWeatherMapのアイコンパス : https://openweathermap.org/img/wn/10d.png
constexpr auto WEATHER_ICON_PATH = "https://openweathermap.org/img/wn/";

#endif // CONFIG_H