// UmbrellaAlert_M5NanoC6.ino - おでかけ傘アラート（M5 NanoC6 / 画面なし・LEDのみ版）
// 12時間以内の雨予報をチェックし、リングLEDで「傘が必要か」を知らせる。
// 初期設定は本体のアクセスポイントにスマホで接続し、ブラウザから行う。

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <M5Unified.h>

#include "config.h"
#include "styles.h"
#include "settings.h"      // preferences・場所設定を定義
#include "wifi_manager.h"
#include "web_server.h"

// ===== NeoPixel LED =====
Adafruit_NeoPixel pixels  = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);          // 外付けリング
Adafruit_NeoPixel onboard = Adafruit_NeoPixel(1, ONBOARD_LED_PIN, NEO_GRB + NEO_KHZ800);          // 本体内蔵RGB（インジケーター）
int rotation = 0;

// ===== 状態 =====
DeviceMode deviceMode = SETUP_MODE;
bool  willRain = false;
float rainProbability = 0;
DynamicJsonDocument doc(16384);
unsigned long lastUpdateTime = 0;

// LED表示の状態
enum LedState { LED_CONNECTING, LED_SETUP, LED_APP, LED_RESET_ARMING, LED_RESETTING, LED_OFF };
volatile int ledState = LED_CONNECTING;
volatile unsigned long resetHeldMs = 0;  // ボタン長押しの経過時間（リセット進捗表示用）

// ===== Web/WiFi 用グローバル（各ヘッダから extern 参照）=====
String ssidList;
String wifi_ssid;
String wifi_password;
int networkCount = 0;
unsigned long lastWiFiCheck = 0;
DNSServer dnsServer;
WebServer webServer(WEB_SERVER_PORT);

// ===== プロトタイプ =====
bool checkWeatherForecast();
void reloadWeatherApi();
void updateLEDs();
void ledTask(void* param);
void rebootDevice();

// ===== セットアップ =====
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    // 設定読み込み（場所・モード）
    loadSettings();

    // M5Unified初期化
    auto cfg = M5.config();
    M5.begin(cfg);

    // 個体固有のSSID/mDNSホスト名を確定（複数台運用のため）
    initDeviceIdentity();

    // LED初期化（ピンはGrove自動 or 固定）
#if LED_USE_GROVE_PIN
    pixels.setPin(M5.Ex_I2C.getSDA());
#else
    pixels.setPin(LED_PIN);
#endif
    pixels.begin();
    pixels.setBrightness(LED_BRIGHTNESS);
    pixels.clear();
    pixels.show();

    // 本体内蔵RGB LED（インジケーター）初期化。電源イネーブルをHIGHに
    pinMode(ONBOARD_LED_POWER_PIN, OUTPUT);
    digitalWrite(ONBOARD_LED_POWER_PIN, HIGH);
    onboard.begin();
    onboard.setBrightness(ONBOARD_LED_BRIGHTNESS);
    onboard.clear();
    onboard.show();

    // LED更新を別コアの専用タスクで回す（描画/通信にブロックされない）
    ledState = LED_CONNECTING;
    xTaskCreatePinnedToCore(ledTask, "ledTask", 4096, NULL, 1, NULL, 0);

    // 保存済みWiFiで接続を試みる
    if (restoreConfig() && checkConnection()) {
        // 接続成功 → 通常モード
        deviceMode = APP_MODE;
        startWebServer();
        MDNS.begin(g_mdnsHost.c_str());  // http://umbrella-alert-xxxx.local
        Serial.println("通常モード: http://" + g_mdnsHost + ".local  IP=" + WiFi.localIP().toString());
        ledState = LED_APP;
        reloadWeatherApi();
    } else {
        // 未設定 or 接続失敗 → 設定モード（アクセスポイント）
        deviceMode = SETUP_MODE;
        setupMode();
        ledState = LED_SETUP;
    }
}

