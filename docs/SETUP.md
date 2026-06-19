# セットアップ手順 / 新デバイスへのインストールガイド

> このドキュメントは「数ヶ月後に記憶を失っても、これを読めば新しいM5Stackデバイスに
> 一から書き込める」ことを目的にまとめています。Core2 / CoreS3（LCDあり）と
> NanoC6（LCDなし・LEDのみ）の両方をカバーします。

- リポジトリ: https://github.com/TsubokuLab/umbrella_alert
- 対象デバイス: M5Stack Core2 / CoreS3 / NanoC6 など
- 開発環境: **Arduino IDE 2.3.x**（このドキュメントは 2.3.10 で確認）

---

## 0. 対象デバイスごとの違い（早見表）

| 項目 | Core2 | CoreS3 | NanoC6 |
|------|-------|--------|--------|
| SoC | ESP32 | ESP32-S3 | ESP32-C6 (RISC-V) |
| フラッシュ | 16MB | 16MB | 4MB |
| LCD画面 | あり（タッチ） | あり（タッチ） | **なし** |
| 出力 | 画面 + NeoPixel LED | 画面 + NeoPixel LED | **NeoPixel LEDのみ** |
| 画像(LittleFS) | **必要** | **必要** | 不要（画像を使わない） |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) | Huge APP (3MB No OTA/1MB SPIFFS) | デフォルトでOK |
| スケッチ | `UmbrellaAlert_Display/` | `UmbrellaAlert_Display/` | `UmbrellaAlert_M5NanoC6/` |

> NanoC6版は画面がないため天気アイコンPNGを使いません。よって **LittleFSのアップロードは不要**で、
> Partitionも気にしなくて大丈夫です。以降の「LittleFS」関連の章はCore2 / CoreS3のみ対象です。

---

## 1. Arduino IDE 2.x の環境構築

### 1-1. ボードマネージャにM5Stackを追加
1. `ファイル → 基本設定`（Preferences）を開く
2. 「追加のボードマネージャのURL」に以下を追加:
   ```
   https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
   ```
3. `ツール → ボード → ボードマネージャ` で **「M5Stack」** を検索してインストール

### 1-2. 使用ボードを選択
`ツール → ボード → M5Stack` から書き込むデバイスを選ぶ:
- Core2 → **M5Core2**
- CoreS3 → **M5CoreS3**
- NanoC6 → **M5NanoC6**

### 1-3. 必要なライブラリ
`スケッチ → ライブラリをインクルード → ライブラリを管理` で以下をインストール:

| ライブラリ | 用途 | 備考 |
|-----------|------|------|
| **M5Unified** | M5Stack全般の制御 | 必須 |
| **ArduinoJson** | 天気APIのJSON解析 | v6系 |
| **Adafruit NeoPixel** | LED制御 | 必須 |
| **Time**（by Paul Stoffregen / TimeLib） | 時刻処理 | LCD版のui_helperで使用 |

---

## 2. LittleFS環境の構築（Core2 / CoreS3 のみ）

天気アイコンPNGを本体フラッシュに書き込むためにLittleFSを使います。
**SPIFFSではなくLittleFS**を使用します（SPIFFS用アップローダーでは書き込めません）。

### 2-1. LittleFSアップロードプラグインのインストール
Arduino IDE 2.x は1.x系のプラグインと互換がないため、**専用プラグイン**を入れます。

- プラグイン: **arduino-littlefs-upload**（earlephilhower氏）
  - https://github.com/earlephilhower/arduino-littlefs-upload
- インストール先（Windows）:
  ```
  C:\Users\<ユーザー名>\.arduinoIDE\plugins\
  ```
  ※ `plugins` フォルダが無ければ作成し、リリースの `.vsix` を入れる
- Arduino IDEを**再起動**

> IDE 2.3.x では新しめの版（1.5.x前後）を使うこと。古い版だとコマンドが出てこない場合がある。

### 2-2. Partition Scheme の設定（重要）
`ツール → Partition Scheme` で次を選ぶ:

```
Huge APP (3MB No OTA/1MB SPIFFS)
```

理由:
- **LittleFSは「spiffs」ラベルのパーティション領域を流用する**ため、`spiffs`を含むスキームが必須。
  - ⚠️ 「16M Flash (3MB APP/9.9MB FATFS)」は **FAT(ffat)** 領域でLittleFSがマウントできないので**選ばない**こと。
- 本アプリは M5Unified + WiFi + WebServer + ArduinoJson + NeoPixel と大きく、APPが1.2MBを超えるため
  3MBのAPP領域がある「Huge APP」が安全（`text section exceeds available space` を回避）。
- 画像は約157KBなので1MBのSPIFFS(=LittleFS)領域で十分。OTAは未使用なので「No OTA」で問題なし。

---

## 3. データ構造（画像ファイル）

LCD版は天気アイコンを `data/` フォルダにPNGで持ち、LittleFSへ書き込みます。

```
UmbrellaAlert_Display/
├── UmbrellaAlert_Display.ino
├── config.h / settings.h / weather.h / ...
└── data/                ← この中身がLittleFSのルート(/)に書き込まれる
    ├── 01d.png          ← OpenWeatherMapのアイコンコード.png
    ├── 01n.png
    ├── 10d.png
    ├── 13n.png
    ├── ...
    └── resize/          ← 予報画面用の小サイズ版
        ├── 01d.png
        └── ...
```

