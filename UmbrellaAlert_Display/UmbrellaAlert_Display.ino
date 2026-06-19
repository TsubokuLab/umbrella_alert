// umbrella_alert.ino - お出かけ傘アラート
// 12時間以内の雨予報をチェックして、傘が必要かどうかを表示

#include <LittleFS.h>      // M5Unifiedより前にインクルードする必要がある
#include <M5Unified.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

#include "config.h"
#include "styles.h"
#include "wifi_manager.h"    // WiFi管理機能
#include "web_server.h"      // Webサーバー機能
#include "ui_helper.h"
#include "settings.h"
#include "weather.h"

// NeoPixel LEDの設定
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(24, M5.Ex_I2C.getSDA(), NEO_GRB + NEO_KHZ800);
int rotation = 0;
bool attentionFlg = false;

// タイマー設定
unsigned long lastUpdateTime = 0;

// 状態変数
bool willRain = false;
float rainProbability = 0;
float temperature = 0;
float temperature_min = 0;
float temperature_max = 0;
float humidity = 0;
float windSpeed = 0;
String weatherDescription = "";
String weatherIcon = "";
String cityName = "";

// jsonファイル
DynamicJsonDocument doc(16384);

// ==== グローバル変数 ====
String ssidList;                      // スキャンしたネットワークリスト（HTML形式）
String wifi_ssid;                     // 保存されたWiFi SSID
String wifi_password;                 // 保存されたWiFiパスワード
int networkCount = 0;                 // スキャンしたネットワーク数

int width = 320;                      // ディスプレイ幅
int height = 240;                     // ディスプレイ高さ
M5Canvas canvas(&M5.Display);         // 画面全体描画用キャンバス

// WiFi監視用変数
unsigned long lastWiFiCheck = 0;     // 最後のWiFiチェック時刻
boolean isConnect = false;           // 接続状態フラグ
String connectionStatus = "初期化中"; // 接続状態メッセージ
int reconnectCount = 0;              // 再接続試行回数

DNSServer dnsServer;                 // DNSサーバーインスタンス
WebServer webServer(WEB_SERVER_PORT); // Webサーバーインスタンス
Preferences preferences;             // 設定保存用インスタンス

DeviceMode deviceMode = SETUP_MODE;  // 現在のデバイスモード
ScreenMode currentScreen = MAIN_PAGE; // 画面モード

// ===== 関数プロトタイプ =====
void connectWiFi();
bool checkWeatherForecast();
void updateLEDs();
void reloadWeatherApi();
void drawVirtualButtons(String btnA, String btnB, String btnC);

// メインのディスプレイ描画処理（モードに応じた画面切り替え）
void drawDisplay(){
    M5.Display.startWrite();
    switch(deviceMode){
        case SETUP_MODE:
            drawSetupPage();
            break;
        case APP_MODE:
            //drawMainPage();
            switch(currentScreen){
                case MAIN_PAGE:
                    showMainScreen(willRain, rainProbability, temperature, temperature_min, temperature_max, weatherIcon, cityName);
                    break;
                case FORECAST_PAGE:
                    showForecastScreen(doc, willRain);
                    break;
                case SETTINGS_PAGE:
                    showSettingsScreen();
                    break;
                case DETAIL_PAGE:
                    showDetailScreen(temperature, humidity, windSpeed, weatherDescription);
                    break;
                default:
                    break;
            }
            break;
        case SETTING_MODE:
            drawSettingsPage();
            break;
        default:
            break;
    }
    M5.Display.endWrite();
}

