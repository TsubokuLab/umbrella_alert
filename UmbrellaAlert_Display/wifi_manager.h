/*
*******************************************************************************
* wifi_manager.h - WiFi接続管理機能
* 
* WiFi接続、設定保存・復元、ネットワークスキャン、
* アクセスポイントモードなどのWiFi関連機能を管理
*******************************************************************************
*/

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <M5Unified.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_mac.h>
#include "config.h"

// ==== 外部変数・関数の宣言 ====
extern String ssidList;
extern String wifi_ssid;
extern String wifi_password;
extern int networkCount;
extern unsigned long lastWiFiCheck;
extern DNSServer dnsServer;
extern WebServer webServer;
extern Preferences preferences;

// 外部関数の宣言
extern void startWebServer();
extern void rebootDevice();

// ==== デバイス個体識別（複数台運用のためMAC由来で一意化）====
// AP SSID = "Umbrella-Alert-A1B2" / mDNS = "umbrella-a1b2.local"
String g_apSsid;
String g_mdnsHost;

String deviceSuffix() {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[5];
    snprintf(buf, sizeof(buf), "%02X%02X", mac[4], mac[5]);
    return String(buf);
}

void initDeviceIdentity() {
    String suf = deviceSuffix();
    g_apSsid   = String(AP_SSID) + "-" + suf;
    g_mdnsHost = String(DNS_DOMAIN) + "-" + suf;
    g_mdnsHost.toLowerCase();
    Serial.println("デバイス識別: SSID=" + g_apSsid + " / http://" + g_mdnsHost + ".local");
}

// ==== WiFi管理機能の実装 ====

// 保存された設定の復元
boolean restoreConfig() {
    preferences.begin("wifi-config", false);
    wifi_ssid = preferences.getString("WIFI_SSID", "");
    wifi_password = preferences.getString("WIFI_PASSWD", "");
    preferences.end();
    if (wifi_ssid.length() > 0) {
        WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
        return true;
    }
    return false;
}

// WiFi接続確認
boolean checkConnection() {
    int count = 0;
    Serial.print("WiFi接続確認中");

    // WiFi接続確認時間を記録
    lastWiFiCheck = millis();

    while (count < WIFI_CONNECTION_TIMEOUT) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n接続成功!");
            return true;
        }
        delay(500);
        Serial.print(".");
        count++;
    }
    
    Serial.println("接続タイムアウト");
    return false;
}

// ネットワークリスト更新（WiFiスキャン）
void updateNetworkList() {
    // WiFiネットワークスキャン
    networkCount = WiFi.scanNetworks();
    delay(100);
    Serial.printf("ネットワークリスト更新: %d個のネットワークを検出\n", networkCount);
    
    // ネットワークリスト作成（HTML option形式）
    ssidList = "";
    for (int i = 0; i < networkCount; ++i) {
        ssidList += "<option value='" + WiFi.SSID(i) + "'>";
        ssidList += WiFi.SSID(i);
        ssidList += " (" + String(WiFi.RSSI(i)) + "dBm)";
        ssidList += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " 🔓" : " 🔒";
        ssidList += "</option>";
    }
}

// セットアップモード（アクセスポイントモード）の開始
void setupMode() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.println("ネットワークスキャンを開始");
    
    // ネットワークリスト更新（スキャンも含む）
    updateNetworkList();
    
    // アクセスポイント開始
    delay(100);
    WiFi.softAPConfig(AP_IP_ADDR, AP_IP_ADDR, IPAddress(255, 255, 255, 0));
    WiFi.softAP(g_apSsid.c_str(), AP_PASS);
    WiFi.mode(WIFI_AP);

    // Webサーバー開始
    startWebServer();

    // DNSサーバー開始（mDNS対応・個体固有ホスト名）
    if (MDNS.begin(g_mdnsHost.c_str())) {
        Serial.println("MDNS responder started : http://" + g_mdnsHost + ".local");
    }

    Serial.printf("\nアクセスポイント開始: \"%s\"\n", g_apSsid.c_str());
    Serial.printf("設定URL: http://%s\n", AP_IP_ADDR.toString().c_str());
}

// 設定をクリア（WiFi設定削除）
void resetSettings(){
    Serial.println("設定をクリア");
    // 音量/場所の保存で名前空間が切り替わっている可能性があるため明示的に開く
    preferences.begin("wifi-config", false);
    preferences.remove("WIFI_SSID");
    preferences.remove("WIFI_PASSWD");
    preferences.end();
    rebootDevice();
}

#endif // WIFI_MANAGER_H