// ===== メインループ =====
void loop() {
    if (deviceMode == SETUP_MODE) dnsServer.processNextRequest();
    webServer.handleClient();

    M5.update();
    // ボタン長押しでWiFi設定をリセット。押している間はインジケーターで進捗を表示。
    static unsigned long btnDown = 0;
    static int prevLedState = LED_CONNECTING;
    if (M5.BtnA.wasPressed()) { btnDown = millis(); prevLedState = ledState; }
    if (M5.BtnA.isPressed() && btnDown) {
        resetHeldMs = millis() - btnDown;
        if (resetHeldMs >= RESET_HOLD_MS) {
            ledState = LED_RESETTING;   // 確定: 赤を1秒点灯
            delay(1000);
            ledState = LED_OFF;         // 消灯してから再起動
            delay(150);
            resetSettings();            // WiFi設定をクリアして再起動
        } else {
            ledState = LED_RESET_ARMING; // 押下中: 赤の進捗表示
        }
    }
    if (M5.BtnA.wasReleased()) {
        if (ledState == LED_RESET_ARMING) ledState = prevLedState;  // 途中で離した → 元の状態へ
        btnDown = 0;
        resetHeldMs = 0;
    }

    // 定期的に天気を更新（通常モードのみ）
    if (deviceMode == APP_MODE) {
        unsigned long now = millis();
        if (now - lastUpdateTime >= UPDATE_INTERVAL) {
            lastUpdateTime = now;
            reloadWeatherApi();
        }
    }

    delay(5);
}

// 内蔵RGB LED（インジケーター）を状態に応じて点灯
// 原則: 点滅=操作待機 / 呼吸・点灯=正常
void updateOnboard() {
    uint8_t r = 0, g = 0, b = 0;
    switch (ledState) {
        case LED_OFF:
            break;
        case LED_SETUP: {  // 未設定/AP待ち: 白の呼吸（やや速め）
            float a = 0.25 + 0.75 * (0.5 + 0.5 * sin(millis() * 0.004));
            r = 255 * a; g = 255 * a; b = 255 * a;
        } break;
        case LED_CONNECTING: {  // 接続中(待機): 白点滅
            if ((millis() / 250) % 2 == 0) { r = 255; g = 255; b = 255; }
        } break;
        case LED_RESET_ARMING: {  // 長押し中(待機): 赤点滅 0.25秒
            if ((millis() / 250) % 2 == 0) r = 255;
        } break;
        case LED_RESETTING: {  // 確定: 赤点灯
            r = 255;
        } break;
        default: {  // LED_APP（正常）: 呼吸。雨=青 / 晴=オレンジ
            float a = 0.25 + 0.75 * (0.5 + 0.5 * sin(millis() * 0.0015));
            if (willRain) { b = 255 * a; }
            else          { r = 255 * a; g = 120 * a; }
        } break;
    }
    onboard.setPixelColor(0, r, g, b);
    onboard.show();
}