// WiFi未接続時の初期設定画面
void drawSetupPage(){
    Serial.println("セットアップ画面を表示");
    int itemX = 10;
    int itemY = 10;
    int itemHeight = 16 * FONT_SCALE;
    int spacing = 10;
    int qr_size = 80;
    
    canvas.clear(LCD_BG_COLOR);
    canvas.setTextColor(LCD_TEXT_COLOR);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    
    // ヘッダー
    itemY = 10;
    canvas.setTextDatum(MC_DATUM);
    canvas.fillRoundRect(10, 10, 320/2 - 20, 40, 10, SETUP_COLOR);
    itemY += 20;
    canvas.setTextColor(WHITE);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.drawString("設定モード", 320/4, itemY);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    canvas.setTextColor(LCD_TEXT_COLOR);
    itemY += 20;
    itemY += spacing;

    // SSID・パスワード表示
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("SSID: " + String(AP_SSID), itemX, itemY);
    itemY += itemHeight;
    canvas.drawString("PASS: " + String(AP_PASS), itemX, itemY);
    
    // WiFi初期設定方法の見出し
    itemY = 100;
    canvas.setTextDatum(TL_DATUM);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.drawString("■WiFi設定方法", itemX, itemY);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    
    // QRコード描画
    itemY = 125;
    // 1. WiFi接続用QRコード
    canvas.qrcode("WIFI:T:WPA;S:" + String(AP_SSID) + ";P:" + AP_PASS + ";H:false;;", 0, height - qr_size, qr_size, 5);
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("1. WiFiに接続", itemX, itemY);

    // 2. 設定画面用QRコード
    String _setting_url = "http://" + AP_IP_ADDR.toString();
    canvas.qrcode(_setting_url, 320 / 2, height - qr_size, qr_size, 5);
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("2. 設定画面を開く", 320 / 2, itemY);
    itemY += itemHeight;
    canvas.drawString(_setting_url, 320 / 2, itemY);
    canvas.pushSprite(&M5.Display, 0, 0);
}

// アプリのメイン画面描画（ここをカスタマイズしてください）
void drawMainPage(){
    // ここにアプリのメイン画面描画処理などを書く ////////////////////
    // サンプルアプリ画面
    int itemX = 10;
    int itemY = 10;
    int itemHeight = 16 * FONT_SCALE;
    
    canvas.clear(LCD_BG_COLOR);
    canvas.setTextSize(1.0 * FONT_SCALE);
    
    // ヘッダー
    itemY = 10;
    canvas.setTextDatum(MC_DATUM);
    canvas.fillRoundRect(10, 10, width/2 - 20, 40, 10, APP_COLOR);
    itemY += 20;
    canvas.setTextColor(WHITE);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.drawString("アプリ画面", width/4, itemY);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    
    // コンテンツエリア
    itemY = 60;
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(LCD_TEXT_COLOR);
    canvas.drawString("<アプリメイン画面サンプル>", itemX, itemY);
    itemY += itemHeight;
    itemY += itemHeight;
    canvas.drawString("サンプルテキスト", itemX, itemY);
    itemY += itemHeight;
    canvas.drawString("サンプルテキスト", itemX, itemY);
    itemY += itemHeight;
    canvas.drawString("サンプルテキスト", itemX, itemY);

    drawConnectionStatus();
    drawVirtualButtons("設定", "---", "---");
    canvas.pushSprite(&M5.Display, 0, 0);
    ///////////////////////////////////////////////////////////////
}

// 設定画面の描画（WiFi状態確認・管理）
void drawSettingsPage(){
    int itemX = 10;
    int itemY = 10;
    int itemHeight = 16 * FONT_SCALE;
    int spacing = 10;
    int qr_size = 80;

    canvas.clear(LCD_BG_COLOR);
    canvas.setTextColor(LCD_TEXT_COLOR);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    
    // ヘッダー
    itemY = 10;
    canvas.setTextDatum(MC_DATUM);
    canvas.fillRoundRect(10, 10, 320/2 - 20, 40, 10, SETTING_COLOR);
    itemY += 20;   
    canvas.setTextColor(WHITE);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.drawString("設定画面", 320/4, itemY);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    canvas.setTextColor(LCD_TEXT_COLOR);
    itemY += 20;
    itemY += spacing;

    // WiFi接続情報表示
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("ネットワーク: " + String(WiFi.SSID()), itemX, itemY);
    itemY += itemHeight;
    canvas.drawString("信号強度: " + String(WiFi.RSSI()) + "dBm", itemX, itemY);
    itemY += itemHeight;
    canvas.drawString("稼働: " + String(millis() / 1000) + " 秒", itemX, itemY);
    itemY += itemHeight;
    canvas.drawString("IPアドレス: " + WiFi.localIP().toString(), itemX, itemY);
    itemY += itemHeight * 2;
    
    // 設定ページへのQRコード
    String _setting_url = "http://" + WiFi.localIP().toString();
    canvas.qrcode(_setting_url, width - qr_size - spacing, height - 40 - qr_size - spacing, qr_size, 5);
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString("■設定ページ", itemX, itemY);
    itemY += itemHeight;
    canvas.drawString(_setting_url, itemX, itemY);

    drawConnectionStatus();
    drawVirtualButtons("戻る", "再起動", "初期化");
    canvas.pushSprite(&M5.Display, 0, 0);
}

