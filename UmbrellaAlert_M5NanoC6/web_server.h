/*
*******************************************************************************
* web_server.h - Webサーバー機能（画面なし版）
*  設定モード: 1画面のWiFi設定（キャプティブポータルが直接これを開く）
*  通常モード : 状態表示／都道府県選択／地図ページ導線／リセット
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
extern void reloadWeatherApi();  // 設定変更を再起動せず即時反映するため
extern String g_apSsid;     // 個体固有のAP SSID
extern String g_mdnsHost;   // 個体固有のmDNSホスト名（小文字、.local無し）

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

// 設定モードのWiFi設定ページ（1画面）。キャプティブポータルが直接これを表示する。
String wifiSetupHtml() {
    String s = "<h1>☂️ " + String(APP_TITLE) + "</h1>";
    s += "<div class='info'>この本体: <strong>" + g_apSsid + "</strong>（" + g_mdnsHost + ".local）<br>";
    s += "接続するWi-Fiを選んでパスワードを入力してください</div>";
    s += "<form method='get' action='setap'>";
    s += "<div style='display:flex;align-items:center;gap:10px;margin-bottom:8px;'>";
    s += "<label style='font-weight:bold;color:#374151;flex:1;'>ネットワークを選択:</label>";
    s += "<button type='button' onclick='refreshNetworks()' class='btn' style='width:auto;padding:8px 16px;margin:0;font-size:14px;'>🔄 更新</button>";
    s += "</div>";
    s += "<select id='networkSelect' name='ssid' style='margin-bottom:20px;'>" + ssidList + "</select>";
    s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>パスワード:</label>";
    s += "<input name='pass' type='password' placeholder='ネットワークパスワードを入力' maxlength='64'>";

    // 保存後の場所設定の案内（GitHub Pagesの「はじめての設定」に準拠したステップ）。
    // 再起動でこの画面は閉じるため、先に本体URLをコピーさせる。
    String localUrl = "http://" + g_mdnsHost + ".local";
    s += "<div class='howto'>";
    s += "<div class='howto-title'>📍 保存後の場所設定（先にURLをコピー）</div>";
    s += "<ol>";
    s += "<li>下の本体URLを<b>コピー</b>する</li>";
    s += "<li>「🛜 接続設定を保存」を押す（本体が再起動）</li>";
    s += "<li>スマホを<b>自宅のWi-Fi</b>に接続し直す</li>";
    s += "<li>コピーしたURLを開いて<b>場所を設定</b>する</li>";
    s += "</ol>";
    s += "<div class='urlrow'><span class='u' id='localurl'>" + localUrl + "</span>";
    s += "<button type='button' class='btn-sm' onclick='copyUrl()'>📋 コピー</button></div>";
    s += "<div id='copied' style='display:none;color:#166534;font-size:13px;margin:4px 0 0;'>コピーしました</div>";
    s += "</div>";

    s += "<button type='submit' class='btn'>🛜 接続設定を保存</button></form>";
    s += "<script>function refreshNetworks(){var btn=event.target;btn.innerHTML='更新中...';btn.disabled=true;";
    s += "fetch('/refresh-networks').then(r=>r.json()).then(d=>{document.getElementById('networkSelect').innerHTML=d.networks;btn.innerHTML='🔄 更新';btn.disabled=false;})";
    s += ".catch(e=>{btn.innerHTML='🔄 更新';btn.disabled=false;alert('更新に失敗しました');});}";
    // テキスト表示(span)からコピー。HTTP=非セキュアコンテキストでも動くよう execCommand フォールバック。
    s += "function copyUrl(){var t=document.getElementById('localurl').textContent;";
    s += "if(navigator.clipboard){navigator.clipboard.writeText(t).then(show).catch(function(){fb(t);});}else{fb(t);}";
    s += "function fb(x){var ta=document.createElement('textarea');ta.value=x;ta.style.position='fixed';ta.style.opacity='0';";
    s += "document.body.appendChild(ta);ta.select();try{document.execCommand('copy');show();}catch(e){}document.body.removeChild(ta);}";
    s += "function show(){document.getElementById('copied').style.display='block';}}</script>";
    return makePage("WiFi設定", s);
}

// Webサーバー開始（モードに応じたルート設定）
void startWebServer() {
    if (deviceMode == SETUP_MODE) {
        // ===== 設定モード（アクセスポイント）=====
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.softAPIP());
        dnsServer.start(DNS_SERVER_PORT, "*", WiFi.softAPIP());

        // キャプティブポータルは未知のURLを叩くので onNotFound で直接WiFi設定画面を出す（1画面）
        webServer.onNotFound([]() { webServer.send(200, "text/html", wifiSetupHtml()); });
        webServer.on("/settings", []() { webServer.send(200, "text/html", wifiSetupHtml()); });

        // ネットワーク再スキャン（Ajax）
        webServer.on("/refresh-networks", []() {
            updateNetworkList();
            String escaped = ssidList;
            escaped.replace("\\", "\\\\");
            escaped.replace("\"", "\\\"");
            webServer.send(200, "application/json",
                           "{\"networks\":\"" + escaped + "\",\"count\":" + String(networkCount) + "}");
        });

        // WiFi設定保存 → 再起動（場所は接続後に設定）
        webServer.on("/setap", []() {
            String ssid = urlDecode(webServer.arg("ssid"));
            String pass = urlDecode(webServer.arg("pass"));
            Serial.printf("SSID: %s\n", ssid.c_str());

            preferences.begin("wifi-config", false);
            preferences.putString("WIFI_SSID", ssid);
            preferences.putString("WIFI_PASSWD", pass);
            preferences.end();

            String localUrl = "http://" + g_mdnsHost + ".local";   // 個体の状態ページ（場所設定はここに集約）
            String s = "<h1>✅ WiFi設定完了</h1>";
            s += "<div class='success'>この本体（<strong>" + g_mdnsHost + ".local</strong>）が再起動して接続します。</div>";
            // 場所設定は本体URL（状態ページ）に集約。先にURLをコピーさせる。
            s += "<div class='howto'>";
            s += "<div class='howto-title'>📍 次に：場所の設定</div>";
            s += "<ol>";
            s += "<li>下の本体URLを<b>コピー</b>する</li>";
            s += "<li>スマホを<b>自宅のWi-Fi</b>に接続し直す</li>";
            s += "<li>コピーしたURLを開いて<b>場所を設定</b>する</li>";
            s += "</ol>";
            s += "<div class='urlrow'><span class='u' id='localurl'>" + localUrl + "</span>";
            s += "<button type='button' class='btn-sm' onclick='copyUrl()'>📋 コピー</button></div>";
            s += "<div id='copied' style='display:none;color:#166534;font-size:13px;margin:4px 0 0;'>コピーしました</div>";
            s += "</div>";
            s += "<a href='" + localUrl + "' class='btn' target='_blank' rel='noopener'>🔗 本体URLを開く</a>";
            // テキスト表示(span)からコピー。HTTP=非セキュアコンテキストでも動くよう execCommand フォールバック。
            s += "<script>function copyUrl(){var t=document.getElementById('localurl').textContent;";
            s += "if(navigator.clipboard){navigator.clipboard.writeText(t).then(show).catch(function(){fb(t);});}else{fb(t);}";
            s += "function fb(x){var ta=document.createElement('textarea');ta.value=x;ta.style.position='fixed';ta.style.opacity='0';";
            s += "document.body.appendChild(ta);ta.select();try{document.execCommand('copy');show();}catch(e){}document.body.removeChild(ta);}";
            s += "function show(){document.getElementById('copied').style.display='block';}}</script>";
            webServer.send(200, "text/html", makePage("設定完了", s));
            delay(2000);
            ESP.restart();
        });

    } else {
        // ===== 通常モード（WiFi接続済み）=====
        Serial.print("Webサーバー開始: ");
        Serial.println(WiFi.localIP());

        // 状態表示＋都道府県選択＋地図ページ導線
        webServer.on("/", []() {
            String s = "<h1>☂️ " + String(APP_TITLE) + "</h1>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>✅ 稼働中</label>";
            s += "<div class='info'>";
            s += "本体名: <strong>" + g_mdnsHost + ".local</strong><br>";
            s += "WiFi: <strong>" + WiFi.SSID() + "</strong><br>";
            s += "IP: <strong>" + WiFi.localIP().toString() + "</strong><br>";
            s += "現在の場所: <strong>" + getLocationName() + "</strong><br>";
            s += "稼働: <strong id='uptime'>-</strong>";
            // WiFiやり直し案内（稼働中の枠に合体）
            s += "<div style='margin-top:10px;padding-top:10px;border-top:1px solid rgba(102,126,234,0.2);font-size:13px;color:#6b7280;'>WiFi設定をやり直す場合は、本体ボタンを長押ししてください。</div>";
            s += "</div>";
            // 稼働秒数を起点に、JS側で毎秒リアルタイムにカウントアップ表示
            s += "<script>var up=" + String(millis() / 1000) + ";";
            s += "function fmt(t){var d=Math.floor(t/86400),h=Math.floor(t%86400/3600),m=Math.floor(t%3600/60),s=t%60;";
            s += "var r='';if(d)r+=d+'日';if(d||h)r+=h+'時間';if(d||h||m)r+=m+'分';r+=s+'秒';return r;}";
            s += "function tick(){document.getElementById('uptime').textContent=fmt(up);up++;}";
            s += "tick();setInterval(tick,1000);</script>";

            // 場所の設定（都道府県プリセット）
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>📍 場所の設定</label>";
            s += "<form method='get' action='setpref'>";
            s += "<label style='display:block;margin-bottom:8px;font-weight:bold;color:#374151;'>都道府県から選ぶ:</label>";
            s += "<select name='pref' onchange=\"document.getElementById('prefBtn').disabled=(this.value=='" + currentPrefValue() + "');\" style='margin-bottom:12px;'>" + prefOptionsHtml() + "</select>";
            s += "<button type='submit' id='prefBtn' class='btn' disabled>📍 この場所に設定</button></form>";

            // さらに細かく地図で（外部の地図ページへ）
            String setupUrl = String(SETUP_PAGE_URL) + "?ip=" + g_mdnsHost + ".local";
            s += "<a href='" + setupUrl + "' class='btn'>🗺 地図から指定する</a>";

            // 雨の通知（直近何時間先までの予報で判定するか）
            s += "<label style='display:block;margin:18px 0 8px;font-weight:bold;color:#374151;'>🌧️ 雨の通知</label>";
            s += "<form method='get' action='setnotify'>";
            s += "<div style='display:flex;align-items:center;gap:8px;margin-bottom:12px;'>";
            s += "<select name='hours' onchange=\"document.getElementById('nhBtn').disabled=(this.value=='" + String(getNotifyHours()) + "');\" style='width:auto;flex:none;margin:0;'>" + notifyHoursOptionsHtml() + "</select>";
            s += "<span style='font-size:14px;color:#374151;'>以内の雨予報を通知する</span>";
            s += "</div>";
            s += "<button type='submit' id='nhBtn' class='btn' disabled>💾 保存して反映</button></form>";

            webServer.send(200, "text/html", makePage("稼働中", s));
        });

        // 都道府県を選択 → 再起動せずに反映（その場で天気を再取得）
        webServer.on("/setpref", []() {
            if (webServer.hasArg("pref")) setPrefecture(webServer.arg("pref").toInt());
            String s = "<h1>✅ 場所を変更しました</h1>";
            s += "<div class='success'>「" + getLocationName() + "」に設定しました。新しい場所で天気を取得しました。</div>";
            s += "<a href='/' class='btn'>← 設定ページに戻る</a>";
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/html", makePage("場所変更", s));
            reloadWeatherApi();  // 再起動せずに即時反映
        });

        // 雨の通知（チェック時間）を変更 → 再起動せずに反映
        webServer.on("/setnotify", []() {
            if (webServer.hasArg("hours")) setNotifyHours(webServer.arg("hours").toInt());
            String s = "<h1>✅ 雨の通知を変更しました</h1>";
            s += "<div class='success'>直近 <strong>" + String(getNotifyHours()) + " 時間</strong>以内の雨予報で通知します。</div>";
            s += "<a href='/' class='btn'>← 設定ページに戻る</a>";
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/html", makePage("通知設定", s));
            reloadWeatherApi();  // 再起動せずに即時反映
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
            s += "新しい場所で天気を取得しました。このタブは自動で閉じます。</div>";
            s += "<script>setTimeout(function(){window.close();},1500);</script>";
            webServer.sendHeader("Access-Control-Allow-Origin", "*");
            webServer.sendHeader("Cache-Control", "no-store");
            webServer.send(200, "text/html", makePage("保存完了", s));
            reloadWeatherApi();  // 再起動せずに即時反映
        });
    }
    webServer.begin();
}

#endif // WEB_SERVER_H
