/*
*******************************************************************************
* web_server.h - Webサーバー機能（画面なし版）
*  設定モード: WiFi＋都市の設定ページ
*  通常モード : 状態表示／都市変更／カスタム場所保存(/save)／リセット
*******************************************************************************
*/

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <M5Unified.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "config.h"
#include "styles.h"
#include "settings.h"

// ==== 外部変数・関数の宣言 ====
extern String ssidList;
extern int networkCount;
extern DNSServer dnsServer;
extern WebServer webServer;
extern Preferences preferences;
extern DeviceMode deviceMode;

extern void updateNetworkList();
extern void resetSettings();

// URLデコード
String urlDecode(String input) {
    String s = input;
    s.replace("%20", " "); s.replace("+", " ");
    s.replace("%21", "!"); s.replace("%22", "\""); s.replace("%23", "#");
    s.replace("%24", "$"); s.replace("%25", "%"); s.replace("%26", "&");
    s.replace("%27", "\'"); s.replace("%28", "("); s.replace("%29", ")");
    s.replace("%2C", ","); s.replace("%2E", "."); s.replace("%2F", "/");
    s.replace("%3A", ":"); s.replace("%3B", ";"); s.replace("%3C", "<");
    s.replace("%3D", "="); s.replace("%3E", ">"); s.replace("%3F", "?");
    s.replace("%40", "@");
    return s;
}