// 接続状態表示の更新（右上の点滅アイコン）
void drawConnectionStatus() {
    if(millis() % 1000 < 500) canvas.fillCircle(320 / 2 + 16, 30, 6, CONNECT_COLOR);
    canvas.setTextDatum(ML_DATUM);
    canvas.setTextColor(CONNECT_COLOR);
    canvas.drawString(connectionStatus, 320 / 2 + 26, 30);
    canvas.setTextColor(LCD_TEXT_COLOR);
}

// ===== セットアップ =====
void setup() {
    // ファイル初期化
    if (!LittleFS.begin(true)) {  // trueを渡すとフォーマットも行う
        Serial.println("LittleFS マウント失敗");
    } else {
        Serial.println("LittleFS マウント成功");
        // LittleFSの内容を確認
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while (file) {
            Serial.print("ファイル: ");
            Serial.println(file.name());
            file = root.openNextFile();
        }
    }

    // 設定読み込み
    loadSettings();

    // M5Unified初期化
    auto cfg = M5.config();
    M5.begin(cfg);
    width = M5.Display.width();
    height = M5.Display.height();
    
    // ディスプレイ初期設定
    M5.Display.setColorDepth(8); // 8bitカラーを指定。必要であれば16に変更。
    M5.Display.clear();
    M5.Display.setCursor(0, 0);
    M5.Display.setBrightness(SCREEN_BRIGHTNESS);  // 画面の明るさを設定
    M5.Display.setFont(&fonts::lgfxJapanGothicP_16);
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.setTextSize(1.0 * FONT_SCALE);
    
    // キャンバス初期設定
    canvas.setColorDepth(8); // 8bitカラーを指定。必要であれば16に変更。
    canvas.createSprite(width, height);
    canvas.clear(LCD_BG_COLOR);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
    
    // シリアル通信
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("M5.Ex_I2C.getSDA() : " + String(M5.Ex_I2C.getSDA()));
    Serial.println("M5.Ex_I2C.getSCL() : " + String(M5.Ex_I2C.getSCL()));

    // 設定保存領域初期化
    preferences.begin("wifi-config");
    delay(10);

    // LCD表示
    M5.Display.println(String(APP_TITLE) + " - " + String(APP_VERSION));
    M5.Display.println("起動中...");

    // LEDの初期化
    pixels.setPin(M5.Ex_I2C.getSDA());
    pixels.begin();
    pixels.setBrightness(LED_BRIGHTNESS);
    pixels.clear();
    for (int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0)); 
    }
    pixels.show();

    // 保存された設定で接続試行
    if (restoreConfig()) {
        if (checkConnection()) {
            // 接続成功: 通常モード
            deviceMode = APP_MODE;
            isConnect = true;
            connectionStatus = "WiFi接続成功";
            M5.Display.println("接続成功 - 稼働モード");
            startWebServer();
            showLoadingScreen();

            // 初回の天気チェック
            reloadWeatherApi();
            drawDisplay();
            return;
        }
        M5.Display.println("接続失敗 - 設定モード");
    }
    // 設定モードで開始
    deviceMode = SETUP_MODE;
    M5.Display.println("設定モードで開始");
    setupMode();
    drawDisplay();
}

