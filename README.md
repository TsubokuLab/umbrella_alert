# Umbrella Alert（お出かけ傘アラート）

12時間以内（3時間×4枠）の雨予報をOpenWeatherMapでチェックし、
「傘が必要かどうか」をM5Stackデバイスで知らせるアプリです。

- 雨予報あり → 青色LEDが回転アニメーション＋画面に傘アイコン
- 晴れ予報 → オレンジ色LEDがゆっくり明滅

## デバイス構成

| バージョン | デバイス | 表示 | フォルダ |
|-----------|---------|------|---------|
| LCD版 | M5Stack **Core2** / **CoreS3** | LCD画面 + NeoPixel LED | [`UmbrellaAlert_Display/`](UmbrellaAlert_Display/) |
| LEDのみ版 | M5Stack **NanoC6** | NeoPixel LEDのみ（傘立て埋め込み用） | [`UmbrellaAlert_M5NanoC6/`](UmbrellaAlert_M5NanoC6/) |

> 補助ツール: [`tools/UmbrellaAlert_ImageWriter/`](tools/UmbrellaAlert_ImageWriter/) — 画像書き込みのテスト用スケッチ。

## セットアップ

新しいデバイスへの書き込み手順（環境構築・Partition設定・LittleFSアップロード・データ構造）は
**[docs/SETUP.md](docs/SETUP.md)** にまとめています。

ざっくり:
1. Arduino IDE 2.x + M5Stackボード + ライブラリ（M5Unified / ArduinoJson / Adafruit NeoPixel / TimeLib）
2. LCD版は Partition Scheme = **Huge APP (3MB No OTA/1MB SPIFFS)**
3. スケッチ書き込み → LittleFSアップロード（`data/` の天気アイコン）
4. 起動後APモードでWiFiとAPIキーを設定

## ライセンス / クレジット
- 作者: [@kohack_v](https://x.com/kohack_v)
- 天気アイコン: OpenWeatherMap
