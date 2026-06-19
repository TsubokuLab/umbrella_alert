// ui_helper.h - UI関連のヘルパー関数

#ifndef UI_HELPER_H
#define UI_HELPER_H

#include <LittleFS.h>      // M5Unifiedより前にインクルードする必要がある
#include <M5Unified.h>
#include "weather.h"
#include "config.h"
#include <TimeLib.h>

extern int width;
extern int height;
extern M5Canvas canvas;
extern ScreenMode currentScreen;

// 関数プロトタイプ宣言
void showMainScreen(bool willRain, float rainProbability, float temperature, float temp_min, float temp_max, String _iconName, String cityName);
void showDetailScreen(float temperature, float humidity, float windSpeed, String weatherDesc);
void showLoadingScreen();
void showErrorScreen(String message);
bool drawPngFromLittleFS(const String& path, int16_t x, int16_t y);
bool drawPngFromLittleFS(const char* path, int16_t x, int16_t y);

// 仮想ボタンの描画（画面下部のタッチボタン）
void drawVirtualButtons(String btnA, String btnB, String btnC) {
    int btm_width = (width + 1) / 3;
    // ボタン背景
    canvas.fillRoundRect(btm_width * 0, height - 40, btm_width - 1, 40, 10, BTN_BG_COLOR);
    canvas.fillRoundRect(btm_width * 1, height - 40, btm_width - 1, 40, 10, BTN_BG_COLOR);
    canvas.fillRoundRect(btm_width * 2, height - 40, btm_width - 1, 40, 10, BTN_BG_COLOR);
    // ボタンラベル
    canvas.setTextColor(BTN_TEXT_COLOR);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(MC_DATUM);
    canvas.drawString(btnA, width / 6 * 1, height - 20);
    canvas.drawString(btnB, width / 6 * 3, height - 20);
    canvas.drawString(btnC, width / 6 * 5, height - 20);
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
}

void showMainScreen(bool willRain, float rainProbability, float temperature, float temp_min, float temp_max, String _iconName, String cityName){
    currentScreen = MAIN_PAGE;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240
    
    String _iconPath = _iconName;
    // if(willRain){
    //     _iconPath = UMBRELLA_PATH;
    // }else{
    //     // 天気アイコンを選択
    //     String _numStr = _iconName;
    //     _numStr.replace("d", "");
    //     _numStr.replace("n", "");
    //     if(_iconName.indexOf("d") != -1){
    //         // 昼
    //         if(_numStr == "01" || _numStr == "02"){
    //             _iconPath = SUN_PATH;
    //         }else if(_numStr == "03" || _numStr == "04"){
    //             _iconPath = DAY_CLOUD_PATH;
    //         }else if(_numStr == "09" || _numStr == "10" || _numStr == "11" || _numStr == "13"){
    //             _iconPath = CLOUD_PATH;
    //         }
    //     }
    //     if(_iconName.indexOf("n") != -1){
    //         // 夜
    //         if(_numStr == "01" || _numStr == "02"){
    //             _iconPath = SUN_PATH;
    //         }else if(_numStr == "03" || _numStr == "04"){
    //             _iconPath = DAY_CLOUD_PATH;
    //         }else if(_numStr == "09" || _numStr == "10" || _numStr == "11" || _numStr == "13"){
    //             _iconPath = CLOUD_PATH;
    //         }
    //     }
    //     _iconPath = _iconName;
    // }
    // Serial.println("Path: " + _iconPath);

    canvas.fillScreen(willRain ? RAINY_COLOR : SUNNY_COLOR);
    canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);
    
    // 傘の必要性を大きく表示
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(TC_DATUM);
    canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);

    canvas.setFont(&fonts::lgfxJapanGothicP_40);
    if (willRain) {
        canvas.drawString("傘が必要です", width/2, 10);
    } else {
        canvas.drawString("傘は不要です", width/2, 10);
    }
    // PNG表示を試みる
    drawPngFromLittleFS(_iconPath, width/2 - 64 - 70, height/2 - 64 - 0);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);

    // 都市名表示（追加部分）
    if (cityName != "") {
        canvas.setTextSize(1.0 * FONT_SCALE);
        canvas.setTextDatum(TC_DATUM);
        canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);
        canvas.drawString(cityName, width - 80, 60);
    }

    // 温度表示
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(MR_DATUM);
    canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);
    canvas.setFont(&fonts::lgfxJapanGothicP_40);
    canvas.drawString(String(temperature,1) + "℃", width - 20, height - 130);
    canvas.setFont(&fonts::lgfxJapanGothicP_24);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(MAX_COLOR);
    canvas.drawString(String((int)temp_max) + "℃", width - 120, height - 90); // " + String(temp_max) + "℃"
    canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);
    canvas.drawString("/", width - 80, height - 90);
    canvas.setTextColor(MIN_COLOR);
    canvas.drawString(String((int)temp_min) + "℃", width - 40, height - 90);

    // 降水確率表示
    canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(BR_DATUM);
    char probStr[24];
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    snprintf(probStr, sizeof(probStr), "降水確率: %.0f%%", rainProbability);
    canvas.drawString(probStr, width - 20, height - 50);
    
    // 注視枠アニメーション
    if(willRain && millis() % 500 < 250){
        int _bold = 8; // ライン太さ
        canvas.fillRect(0, 0, _bold, height - 40, WARNING_COLOR);
        canvas.fillRect(width - _bold, 0, _bold, height - 40, WARNING_COLOR);
        canvas.fillRect(0, 0, width, _bold, WARNING_COLOR);
        canvas.fillRect(0, height - 40 - _bold, width, _bold, WARNING_COLOR);
    }

    // 操作ガイド
    drawVirtualButtons("設定", "詳細", "都市");
    canvas.pushSprite(&M5.Display, 0, 0);
}

