// umbrella_alert.ino - お出かけ傘アラート
// 12時間以内の雨予報をチェックして、傘が必要かどうかを表示

#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "settings.h"
#include "weather.h"

// NeoPixel LEDの設定
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(24, M5.Ex_I2C.getSDA(), NEO_GRB + NEO_KHZ800);
int rotation = 0;

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

// LEDアニメーション用の変数
TaskHandle_t ledTaskHandle = NULL;
SemaphoreHandle_t ledSemaphore = NULL;
volatile bool ledAnimationRunning = false;
volatile bool ledWeatherState = false; // false: 晴れ, true: 雨

// 画面モード
enum ScreenMode {
    MAIN_SCREEN,
    DETAIL_SCREEN,
    SETTINGS_SCREEN,
    FORECAST_SCREEN
};

ScreenMode currentScreen = MAIN_SCREEN;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

// ===== 関数プロトタイプ =====
void connectWiFi();
bool checkWeatherForecast();
void updateLEDs();
void reloadWeatherApi();

// ===== セットアップ =====
void setup() {

    width = M5.Display.width();
    height = M5.Display.height();

    // 設定読み込み
    loadSettings();

    // M5Unified初期化
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // シリアル通信
    Serial.begin(115200);
    Serial.println("M5.Ex_I2C.getSDA() : " + String(M5.Ex_I2C.getSDA()));
    
    // LEDの初期化
    pixels.setPin(M5.Ex_I2C.getSDA());
    pixels.begin();
    pixels.setBrightness(LED_BRIGHTNESS);
    pixels.clear();
    for (int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(50, 50, 50)); 
    }
    pixels.show();

    // Wi-Fi接続
    connectWiFi();
    
    // 初回の天気チェック
    reloadWeatherApi();
}

// ===== メインループ =====
void loop() {
    updateLEDs();
    M5.update();
    auto touch = M5.Touch.getDetail();

    if (touch.isPressed()) {
        int touchX = touch.x;
        int touchY = touch.y;
        if( touchY > height - 40 && touchY < height){
            if (touchX > 0 && touchX < width / 3)                   A_Pressed();
            if (touchX > width / 3 * 1 && touchX < width / 3 * 2)   B_Pressed();
            if (touchX > width / 3 * 2 && touchX < width)           C_Pressed();
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
}

// ボタンA押下処理（左ボタン）
void A_Pressed(){
    Serial.println("A Button Pressed.");
    if (currentScreen == MAIN_SCREEN || currentScreen == FORECAST_SCREEN) {
        reloadWeatherApi();
    } else {
        // メイン画面に戻る
        currentScreen = MAIN_SCREEN;
    }
    delay(300);
}

// ボタンB押下処理（中央ボタン）
void B_Pressed(){
    Serial.println("B Button Pressed.");
    if (currentScreen == MAIN_SCREEN || currentScreen == FORECAST_SCREEN) {
        // 詳細画面表示
        currentScreen = DETAIL_SCREEN;
    } else if (currentScreen == SETTINGS_SCREEN) {
        // 都市を決定
        setCurrentCity();
        reloadWeatherApi();
    }
    delay(300);
}

// ボタンC押下処理（右ボタン）
void C_Pressed(){
    Serial.println("C Button Pressed.");
    if (currentScreen == MAIN_SCREEN || currentScreen == DETAIL_SCREEN || currentScreen == FORECAST_SCREEN) {
        currentScreen = SETTINGS_SCREEN;
    } else if (currentScreen == SETTINGS_SCREEN) {
        selectNextCity();
    }
    delay(300);
}

void updateLEDs(){
    int brightness = 100;
    int divisions = 1;
    if (willRain) {
        float _speed = 1.0 * 3.14;
        float _fade = 1.0 + float(sin(millis() * 0.001 * _speed) * 0.5);
        for (int offset = 0; offset <= LED_COUNT; offset++) {
            int pos = (rotation + offset + LED_COUNT) % LED_COUNT;
            float _amp = (abs(offset * divisions % LED_COUNT) / float(LED_COUNT)) * _fade;
            pixels.setPixelColor(pos, 0, brightness * _amp, LED_BRIGHTNESS * _amp);
        }
        rotation = (rotation + 1) % LED_COUNT;
    } else {
        float _speed = 0.5 * 3.14;
        float _amp = 1.0 + float(sin(millis() * 0.001 * _speed) * 0.4);
        for (int i = 0; i < LED_COUNT; i++) {
            pixels.setPixelColor(i, brightness * _amp, brightness / 3 * _amp, 0);
        }
    }
    pixels.show();
    delay(20);
}

// Wi-Fi接続
void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        delay(1000);
    } else {
        delay(3000);
    }
}

// 天気予報を取得して12時間以内に雨が降るかチェック
bool checkWeatherForecast() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
    }
    
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
                weatherIcon = doc["list"][i]["weather"][0]["icon"].as<String>();
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
            currentScreen = MAIN_SCREEN;
            return;
        } else {
            // 失敗時の処理
            retryCount++;
            
            if (retryCount < MAX_RETRIES) {
                // 待機中にボタン操作を受け付ける
                unsigned long startTime = millis();
                bool aborted = false;
                
                while (millis() - startTime < RETRY_DELAY && !aborted) {
                    M5.update();
                    
                    // 残り時間表示
                    unsigned long remainingSecs = (RETRY_DELAY - (millis() - startTime)) / 1000;
                    char timeMsg[32];
                    snprintf(timeMsg, sizeof(timeMsg), "残り時間: %02lu:%02lu", remainingSecs / 60, remainingSecs % 60);

                    // ボタンCを押すと即時再試行
                    if (M5.BtnC.wasPressed() || 
                        (M5.Touch.getDetail().isPressed() && 
                         M5.Touch.getDetail().x > SCREEN_WIDTH*2/3 && 
                         M5.Touch.getDetail().y > SCREEN_HEIGHT - 40)) {
                        break; // ループを抜けて再試行へ
                    }
                    delay(100); // 短いdelayでCPU負荷を軽減
                }
            } else {
                return;
            }
        }
    }
}