- ファイル名は **OpenWeatherMapのアイコンコード**（`01d`=晴れ昼, `10d`=雨, `13d`=雪 など）に対応。
  - 参照: https://openweathermap.org/weather-conditions
- コード側は `/01d.png`・`/resize/10n.png` のように **先頭スラッシュ付きの絶対パス**で参照する
  （[ui_helper.h](../UmbrellaAlert_Display/ui_helper.h) の `drawPngFromLittleFS()`）。
- 画像を差し替える場合は、同じファイル名・PNG形式で `data/` を更新してから再アップロードする。

---

## 4. 書き込み手順

### 4-1. LCD版（Core2 / CoreS3）
1. `UmbrellaAlert_Display/UmbrellaAlert_Display.ino` を Arduino IDE で開く
2. `ツール` でボード・ポート・Partition Scheme（Huge APP）を設定
3. **スケッチを書き込む**（通常の `→` ボタン）
4. **シリアルモニタを閉じる**（開いたままだとポートを掴んでLittleFS書き込みが失敗する）
5. `Ctrl + Shift + P` → 「**Upload LittleFS to Pico/ESP8266/ESP32**」を実行
   - 現在開いているスケッチの `data/` がLittleFSへ書き込まれる
6. 完了後、シリアルモニタを開いて起動ログを確認:
   ```
   LittleFS マウント成功
   ファイル: /01d.png
   ファイル: /13n.png
   ...
   ```
   ファイル名が並べば成功。「マウント失敗」やファイルが出ない場合は §5 を参照。

### 4-2. LEDのみ版（NanoC6）
1. `UmbrellaAlert_M5NanoC6/UmbrellaAlert_M5NanoC6.ino` を開く
2. ボード=M5NanoC6 を選び、**スケッチを書き込むだけ**（LittleFSアップロードは不要）

### 4-3. WiFi / APIキーの初回設定
- 起動後、デバイスがAPモード（SSID: `Umbrella-Alert` / PASS: `12345678`）になる
- スマホ等で接続し、表示されるQR/URL（`http://192.168.4.1`）から設定画面を開いてWiFiを登録
- OpenWeatherMapのAPIキー・都市は [config.h](../UmbrellaAlert_Display/config.h) で定義（`API_KEY` / `CITY_*`）

---

## 5. トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| アイコンが出ない / `ファイルが開けません` がシリアルに出る | LittleFSへ画像未アップロード | §4-1 のステップ5を実行 |
| **スケッチを焼き直したら画像が消えた** | **Partition Schemeが既定に戻り、spiffs領域がずれて`LittleFS.begin(true)`で自動フォーマットされた** | **§5-1 参照。スキームをHuge APPに戻し、LittleFSを再アップロード** |
| `LittleFS マウント失敗` | Partitionが`spiffs`系でない | Partition Schemeを「Huge APP」に変更して再書き込み |
| LittleFSアップロードが失敗する | シリアルモニタが開いている | モニタを閉じてから再実行 |
| コマンドパレットに「Upload LittleFS」が出ない | プラグイン未インストール / 古い版 | §2-1 のプラグインを入れIDE再起動 |
| `text section exceeds available space` | APP領域が小さいPartition | 「Huge APP (3MB No OTA/1MB SPIFFS)」を選択 |

### 5-1. 「スケッチ書き込みで画像が消える」問題（重要）
通常、**スケッチ（APP領域）の書き込みではLittleFS（画像）は消えません**。毎回画像を再アップロードする必要はありません。
それでも消える場合、原因はほぼ次のどちらかです。

- **Partition Schemeが既定に戻っている（最頻出）**
  Arduino IDE 2.x の Partition Scheme 選択は **「ボードごと」にIDE設定として記憶**されます（スケッチには保存されない）。
  このリポジトリは Core2 / CoreS3 / NanoC6 を扱うため、**別ボードを選んでCoreS3に戻すと既定スキームにリセット**されがち。
  既定スキームに変わると spiffs 領域の位置・サイズがずれ、`LittleFS.begin(true)`（マウント失敗時フォーマット）で**空に初期化**される。
  → **書き込み前に毎回 `ツール → Partition Scheme = Huge APP (3MB No OTA/1MB SPIFFS)` を確認する**。
- **「Erase All Flash Before Sketch Upload」が Enabled**
  これが有効だと書き込みのたびに全フラッシュ消去（LittleFS含む）される。→ **Disabled** にする。

復旧: ①スキームをHuge APPに戻す → ②`Erase All Flash...`をDisabledに → ③もう一度 Upload LittleFS（§4-1 ステップ5）。
以降はスキームを固定したままなら、`.ino`の書き込みだけで画像は保持される。

---

## 6. メモ（ハードウェア）
- NeoPixel LEDのピンはコード内で `M5.Ex_I2C.getSDA()` から動的取得しているため、
  Core2/CoreS3でGroveポートに接続したLEDなら基本そのまま動く（[config.h](../UmbrellaAlert_Display/config.h) の `LED_PIN` はCore2の参考値）。
- LED数は `LED_COUNT`（既定24）。リング/本数に合わせて変更する。