// Webサーバー開始（モードに応じたルート設定）
void startWebServer() {
    if (deviceMode == SETUP_MODE) {
        // ===== 設定モード（アクセスポイント）=====
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.softAPIP());
        dnsServer.start(DNS_SERVER_PORT, "*", WiFi.softAPIP());

        // WiFi＋都市の設定ページ
        webServer.on("/settings", []() {
            String s = "<h1>📶 WiFi設定</h1>";
            s += "<div class='info'>接続するWiFiとパスワード、表示したい地域を選んでください</div>";
            s += "<form method='get' action='setap'>";
            s += "<div style='display:flex;align-items:center;gap:10px;margin-bottom:8px;'>";
            s += "<label style='font-weight:bold;color:#374151;flex:1;'>ネットワークを選択:</label>";
            s += "<button type='button' onclick='refreshNetworks()' class='btn' style='width:auto;padding:8px 16px;margin:0;font-size:14px;'>🔄 更新</button>";
            s += "</div>";
            s += "<select id='networkSelect' name='ssid' style='margin-bottom:20px;'>" + ssidList + "</select>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>パスワード:</label>";
            s += "<input name='pass' type='password' placeholder='ネットワークパスワードを入力' maxlength='64'>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>地域:</label>";
            s += "<select name='city' style='margin-bottom:20px;'>" + cityOptionsHtml() + "</select>";
            s += "<button type='submit' class='btn'>🔗 接続設定を保存</button></form>";

            s += "<script>function refreshNetworks(){var btn=event.target;btn.innerHTML='更新中...';btn.disabled=true;";
            s += "fetch('/refresh-networks').then(r=>r.json()).then(d=>{document.getElementById('networkSelect').innerHTML=d.networks;btn.innerHTML='🔄 更新';btn.disabled=false;})";
            s += ".catch(e=>{btn.innerHTML='🔄 更新';btn.disabled=false;alert('更新に失敗しました');});}</script>";

            webServer.send(200, "text/html", makePage("WiFi設定", s));
        });

        // ネットワーク再スキャン（Ajax）
        webServer.on("/refresh-networks", []() {
            updateNetworkList();
            String escaped = ssidList;
            escaped.replace("\\", "\\\\");
            escaped.replace("\"", "\\\"");
            webServer.send(200, "application/json",
                           "{\"networks\":\"" + escaped + "\",\"count\":" + String(networkCount) + "}");
        });

        // 設定保存（WiFi＋都市）→ 再起動
        webServer.on("/setap", []() {
            String ssid = urlDecode(webServer.arg("ssid"));
            String pass = urlDecode(webServer.arg("pass"));
            Serial.printf("SSID: %s / 地域index: %s\n", ssid.c_str(), webServer.arg("city").c_str());

            preferences.begin("wifi-config", false);
            preferences.putString("WIFI_SSID", ssid);
            preferences.putString("WIFI_PASSWD", pass);
            preferences.end();

            if (webServer.hasArg("city")) setCityByIndex(webServer.arg("city").toInt());

            String s = "<h1>✅ 設定完了</h1>";
            s += "<div class='success'>設定を保存しました。<br>本体が再起動して接続を開始します。</div>";
            webServer.send(200, "text/html", makePage("設定完了", s));
            delay(2000);
            ESP.restart();
        });

        // キャプティブポータル
        webServer.onNotFound([]() {
            String s = "<h1>📱 " + String(APP_TITLE) + "</h1>";
            s += "<div class='info'>WiFi接続設定を開始します。<br>";
            s += "アクセスポイント名: <strong>" + String(AP_SSID) + "</strong><br>";
            s += "設定用IP: <strong>" + WiFi.softAPIP().toString() + "</strong></div>";
            s += "<a href='/settings' class='btn'>⚙️ 設定を開始</a>";
            webServer.send(200, "text/html", makePage("セットアップ", s));
        });

    } else {
        // ===== 通常モード（WiFi接続済み）=====
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.localIP());

        // 状態表示＋地域変更＋場所設定ページへの導線
        webServer.on("/", []() {
            String s = "<h1>✅ 接続中</h1>";
            s += "<div class='info'>";
            s += "WiFi: <strong>" + WiFi.SSID() + "</strong><br>";
            s += "IP: <strong>" + WiFi.localIP().toString() + "</strong><br>";
            s += "現在の場所: <strong>" + getLocationName() + "</strong><br>";
            s += "稼働: <strong>" + String(millis() / 1000) + " 秒</strong>";
            s += "</div>";

            // 地域（プリセット）変更
            s += "<form method='get' action='setcity'>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>地域を選ぶ:</label>";
            s += "<select name='city' style='margin-bottom:12px;'>" + cityOptionsHtml() + "</select>";
            s += "<button type='submit' class='btn'>📍 この地域に設定</button></form>";

            // カスタム緯度経度（外部の地図ページへ）
            String setupUrl = String(SETUP_PAGE_URL) + "?ip=" + WiFi.localIP().toString();
            s += "<a href='" + setupUrl + "' class='btn'>🗺 地図でカスタム地点を設定</a>";

            s += "<a href='/reset' class='btn btn-danger'>🔄 WiFi設定をリセット</a>";
            webServer.send(200, "text/html", makePage("稼働中", s));
        });

        // 地域（プリセット都市）変更 → 再起動で反映
        webServer.on("/setcity", []() {
            if (webServer.hasArg("city")) setCityByIndex(webServer.arg("city").toInt());
            String s = "<h1>✅ 地域を変更しました</h1>";
            s += "<div class='success'>本体が再起動して反映します。</div>";
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/html", makePage("地域変更", s));
            delay(500);
            ESP.restart();
        });

        // WiFi設定リセット
        webServer.on("/reset", []() {
            String s = "<h1>🔄 リセット完了</h1>";
            s += "<div class='success'>WiFi設定をリセットしました。<br>再起動後、設定モードになります。</div>";
            webServer.send(200, "text/html", makePage("リセット", s));
            resetSettings();
        });

        // カスタム場所保存（GitHub Pagesの地図ページから /save?lat=&lon=&name=&tz=）
        webServer.on("/save", []() {
            if (!webServer.hasArg("lat") || !webServer.hasArg("lon")) {
                webServer.sendHeader("Access-Control-Allow-Origin", "*");
                webServer.send(400, "text/plain", "missing lat/lon");
                return;
            }
            String lat  = webServer.arg("lat");
            String lon  = webServer.arg("lon");
            String name = urlDecode(webServer.arg("name"));
            long   tz   = webServer.hasArg("tz") ? (long)webServer.arg("tz").toInt() : (long)DEFAULT_TIMEZONE_OFFSET;
            Serial.printf("場所設定を受信: lat=%s lon=%s name=%s tz=%ld\n",
                          lat.c_str(), lon.c_str(), name.c_str(), tz);
            setCustomLocation(lat, lon, name, tz);

            String disp = name.length() > 0 ? name : (lat + ", " + lon);
            String s = "<h1>✅ 保存しました</h1>";
            s += "<div class='success'>場所を保存しました: <strong>" + disp + "</strong><br>";
            s += "本体が再起動して反映します。このタブは自動で閉じます。</div>";
            s += "<script>setTimeout(function(){window.close();},1500);</script>";
            webServer.sendHeader("Access-Control-Allow-Origin", "*");
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/html", makePage("保存完了", s));
            delay(500);
            ESP.restart();
        });
    }
    webServer.begin();
}

#endif // WEB_SERVER_H