// ===== メインループ =====
void loop() {
    // ネットワーク処理
    dnsServer.processNextRequest();
    webServer.handleClient();
    updateLEDs();

    // メイン画面か設定画面時のみ毎回更新
    if(currentScreen == MAIN_PAGE || deviceMode == SETTING_MODE){
        drawDisplay();
    }

    M5.update();
    // タッチスクリーン処理
    auto touch = M5.Touch.getDetail();
    if (touch.isPressed()) {
        int touchX = touch.x;
        int touchY = touch.y;
        if( touchY > height - 40 && touchY < height){
            if (touchX > 0 && touchX < width / 3)                   A_Pressed();
            if (touchX > width / 3 * 1 && touchX < width / 3 * 2)   B_Pressed();
            if (touchX > width / 3 * 2 && touchX < width)           C_Pressed();
        }else if( 0 < touchY && touchY < height - 40){
            // 設定画面のタッチ処理の追加
            if(deviceMode == APP_MODE){
                switch(currentScreen){
                    case MAIN_PAGE:
                        // 予報ページを表示
                        changeScreenMode(FORECAST_PAGE);
                        break;
                    case FORECAST_PAGE:
                        // メイン画面に戻る
                        changeScreenMode(MAIN_PAGE);
                        break;
                    case SETTINGS_PAGE:
                        handleSettingsTouch(touch.x, touch.y);
                        break;
                    case DETAIL_PAGE:
                        
                        break;
                    default:
                        break;
                }

            }
        }
    }
    // 物理ボタン処理
    if (M5.BtnA.wasPressed()) A_Pressed();
    if (M5.BtnB.wasPressed()) B_Pressed();
    if (M5.BtnC.wasPressed()) C_Pressed();
    
    // 定期的に天気を更新
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        reloadWeatherApi();
    }

    delay(20);  // 軽いディレイでCPU負荷軽減
}

// ==== ボタン処理機能 ====
// モード変更処理（効果音付き）
void changeDeviceMode(DeviceMode _mode){
    changeDeviceMode(_mode, true);
}
void changeDeviceMode(DeviceMode _mode, bool _isBeep){
    if(_mode == deviceMode) return;
    switch(_mode){
        case APP_MODE:
            Serial.println("アプリ画面を表示");
            break;
        case SETTING_MODE:
            Serial.println("設定画面を表示");
            break;
        default:
            break;
    }
    deviceMode = _mode;
    drawDisplay();
    if(_isBeep){
        M5.Speaker.tone(3000, 100);
        delay(500);
    }
}

// モード変更処理（効果音付き）
void changeScreenMode(ScreenMode _mode){
    changeScreenMode(_mode, true);
}
void changeScreenMode(ScreenMode _mode, bool _isBeep){
    switch(_mode){
        case MAIN_PAGE:
            Serial.println("メイン画面を表示");
            break;
        case SETTINGS_PAGE:
            Serial.println("設定画面を表示");
            break;
        case DETAIL_PAGE:
            Serial.println("詳細画面を表示");
            break;
        case FORECAST_PAGE:
            Serial.println("天気予報画面を表示");
            break;
        default:
            break;
    }    
    currentScreen = _mode;
    drawDisplay();
    if(_isBeep){
        M5.Speaker.tone(3000, 100);
        delay(500);
    }
}

// ボタンA押下処理（左ボタン）
void A_Pressed(){
    Serial.println("A Button Pressed.");

    switch(deviceMode){
        case APP_MODE:
            switch(currentScreen){
                case MAIN_PAGE:
                case FORECAST_PAGE:
                    // 設定画面へ
                    changeDeviceMode(SETTING_MODE);
                    break;
                case SETTINGS_PAGE:
                    changeScreenMode(MAIN_PAGE);
                    break;
                case DETAIL_PAGE:
                    currentScreen = MAIN_PAGE;
                    changeDeviceMode(SETTING_MODE);
                    break;
                default:
                    break;
            }
            break;
        case SETTING_MODE:
            // アプリメイン画面へ
            changeDeviceMode(APP_MODE);
            break;
        default:
            break;
    }
}

// ボタンB押下処理（中央ボタン）
void B_Pressed(){
    Serial.println("B Button Pressed.");

    switch(deviceMode){
        case APP_MODE:
            switch(currentScreen){
                case MAIN_PAGE:
                case FORECAST_PAGE:
                    // 詳細画面表示
                    changeScreenMode(DETAIL_PAGE);
                    break;
                case SETTINGS_PAGE:
                    // 都市を決定
                    setCurrentCity();
                    showLoadingScreen();
                    reloadWeatherApi();
                    break;
                case DETAIL_PAGE:
                    changeScreenMode(MAIN_PAGE);
                    break;
                default:
                    break;
            }
            break;
        case SETTING_MODE:
            // 再起動
            rebootDevice();
            break;
        default:
            break;
    }
}

