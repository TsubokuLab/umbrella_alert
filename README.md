# Umbrella Alert（お出かけ傘アラート）

12時間以内（3時間×4枠）の雨予報を OpenWeatherMap でチェックし、
「傘が必要かどうか」を M5Stack デバイスで知らせるアプリです。

- 雨予報あり → 青色LEDが回転アニメーション＋画面に傘アイコン
- 晴れ予報 → オレンジ色LEDがゆっくり明滅

## 主な機能
- **forecastベースの傘判定**: 直近12時間（`FORECAST_CHECK_HOURS`）の予報で雨/降水確率を判定
- **場所指定はハイブリッド**: プリセット都市（札幌/東京/大阪/福岡/那覇）＋ **カスタム緯度経度**（スマホの設定ページで地図から指定）
  - カスタム地点は都市選択リストの先頭（★）に常駐。プリセットを選んでも消えません
- **NeoPixel LED**: 雨/晴れアニメーション。LEDは専用タスク（別コア）で約100fpsを維持（描画負荷の影響を受けない）
- **ビープ音量ボタン**: 設定画面のQR上のボタンをタップで 小→中→大→OFF を循環・保存
- **WiFi初期設定**: APモードのキャプティブポータル（内蔵Webページ）から登録
- **設定はconfigに集約**: 更新間隔・しきい値・LED数/ピン・点滅間隔・タイムゾーン等を [config.h](UmbrellaAlert_Display/config.h) で変更可

## デバイス構成

| バージョン | デバイス | 表示 | フォルダ |
|-----------|---------|------|---------|
| LCD版 | M5Stack **Core2** / **CoreS3** | LCD画面 + NeoPixel LED | [`UmbrellaAlert_Display/`](UmbrellaAlert_Display/) |
| LEDのみ版 | M5Stack **NanoC6** | NeoPixel LEDのみ（傘立て埋め込み用） | [`UmbrellaAlert_M5NanoC6/`](UmbrellaAlert_M5NanoC6/) |

> 補助ツール: [`tools/UmbrellaAlert_ImageWriter/`](tools/UmbrellaAlert_ImageWriter/) — 画像書き込みのテスト用スケッチ。

### NanoC6版（画面なし）の初期設定
画面が無いので、WiFi・場所はすべて**本体のアクセスポイントにスマホで接続してブラウザから**設定します（LCD版と同じAP方式）。状態は**LEDで表示**：設定モード=シアン呼吸／接続中=黄コメット／通常=雨青回転・晴橙明滅。本体ボタン長押しでWiFiリセット。
- 接続情報は固定値なので**シール印刷**で案内可能：WiFi `Umbrella-Alert` / `12345678`、設定 `http://192.168.4.1`、再設定 `http://umbrella.local`
- 詳細は [docs/SETUP.md](docs/SETUP.md) §4-4-2 / [docs/SPEC.md](docs/SPEC.md) §12

## 場所設定ページ（スマホ・GitHub Pages）

緯度経度のカスタム指定は、スマホのブラウザから行います。フロントエンドは GitHub Pages で公開しています。

**URL:** <https://tsubokulab.github.io/umbrella_alert/?ip=本体のローカルIP>

（実装は [`web/`](web/)。`master` への push で [Actionsワークフロー](.github/workflows/deploy-pages.yml) が自動デプロイ）

### 使い方
1. 本体をWiFiに接続し、**設定画面（SETTING_MODE）に表示されるQRコード**をスマホで読む
   - QRには本体のローカルIPが `?ip=` で埋め込まれます（上のURL形式）
2. 開いたページで地図を動かし、**中央のレティクル（照準）を目的地に合わせる**（中心が常に選択地点）
   - 「現在地へ移動」ボタンでGPS位置へジャンプ可能（HTTPSページなのでGPSが使えます）
3. 地名は自動入力されます（任意・編集可）。緯度経度は地図直下に表示
4. 「本体に設定を保存」を押すと、本体の `/save` に座標が送られ、保存後に自動再起動して反映

### 仕組みと制約
- ページ（HTTPS）→ 本体（HTTP, `GET /save?lat=&lon=&name=&tz=`）へ送信し、本体は Preferences に保存して再起動します
- ⚠️ **Mixed Content 制限**: HTTPSページからHTTP機器への送信はブラウザにブロックされる場合があります。その場合はページに表示される**手動URL**を、本体と同じWi-Fiに繋いだ状態で開けば保存できます
- 詳細な設計・データモデル・エンドポイント仕様は [docs/SPEC.md](docs/SPEC.md) を参照

## ドキュメント
- **[docs/SETUP.md](docs/SETUP.md)** — 書き込み手順（環境構築・Partition設定・LittleFSアップロード・`secrets.h`・データ構造）
- **[docs/SPEC.md](docs/SPEC.md)** — 仕様書（現行実装＋場所カスタム化：ハイブリッド都市/緯度経度、外部GitHub Pages、API、Mixed Content対策）

## セットアップ（ざっくり）
詳細は **[docs/SETUP.md](docs/SETUP.md)**。

1. Arduino IDE 2.x + M5Stackボード + ライブラリ（M5Unified / ArduinoJson / Adafruit NeoPixel / TimeLib）
2. **`secrets.h` を作成**: `secrets.h.example` を `secrets.h` にコピーし、OpenWeatherMap の APIキー（NanoC6版はWiFi資格情報も）を設定（`secrets.h` は `.gitignore` 済みでコミットされません）
3. LCD版は Partition Scheme = **Huge APP (3MB No OTA/1MB SPIFFS)**
4. スケッチ書き込み → LittleFSアップロード（`data/` の天気アイコン）
5. 起動後APモードでWiFiを登録 → 都市選択 or スマホの設定ページでカスタム座標を指定

## ライセンス / クレジット
- 作者: [@kohack_v](https://x.com/kohack_v)
- 天気アイコン: OpenWeatherMap / 地図: OpenStreetMap・Leaflet