// ===== LED表示（外付けリング ＋ 内蔵インジケーター）=====
void updateLEDs() {
    const int brightness = 100;

    if (ledState == LED_OFF) {
        pixels.clear();

    } else if (ledState == LED_RESET_ARMING) {
        // リセット長押し中(待機): 赤点滅 0.25秒
        bool on = (millis() / 250) % 2 == 0;
        for (int i = 0; i < LED_COUNT; i++) pixels.setPixelColor(i, on ? 180 : 0, 0, 0);

    } else if (ledState == LED_RESETTING) {
        // リセット確定: 赤点灯
        for (int i = 0; i < LED_COUNT; i++) pixels.setPixelColor(i, 200, 0, 0);

    } else if (ledState == LED_SETUP) {
        // 設定モード: シアンのゆっくり呼吸
        float amp = 0.25 + 0.75 * (0.5 + 0.5 * sin(millis() * 0.002));
        for (int i = 0; i < LED_COUNT; i++) pixels.setPixelColor(i, 0, brightness * amp, brightness * amp);

    } else if (ledState == LED_CONNECTING) {
        // 接続中: 黄色のコメットが回る
        int pos = (millis() / 60) % LED_COUNT;
        for (int i = 0; i < LED_COUNT; i++) pixels.setPixelColor(i, 0, 0, 0);
        for (int t = 0; t < 5; t++) {
            int p = (pos - t + LED_COUNT) % LED_COUNT;
            float f = 1.0 - t * 0.2;
            pixels.setPixelColor(p, brightness * f, brightness * 0.6 * f, 0);
        }

    } else {  // LED_APP
        if (willRain) {
            // 雨: 青の回転（時間ベース）
            float _fade = 1.0 + float(sin(millis() * 0.001 * 3.14) * 0.5);
            rotation = (int)(((float)(millis() % LED_ROTATION_PERIOD) / LED_ROTATION_PERIOD) * LED_COUNT) % LED_COUNT;
            for (int offset = 0; offset <= LED_COUNT; offset++) {
                int pos = (rotation + offset) % LED_COUNT;
                float _amp = (abs(offset % LED_COUNT) / float(LED_COUNT)) * _fade;
                pixels.setPixelColor(pos, 0, brightness * _amp, LED_BRIGHTNESS * _amp);
            }
        } else {
            // 晴れ: オレンジのゆっくり明滅
            float _amp = 1.0 + float(sin(millis() * 0.001 * 0.5 * 3.14) * 0.4);
            for (int i = 0; i < LED_COUNT; i++) pixels.setPixelColor(i, brightness * _amp, brightness / 3 * _amp, 0);
        }
    }
    pixels.show();

    // 本体内蔵LED（インジケーター）も同じ状態を表示
    updateOnboard();
}

// LED更新タスク（別コア）
void ledTask(void* param) {
    for (;;) {
        updateLEDs();
        vTaskDelay(pdMS_TO_TICKS(LED_UPDATE_INTERVAL));
    }
}

// ===== 天気取得 =====
bool checkWeatherForecast() {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    const String common = "&units=" + String(UNITS) + "&lang=" + String(LANGUAGE) + "&appid=" + String(API_KEY);
    // 場所は常に緯度経度で指定（都道府県プリセットも地図も lat/lon に統一）
    String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + customLat + "&lon=" + customLon + common;

    Serial.println("OpenWeatherMapに接続 : " + url);
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        http.end();
        Serial.println("API接続エラー");
        return false;
    }

    String payload = http.getString();
    http.end();

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println("JSON解析エラー");
        return false;
    }

    // 直近 FORECAST_CHECK_HOURS（3時間刻み）の雨をチェック
    willRain = false;
    rainProbability = -1;
    const int slots = FORECAST_CHECK_HOURS / 3;
    for (int i = 0; i < slots; i++) {
        String weatherMain = doc["list"][i]["weather"][0]["main"].as<String>();
        float pop = doc["list"][i]["pop"].as<float>() * 100;  // 降水確率(%)
        if (rainProbability < pop) rainProbability = pop;
        if (weatherMain == "Rain" || weatherMain == "Drizzle" || weatherMain == "Thunderstorm" || pop >= RAIN_THRESHOLD) {
            willRain = true;
        }
    }
    Serial.printf("willRain=%d / 最大降水確率=%.0f%% / 場所=%s\n",
                  willRain, rainProbability, getLocationName().c_str());
    lastUpdateTime = millis();
    return true;
}

void reloadWeatherApi() {
    for (int i = 0; i < WEATHER_MAX_RETRIES; i++) {
        if (checkWeatherForecast()) {
            ledState = LED_APP;
            return;
        }
        delay(2000);  // 軽いリトライ間隔（次回はUPDATE_INTERVAL後）
    }
    Serial.println("天気取得に失敗（次回更新まで待機）");
}

// 再起動
void rebootDevice() {
    Serial.println("再起動します");
    delay(500);
    ESP.restart();
}