// ボタンC押下処理（右ボタン）
void C_Pressed(){
    Serial.println("C Button Pressed.");

    switch(deviceMode){
        case APP_MODE:
            switch(currentScreen){
                case MAIN_PAGE:
                case FORECAST_PAGE:
                case DETAIL_PAGE:
                    changeScreenMode(SETTINGS_PAGE);
                    break;
                case SETTINGS_PAGE:
                    selectNextCity();
                    break;
                default:
                    break;
            }
            break;
        case SETTING_MODE:
            // 初期化
            canvas.clear(LCD_BG_COLOR);
            canvas.setTextColor(LCD_TEXT_COLOR);
            canvas.setTextDatum(MC_DATUM);
            canvas.drawString("設定を初期化します...", width/2, height/2);
            canvas.pushSprite(&M5.Display, 0, 0);
            M5.Speaker.tone(3000, 100);
            delay(1000);
            resetSettings();
            break;
        default:
            break;
    }
}

void updateLEDs(){
    int brightness = 100;
    int divisions = 1;
    if (willRain) {
        // 雨のアニメーション - 青色でLEDが回転しながら明るさが変化
        // 回転する明るい部分（ハイライト）
        float _speed = 1.0 * 3.14;
        float _fade = 1.0 + float(sin(millis() * 0.001 * _speed) * 0.5);
        for (int offset = 0; offset <= LED_COUNT; offset++) {
            int pos = (rotation + offset + LED_COUNT) % LED_COUNT;
            float _amp = (abs(offset * divisions % LED_COUNT) / float(LED_COUNT)) * _fade;
            pixels.setPixelColor(pos, 0, brightness * _amp, LED_BRIGHTNESS * _amp);
        }
        // 次の位置へ回転
        rotation = (rotation + 1) % LED_COUNT;
    } else {
        float _speed = 0.5 * 3.14;
        float _amp = 1.0 + float(sin(millis() * 0.001 * _speed) * 0.4);
        // 晴れのアニメーション - オレンジでゆっくり明滅
        for (int i = 0; i < LED_COUNT; i++) {
            // オレンジ色（赤255、緑128、青0）で明るさを変化
            pixels.setPixelColor(i, brightness * _amp, brightness / 3 * _amp, 0);
        }
    }
    pixels.show();
}

// 天気予報を取得して12時間以内に雨が降るかチェック
bool checkWeatherForecast() {
    if (!checkConnection()) return false;
    
    HTTPClient http;
    String url;
    const char* cityId = getCurrentCityId();
    
    // 自動モードとそれ以外で分岐
    if (strcmp(cityId, CITY_AUTO) == 0) {
        // IPアドレスから自動判定するモード
        url = "http://api.openweathermap.org/data/2.5/forecast?q=ip&units=" + 
              String(UNITS) + "&lang=" + String(LANGUAGE) + "&appid=" + String(API_KEY);
    } else {
        // 通常の都市ID指定モード
        url = "http://api.openweathermap.org/data/2.5/forecast?id=" + String(cityId) + 
              "&units=" + String(UNITS) + "&lang=" + String(LANGUAGE) + "&appid=" + String(API_KEY);
    }

    Serial.println("OpenWeatherMapに接続 : " + url);
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        
        Serial.println(payload);
        // JSONデータの解析
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.println("JSON解析エラー");
            return false;
        }
        
        // 12時間（4つの3時間枠）の予報をチェック
        willRain = false;
        rainProbability = -1;
        weatherIcon = "";
        temperature_min = 100;
        temperature_max = -100;
        
        // 最初の予報から詳細情報を取得
        temperature = doc["list"][0]["main"]["temp"].as<float>();
        humidity = doc["list"][0]["main"]["humidity"].as<float>();
        windSpeed = doc["list"][0]["wind"]["speed"].as<float>();
        weatherDescription = doc["list"][0]["weather"][0]["description"].as<String>();
        cityName = doc["city"]["name"].as<String>();

        for (int i = 0; i < 4; i++) {
            String weatherMain = doc["list"][i]["weather"][0]["main"].as<String>();
            float pop = doc["list"][i]["pop"].as<float>() * 100;  // 降水確率（%）
            float t_min = doc["list"][i]["main"]["temp_min"].as<float>();  // 最低気温
            float t_max = doc["list"][i]["main"]["temp_max"].as<float>();  // 最高気温
            Serial.println(doc["list"][i]["dt_txt"].as<String>() + ", weather : " + weatherMain + ", rainProbability : " + String(rainProbability) + ", pop : " + String(pop) + ", icon : " + doc["list"][i]["weather"][0]["icon"].as<String>());
            
            // 降水確率更新
            if(rainProbability < pop){
                Serial.println("降水確率更新");
                rainProbability = pop;
                weatherIcon = "/" + doc["list"][i]["weather"][0]["icon"].as<String>() + ".png";
                //weatherIcon = doc["list"][i]["weather"][0]["icon"].as<String>();
            }
            if(temperature_min > t_min){
                temperature_min = t_min;
            }
            if(temperature_max < t_max){
                temperature_max = t_max;
            }

            if (weatherMain == "Rain" || weatherMain == "Drizzle" || weatherMain == "Thunderstorm" || pop >= RAIN_THRESHOLD) {
                willRain = true;
            }
        }
        lastUpdateTime = millis();
        return true;
    } else {
        http.end();
        Serial.println("API接続エラー");
        return false;
    }
}

