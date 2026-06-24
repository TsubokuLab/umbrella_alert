/*
*******************************************************************************
* web_server.h - Webサーバー機能
* 
* HTTP Webサーバーの設定、URLルーティング、
* WiFi設定ページ、状態確認ページなどのWeb機能を管理
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

// ==== 外部変数・関数の宣言 ====
extern String ssidList;
extern int networkCount;
extern DNSServer dnsServer;
extern WebServer webServer;
extern Preferences preferences;
extern DeviceMode deviceMode;

// 外部関数の宣言
extern void updateNetworkList();
extern void resetSettings();
extern void setCustomLocation(const String& lat, const String& lon, const String& name, long tz);
extern String g_apSsid;     // 個体固有のAP SSID
extern String g_mdnsHost;   // 個体固有のmDNSホスト名（小文字、.local無し）

// ==== ユーティリティ関数 ====

// URLデコード関数（Webフォームから送信されたデータをデコード）
String urlDecode(String input) {
    String s = input;
    s.replace("%20", " ");
    s.replace("+", " ");
    s.replace("%21", "!");
    s.replace("%22", "\"");
    s.replace("%23", "#");
    s.replace("%24", "$");
    s.replace("%25", "%");
    s.replace("%26", "&");
    s.replace("%27", "\'");
    s.replace("%28", "(");
    s.replace("%29", ")");
    s.replace("%2C", ",");
    s.replace("%2E", ".");
    s.replace("%2F", "/");
    s.replace("%3A", ":");
    s.replace("%3B", ";");
    s.replace("%3C", "<");
    s.replace("%3D", "=");
    s.replace("%3E", ">");
    s.replace("%3F", "?");
    s.replace("%40", "@");
    return s;
}

// ==== Webサーバー機能の実装 ====

// Webサーバー開始関数（モードに応じたルート設定）
void startWebServer() {
    if (deviceMode == SETUP_MODE) {
        // 設定モード: アクセスポイントモード
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.softAPIP());

        dnsServer.start(DNS_SERVER_PORT, "*", WiFi.softAPIP());
        
        // WiFi設定ページ
        webServer.on("/settings", []() {
            String s = "<h1>📶 WiFi設定</h1>";
            s += "<div class='info'>利用可能なネットワークを選択してパスワードを入力してください</div>";
            s += "<form method='get' action='setap'>";
            s += "<div style='display:flex;align-items:center;gap:10px;margin-bottom:8px;'>";
            s += "<label style='font-weight:bold;color:#374151;flex:1;'>ネットワークを選択:</label>";
            s += "<button type='button' onclick='refreshNetworks()' class='btn' style='width:auto;padding:8px 16px;margin:0;font-size:14px;'>🔄 更新</button>";
            s += "</div>";
            s += "<select id='networkSelect' name='ssid' style='margin-bottom:20px;'>" + ssidList + "</select>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>パスワード:</label>";
            s += "<input name='pass' type='password' placeholder='ネットワークパスワードを入力' maxlength='64'>";
            s += "<button type='submit' class='btn'>🔗 接続設定を保存</button></form>";
            
            // JavaScriptでAjax更新機能を追加
            s += "<script>";
            s += "function refreshNetworks() {";
            s += "  var btn = event.target;";
            s += "  btn.innerHTML = '更新中...';";
            s += "  btn.disabled = true;";
            s += "  fetch('/refresh-networks')";
            s += "    .then(response => response.json())";
            s += "    .then(data => {";
            s += "      var select = document.getElementById('networkSelect');";
            s += "      select.innerHTML = data.networks;";
            s += "      btn.innerHTML = '🔄 更新';";
            s += "      btn.disabled = false;";
            s += "      console.log('ネットワークリストを更新しました');";
            s += "    })";
            s += "    .catch(error => {";
            s += "      console.error('エラー:', error);";
            s += "      btn.innerHTML = '🔄 更新';";
            s += "      btn.disabled = false;";
            s += "      alert('ネットワークリストの更新に失敗しました');";
            s += "    });";
            s += "}";
            s += "</script>";
            
            webServer.send(200, "text/html", makePage("WiFi設定", s));
        });
        
        // ネットワークリスト更新処理（Ajax対応）
        webServer.on("/refresh-networks", []() {
            Serial.println("🔄 ネットワークリストの更新リクエストを受信");
            
            // ネットワークリストを更新
            updateNetworkList();
            
            // HTMLタグをJSON用にエスケープ
            String escapedSSIDList = ssidList;
            escapedSSIDList.replace("\\", "\\\\");  // バックスラッシュをエスケープ
            escapedSSIDList.replace("\"", "\\\"");    // ダブルクォートをエスケープ
            
            // JSONレスポンスを返す
            String jsonResponse = "{\"networks\":\"" + escapedSSIDList + "\",\"count\":" + String(networkCount) + "}";
            webServer.send(200, "application/json", jsonResponse);
            
            Serial.println("✅ ネットワークリスト更新完了");
        });
        
        // 設定保存処理
        webServer.on("/setap", []() {
            String ssid = urlDecode(webServer.arg("ssid"));
            String pass = urlDecode(webServer.arg("pass"));
            
            Serial.printf("SSID: %s\n", ssid.c_str());
            Serial.printf("パスワード設定完了\n");
            
            preferences.begin("wifi-config", false);
            preferences.putString("WIFI_SSID", ssid);
            preferences.putString("WIFI_PASSWD", pass);
            preferences.end();
            
            String s = "<h1>✅ 設定完了</h1>";
            s += "<div class='success'>WiFi設定が保存されました。<br>デバイスが自動的に再起動され接続を開始します。</div>";
            s += "<script>setTimeout(function(){document.body.innerHTML='<div style=\"text-align:center;padding:50px;background:white;border-radius:20px;\"><h2>🔄 再起動中...</h2><p>しばらくお待ちください</p></div><button type='button' id='close' class='btn'>❌ 閉じる</button>';}, 2000);</script>";
            s += "<script>const closeBtn = document.getElementById('close');closeBtn.addEventListener('click', function () {  window.close(); });</script>";
            webServer.send(200, "text/html", makePage("設定完了", s));
            delay(2000);
            ESP.restart();
        });
        
        // メインページ（設定モード）- キャプティブポータル対応
        webServer.onNotFound([]() {
            String s = "<h1>📱 " + String(APP_TITLE) + "</h1>";
            s += "<div class='info'>";
            s += "WiFi接続設定を開始します。<br>";
            s += "アクセスポイント名: <strong>" + g_apSsid + "</strong><br>";
            s += "設定用IP: <strong>" + WiFi.softAPIP().toString() + "</strong>";
            s += "</div>";
            s += "<a href='/settings' class='btn'>⚙️ WiFi設定を開始</a>";
            webServer.send(200, "text/html", makePage("セットアップ", s));
        });
    } else {
        // 通常モード: WiFi接続済み
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.localIP());
        
        // ステータスページ（通常モード）
        webServer.on("/", []() {
            String s = "<h1>☂️ " + String(APP_TITLE) + "</h1>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>✅ 稼働中</label>";
            s += "<div class='info'>";
            s += "本体名: <strong>" + g_mdnsHost + ".local</strong><br>";
            s += "WiFiネットワーク: <strong>" + WiFi.SSID() + "</strong><br>";
            s += "IPアドレス: <strong>" + WiFi.localIP().toString() + "</strong><br>";
            s += "信号強度: <strong>" + String(WiFi.RSSI()) + " dBm</strong><br>";
            s += "稼働時間: <strong>" + String(millis() / 1000) + " 秒</strong><br>";
            s += "</div>";

            // 雨の通知（直近何時間先までの予報で判定するか）
            s += "<label style='display:block;margin:18px 0 8px;font-weight:bold;color:#374151;'>🌧️ 雨の通知</label>";
            s += "<form method='get' action='setnotify'>";
            s += "<select name='hours' onchange=\"document.getElementById('nhBtn').disabled=(this.value=='" + String(getNotifyHours()) + "');\" style='margin-bottom:12px;'>" + notifyHoursOptionsHtml() + "</select>";
            s += "<button type='submit' id='nhBtn' class='btn' disabled>💾 保存して再起動</button></form>";

            s += "<a href='/reset' class='btn btn-danger'>🔄 設定をリセット</a>";
            webServer.send(200, "text/html", makePage("稼働中", s));
        });

        // 雨の通知（チェック時間）を変更 → 再起動で反映
        webServer.on("/setnotify", []() {
            if (webServer.hasArg("hours")) setNotifyHours(webServer.arg("hours").toInt());
            String s = "<h1>✅ 雨の通知を変更しました</h1>";
            s += "<div class='success'>直近 <strong>" + String(getNotifyHours()) + " 時間</strong>以内の雨予報で通知します。<br>本体が再起動して反映します。</div>";
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/html", makePage("通知設定", s));
            delay(500);
            ESP.restart();
        });
        
        // 設定リセット処理
        webServer.on("/reset", []() {
            String s = "<h1>🔄 リセット完了</h1>";
            s += "<div class='success'>WiFi設定をリセットしました。<br>デバイス再起動後、本体の指示に従って接続設定を行って下さい。</div>";
            webServer.send(200, "text/html", makePage("リセット", s));
            resetSettings();
        });

        // カスタム場所の保存（外部GitHub Pages設定ページからの送信を受ける）
        // GET /save?lat=..&lon=..&name=..&tz=..
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

            // 設定ページから「トップレベル遷移」で開かれるため、見やすい確認ページを返す
            String disp = name.length() > 0 ? name : (lat + ", " + lon);
            String s = "<h1>✅ 保存しました</h1>";
            s += "<div class='success'>場所を保存しました: <strong>" + disp + "</strong><br>";
            s += "本体が再起動して反映します（数十秒）。このタブは自動で閉じます。</div>";
            // 設定ページから別タブ(window.open)で開かれた場合、表示後に自動で閉じる
            s += "<script>setTimeout(function(){window.close();},1500);</script>";
            webServer.sendHeader("Access-Control-Allow-Origin", "*");
            webServer.sendHeader("Cache-Control", "no-store");  // ブラウザにキャッシュさせない
            webServer.send(200, "text/html", makePage("保存完了", s));
            delay(500);
            ESP.restart();  // 反映のため再起動
        });
    }
    webServer.begin();
}

#endif // WEB_SERVER_H