// 詳細画面表示
void showDetailScreen(float temperature, float humidity, float windSpeed, String weatherDesc) {
    currentScreen = DETAIL_PAGE;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

    canvas.fillScreen(TFT_NAVY);
    canvas.setTextColor(TFT_WHITE);
    
    // ヘッダー
    canvas.setTextDatum(TC_DATUM);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setFont(&fonts::lgfxJapanGothicP_24);
    canvas.drawString("天気詳細", width / 2, 10);
    
    // 詳細情報
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setCursor(20, 50);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.printf("天気状態: %s", weatherDesc.c_str());
    
    canvas.setCursor(20, 80);
    canvas.printf("気温: %.1f°C", temperature);
    
    canvas.setCursor(20, 110);
    canvas.printf("湿度: %.0f%%", humidity);
    
    canvas.setCursor(20, 140);
    canvas.printf("風速: %.1fm/s", windSpeed);
    
    // 操作ガイド
    drawVirtualButtons("設定", "詳細", "都市");

    canvas.pushSprite(&M5.Display, 0, 0);
}

void showForecastScreen(DynamicJsonDocument doc, bool willRain){
    currentScreen = FORECAST_PAGE;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

    String cityName = doc["city"]["name"].as<String>();

    canvas.fillScreen(willRain ? RAINY_COLOR : SUNNY_COLOR);
    
    // 時間情報表示
    canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);

    // ヘッダー
    canvas.setTextDatum(TC_DATUM);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setFont(&fonts::lgfxJapanGothicP_24);
    canvas.drawString("今後の天気 : " + cityName, width / 2, 10);

    // 詳細情報
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(TL_DATUM);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);

    int startY = 45;
    int itemHeight = 25;
    for (int i = 0; i < 6; i++) {
        String main = doc["list"][i]["weather"][0]["main"].as<String>();
        //String description = doc["list"][0]["weather"][0]["description"].as<String>();
        String _iconPath = doc["list"][i]["weather"][0]["icon"].as<String>();
        float pop = doc["list"][i]["pop"].as<float>() * 100;  // 降水確率（%）
        float temp = doc["list"][i]["main"]["temp_min"].as<float>();  // 気温
        float t_min = doc["list"][i]["main"]["temp_min"].as<float>();  // 最低気温
        float t_max = doc["list"][i]["main"]["temp_max"].as<float>();  // 最高気温

        // 時間
        time_t unixTime = doc["list"][i]["dt"].as<int>();

        // 構造体に展開
        tmElements_t tm;
        breakTime(unixTime + 9 * 3600, tm); // + 9*3600 日本時間
        char buf[20];
        sprintf(buf, "%d/%02d", tm.Month, tm.Day);
        String _timeStr = String(buf);

        int itemY = startY + i * itemHeight;

        canvas.setTextColor(willRain ? TFT_WHITE : NAVY_COLOR);
        canvas.setTextDatum(TL_DATUM);
        canvas.drawString(_timeStr, 20, itemY);
        canvas.setTextDatum(TR_DATUM);
        canvas.drawString(String(tm.Hour) + "時", 120, itemY);
        // PNG表示を試みる
        drawPngFromLittleFS("/resize/" + _iconPath + ".png", 122, itemY);
        canvas.setTextDatum(TC_DATUM);
        canvas.drawString(translate_weather(main), 175, itemY);
        canvas.setTextDatum(TL_DATUM);
        canvas.drawString(String(int(temp)) + "℃", 210, itemY);
        canvas.setTextDatum(TR_DATUM);
        canvas.drawString("" + String(int(pop)) + "%", width - 10, itemY);
    }
    
    // 操作ガイド
    drawVirtualButtons("設定", "詳細", "都市");

    canvas.pushSprite(&M5.Display, 0, 0);
}