void reloadWeatherApi(){
    // 最大試行回数
    const int MAX_RETRIES = 3;
    const unsigned long RETRY_DELAY = 1000 * 60; // 1分
    
    int retryCount = 0;
    bool success = false;
    
    // 天気情報の取得を試みる
    while (!success && retryCount < MAX_RETRIES) {
        success = checkWeatherForecast();
        
        if (success) {
            // 成功時の処理
            changeScreenMode(MAIN_PAGE, false);
            return;
        } else {
            // 失敗時の処理
            retryCount++;
            if (retryCount < MAX_RETRIES) {
                // リトライメッセージを表示
                canvas.fillScreen(TFT_RED);
                canvas.setTextColor(TFT_WHITE);
                canvas.setTextDatum(MC_DATUM);
                canvas.setTextSize(1.0 * FONT_SCALE);
                
                canvas.drawString("天気情報の取得に失敗しました", width/2, height/2 + 20);
                canvas.drawString("1分後に再試行します...", width/2, height/2);

                drawVirtualButtons("中止", "---", "再試行");
                
                // 待機中にボタン操作を受け付ける
                unsigned long startTime = millis();
                bool aborted = false;
                
                while (millis() - startTime < RETRY_DELAY && !aborted) {
                    M5.update();
                    
                    // 残り時間表示
                    unsigned long remainingSecs = (RETRY_DELAY - (millis() - startTime)) / 1000;
                    char timeMsg[32];
                    snprintf(timeMsg, sizeof(timeMsg), "残り時間: %02lu:%02lu", remainingSecs / 60, remainingSecs % 60);
                    
                    // 前の時間表示を消去
                    canvas.setTextDatum(TC_DATUM);
                    canvas.fillRect(0, height/2 + 40, width, 30, TFT_RED);
                    canvas.drawString(timeMsg, width/2, height/2 + 40);
                    canvas.pushSprite(&M5.Display, 0, 0);
                    
                    // ボタンAを押すと中止
                    if (M5.BtnA.wasPressed() || 
                        (M5.Touch.getDetail().isPressed() && 
                         M5.Touch.getDetail().x < width/3 && 
                         M5.Touch.getDetail().y > height - 40)) {
                        aborted = true;
                        showErrorScreen("天気情報の取得をキャンセルしました");
                        return;
                    }
                    
                    // ボタンCを押すと即時再試行
                    if (M5.BtnC.wasPressed() || 
                        (M5.Touch.getDetail().isPressed() && 
                         M5.Touch.getDetail().x > width*2/3 && 
                         M5.Touch.getDetail().y > height - 40)) {
                        break; // ループを抜けて再試行へ
                    }
                    delay(100); // 短いdelayでCPU負荷を軽減
                }
                
                // 次のリトライのためにローディング画面を表示
                if (!aborted) {
                    showLoadingScreen();
                }
            } else {
                // 最大試行回数に達した場合
                showErrorScreen("天気情報の取得に失敗しました\n更新ボタンで再試行");
                return;
            }
        }
    }
}

// 再起動処理
void rebootDevice(){
    Serial.println("再起動します");
    canvas.clear(LCD_BG_COLOR);
    canvas.setTextColor(LCD_TEXT_COLOR);
    canvas.setTextDatum(MC_DATUM);
    canvas.drawString("再起動中...", width/2, height/2);
    canvas.pushSprite(&M5.Display, 0, 0);
    M5.Speaker.tone(3000, 500);
    delay(1000);
    ESP.restart();
}