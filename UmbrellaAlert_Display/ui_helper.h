// ui_helper.h - UI関連のヘルパー関数

#ifndef UI_HELPER_H
#define UI_HELPER_H

#include <LittleFS.h>      // M5Unifiedより前にインクルードする必要がある
#include <M5Unified.h>
#include "weather.h"
#include "config.h"
#include <TimeLib.h>
#include <time.h>      // NTP時刻（日時表示）用

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
    String labels[3] = { btnA, btnB, btnC };
    // ボタン背景（ラベルが空の位置は無効ボタンとして暗いグレーで描画）
    for (int i = 0; i < 3; i++) {
        uint16_t col = labels[i].length() == 0 ? BTN_DISABLED_COLOR : BTN_BG_COLOR;
        canvas.fillRoundRect(btm_width * i, height - 40, btm_width - 1, 40, 10, col);
    }
    // ボタンラベル
    canvas.setTextColor(BTN_TEXT_COLOR);
    canvas.setFont(&fonts::lgfxJapanGothicP_20);
    canvas.setTextSize(1.0 * FONT_SCALE);
    canvas.setTextDatum(MC_DATUM);
    for (int i = 0; i < 3; i++) {
        if (labels[i].length() == 0) continue;
        canvas.drawString(labels[i], width / 6 * (i * 2 + 1), height - 20);
    }
    canvas.setFont(&fonts::lgfxJapanGothicP_16);
}

void showMainScreen(bool willRain, float rainProbability, float temperature, float temp_min, float temp_max, String _iconName, String cityName){
    currentScreen = MAIN_PAGE;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240
    
    String _iconPath = _iconName;

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

    // 日時表示（左下・アイコンに重ねる）。NTP同期済みのみ。
    struct tm tnow;
    if (getLocalTime(&tnow, 5)) {
        static const char* const WDAYS[] = {"日", "月", "火", "水", "木", "金", "土"};
        char dateBuf[24];
        snprintf(dateBuf, sizeof(dateBuf), "%d/%d (%s)", tnow.tm_mon + 1, tnow.tm_mday, WDAYS[tnow.tm_wday]);
        char hh[3], mm[3];
        snprintf(hh, sizeof(hh), "%02d", tnow.tm_hour);
        snprintf(mm, sizeof(mm), "%02d", tnow.tm_min);

        uint16_t fg = willRain ? TFT_WHITE : NAVY_COLOR;
        uint16_t sh = willRain ? CLOCK_SHADOW_RAINY : CLOCK_SHADOW_SUNNY;  // 影でアイコン上でも視認性を確保
        const int shx = 2, shy = 2;  // 影オフセット
        canvas.setTextDatum(BL_DATUM);

        // 日付（M/D (曜)）
        canvas.setFont(&fonts::lgfxJapanGothicP_20);
        canvas.setTextColor(sh); canvas.drawString(dateBuf, 10 + 1, height - 86 + 1);
        canvas.setTextColor(fg); canvas.drawString(dateBuf, 10, height - 86);

        // 時刻（HH:MM・大きめ）。コロンは0.5秒ごとに点滅。
        // 時/コロン/分を分割して描画し、コロン非表示でも分がずれないようにする。
        canvas.setFont(&fonts::lgfxJapanGothicP_40);
        const int tx = 10, ty = height - 44;
        const int wHH = canvas.textWidth(hh);
        const int wColon = canvas.textWidth(":");
        const int mx = tx + wHH + wColon;
        bool colonOn = (millis() / 500) % 2 == 0;
        // 時
        canvas.setTextColor(sh); canvas.drawString(hh, tx + shx, ty + shy);
        canvas.setTextColor(fg); canvas.drawString(hh, tx, ty);
        // コロン（点滅）
        if (colonOn) {
            canvas.setTextColor(sh); canvas.drawString(":", tx + wHH + shx, ty + shy);
            canvas.setTextColor(fg); canvas.drawString(":", tx + wHH, ty);
        }
        // 分
        canvas.setTextColor(sh); canvas.drawString(mm, mx + shx, ty + shy);
        canvas.setTextColor(fg); canvas.drawString(mm, mx, ty);
    }

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
    
    // 注視枠アニメーション（WARNING_BLINK_INTERVAL毎に点灯/消灯を切替）
    if(willRain && (millis() / WARNING_BLINK_INTERVAL) % 2 == 0){
        int _bold = 8; // ライン太さ
        canvas.fillRect(0, 0, _bold, height - 40, WARNING_COLOR);
        canvas.fillRect(width - _bold, 0, _bold, height - 40, WARNING_COLOR);
        canvas.fillRect(0, 0, width, _bold, WARNING_COLOR);
        canvas.fillRect(0, height - 40 - _bold, width, _bold, WARNING_COLOR);
    }

    // 操作ガイド（詳細ボタンは廃止）
    drawVirtualButtons("設定", "", "都市");
    canvas.pushSprite(&M5.Display, 0, 0);
}

// 詳細画面表示
void showDetailScreen(float temperature, float humidity, float windSpeed, String weatherDesc) {
    currentScreen = DETAIL_PAGE;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

    canvas.fillScreen(RAINY_COLOR);
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

    // 操作ガイド（左下の戻るのみ）
    drawVirtualButtons("戻る", "", "");

    canvas.pushSprite(&M5.Display, 0, 0);
}

void showForecastScreen(DynamicJsonDocument& doc, bool willRain, long tzOffsetSec, String cityNameOverride){
    currentScreen = FORECAST_PAGE;
    int width = M5.Display.width(); //320
    int height = M5.Display.height(); //240

    // カスタム地点では利用者の入力名を優先（空ならAPIの都市名）
    String cityName = cityNameOverride.length() > 0 ? cityNameOverride : doc["city"]["name"].as<String>();

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
    for (int i = 0; i < FORECAST_DISPLAY_COUNT; i++) {
        String main = doc["list"][i]["weather"][0]["main"].as<String>();
        //String description = doc["list"][0]["weather"][0]["description"].as<String>();
        String _iconPath = doc["list"][i]["weather"][0]["icon"].as<String>();
        float pop = doc["list"][i]["pop"].as<float>() * 100;  // 降水確率（%）
        float temp = doc["list"][i]["main"]["temp_min"].as<float>();  // 気温

        // 時間
        time_t unixTime = doc["list"][i]["dt"].as<int>();

        // 構造体に展開
        tmElements_t tm;
        breakTime(unixTime + tzOffsetSec, tm); // 選択中の都市のタイムゾーンで補正
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
    
    // 操作ガイド（メインと同配置。詳細ボタンは廃止）
    drawVirtualButtons("設定", "", "都市");

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
// メイン画面は毎フレーム同じアイコンを描画するため、直近1件のPNGを
// RAMにキャッシュしてフラッシュの再読込・メモリ再確保を回避する。
// （描画に渡すバイト列は同一なので表示結果は変わらない）
bool drawPngFromLittleFS(const char* path, int16_t x, int16_t y) {
    static String cachedPath;
    static std::unique_ptr<uint8_t[]> cachedBuf;
    static size_t cachedSize = 0;

    // キャッシュヒット: 同じファイルなら再読込せずに描画
    if (cachedBuf && cachedPath == path) {
        canvas.drawPng(cachedBuf.get(), cachedSize, x, y);
        return true;
    }

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

    // 次回のためにキャッシュへ保存
    cachedPath = path;
    cachedSize = fileSize;
    cachedBuf  = std::move(buffer);
    return true;
}

#endif // UI_HELPER_H