// ローディング画面表示
void showLoadingScreen() {
    Serial.println("ローディング画面を表示");
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

    canvas.fillScreen(TFT_BLACK);
    canvas.setCursor(50, 100);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextColor(TFT_WHITE);
    canvas.println("天気情報を取得中...");
    
    canvas.fillCircle(160, 150, 5, TFT_WHITE);
    canvas.pushSprite(&M5.Display, 0, 0);

    // ローディングアニメーション
    // for (int i = 0; i < 10; i++) {
    //     canvas.fillCircle(160, 150, 5, TFT_WHITE);
    //     delay(100);
    //     canvas.fillCircle(160, 150, 5, TFT_BLACK);
    //     delay(100);
    // }
}

// エラー画面表示
void showErrorScreen(String message) {
    Serial.println("エラー画面を表示");
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

    canvas.fillScreen(TFT_RED);
    canvas.setCursor(20, 100);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextColor(TFT_WHITE);
    canvas.println(message);
    canvas.println("\nタップして再試行");
    
    // 操作ガイド
    drawVirtualButtons("再試行", "---", "---");

    canvas.pushSprite(&M5.Display, 0, 0);
}

// String型または const char* 型を受け付ける関数（オーバーロード）
bool drawPngFromLittleFS(const String& path, int16_t x, int16_t y) {
    return drawPngFromLittleFS(path.c_str(), x, y);  // 既存のconst char*バージョンを呼び出す
}
// LittleFSからPNGを読み込んで表示するヘルパー関数
bool drawPngFromLittleFS(const char* path, int16_t x, int16_t y) {
    auto file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("ファイルが開けません: %s\n", path);
        return false;
    }
    
    size_t fileSize = file.size();
    if (fileSize == 0) {
        Serial.println("ファイルサイズが0です");
        file.close();
        return false;
    }
    
    // ファイルバッファを確保
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[fileSize]);
    if (!buffer) {
        Serial.println("メモリ確保に失敗");
        file.close();
        return false;
    }
    
    // ファイルを読み込む
    size_t readBytes = file.read(buffer.get(), fileSize);
    file.close();
    
    if (readBytes != fileSize) {
        Serial.println("ファイル読み込みエラー");
        return false;
    }
    
    // PNGを描画
    canvas.drawPng(buffer.get(), fileSize, x, y);
    return true;
}

#endif // UI_HELPER_H