# Umbrella Alert（おでかけ傘アラート）

12時間以内（3時間×4枠）の雨予報を OpenWeatherMap でチェックし、
「傘が必要かどうか」を M5Stack デバイスで知らせるアプリです。

- 雨予報あり → 青色LEDが回転アニメーション＋画面に傘アイコン
- 晴れ予報 → オレンジ色LEDがゆっくり明滅

## 主な機能
- **forecastベースの傘判定（dtベース）**: 各予報枠の時刻(dt)が「現在＋通知時間」以内のものだけで雨判定。通知する時間幅（**直近1〜24時間**）は本体ページの「🌧️雨の通知」プルダウンで変更可（既定12h、`FORECAST_CHECK_HOURS`）
- **場所指定はハイブリッド**: **47都道府県プルダウン**＋ **カスタム緯度経度**（GitHub Pages地図から指定）。本体ページ（`http://umbrella-alert-xxxx.local`）で選択でき、Display/NanoC6共通
  - 地図で指定したカスタム場所は別枠で保存され、都道府県に切替後も「カスタム（地名）」でいつでも復元できます
- **設定は再起動なしで即時反映**: 場所・通知時間の保存は **Ajax＋トースト通知**でページ遷移も再起動もなし。その場で天気を再取得
- **WiFi初期設定**: APモードの**1画面キャプティブポータル**（接続すると即WiFi設定フォーム）。本体URLのコピー導線つき
- **NeoPixel LED**: 雨/晴れアニメーション。LEDは専用タスク（別コア）で約100fpsを維持（描画負荷の影響を受けない）
- **ビープ音量ボタン**（Display）: 設定画面のボタンで 小→中→大→OFF を循環・保存
- **設定はconfigに集約**: 更新間隔・しきい値・LED数/ピン・点滅間隔・タイムゾーン等を [config.h](UmbrellaAlert_Display/config.h) で変更可

## デバイス構成

| バージョン | デバイス | 表示 | フォルダ |
|-----------|---------|------|---------|
| LCD版 | M5Stack **Core2** / **CoreS3** | LCD画面 + NeoPixel LED | [`UmbrellaAlert_Display/`](UmbrellaAlert_Display/) |
| LEDのみ版 | M5Stack **NanoC6** | NeoPixel LEDのみ（傘立て埋め込み用） | [`UmbrellaAlert_M5NanoC6/`](UmbrellaAlert_M5NanoC6/) |

> 補助ツール: [`tools/UmbrellaAlert_ImageWriter/`](tools/UmbrellaAlert_ImageWriter/) — 画像書き込みのテスト用スケッチ。

### 初期設定の流れ（両版共通）
1. 本体APに接続 → **1画面のキャプティブポータル**でWiFiを登録 → 本体が再起動して自宅WiFiへ接続
2. スマホを自宅WiFiに戻し、本体ページ **`http://umbrella-alert-xxxx.local`** を開く
3. **47都道府県プルダウン**で場所を選ぶ（or「🗺 地図から指定する」で任意座標）。保存は即時反映（再起動なし）

### NanoC6版（画面なし）の状態表示
画面が無い代わりに状態を**LEDで表示**：設定モード=シアン呼吸／接続中=黄コメット／通常=雨青回転・晴橙明滅。本体ボタン長押しでWiFiリセット。
- 詳細は [docs/SETUP.md](docs/SETUP.md) §4-4-2 / [docs/SPEC.md](docs/SPEC.md) §12

### 複数台運用（個体識別）
家庭内で複数台使う場合に名前が衝突しないよう、**各台はMAC由来の固有名**を自動で持ちます（設定不要）。
- AP SSID = `Umbrella-Alert-XXXX` / 本体名 = `umbrella-alert-xxxx.local`（`XXXX`はMAC末尾4桁HEX）
- 各台が固有なので、WiFi一覧で区別でき、同一LANでも `umbrella-alert-xxxx.local` で個別アクセス可能
- 本体は自分の名前を**シリアル／設定・状態ページ／画面（Display）**に表示するので、それを見て**台ごとの接続QRシール**（固有SSIDのWiFi自動接続QR）を作れる
- OpenWeatherMap APIキーは共通でOK（家庭の数台なら無料枠に対し誤差）

## 地図で細かく指定（スマホ・GitHub Pages）

都道府県より細かい緯度経度指定は、地図ページ（GitHub Pages・HTTPS）から行います。実装は [`web/`](web/)。`master` への push で [Actionsワークフロー](.github/workflows/deploy-pages.yml) が自動デプロイ。

**URL:** <https://tsubokulab.github.io/umbrella_alert/?ip=umbrella-alert-xxxx.local>

### 使い方
1. 本体ページの「🗺 地図から指定する」リンク（または本体LCDの「設定画面URL」QR → 本体ページ）から地図を開く
2. 地図を動かし、**中央のレティクル（照準）を目的地に合わせる**（中心が常に選択地点）。「現在地へ移動」でGPSジャンプ可
3. 地名は逆ジオコーディングで自動入力（任意・編集可）
4. 「本体に設定を保存」→ 本体の `/save` に座標を送信。**再起動せず**反映し、**同一タブのまま本体ページに戻ります**

### 仕組みと制約
- 地図ページ（HTTPS）→ 本体（HTTP, `GET /save?lat=&lon=&name=&tz=`）は**トップレベル遷移**で送信（Mixed Content回避）。本体は保存後に状態ページ(`/`)へリダイレクト
- `.local` が解決しない端末では、地図ページ上部「送信先デバイス」を本体IPに変更すればOK
- 詳細な設計・データモデル・エンドポイント仕様は [docs/SPEC.md](docs/SPEC.md) を参照

## ドキュメント
- **[docs/SETUP.md](docs/SETUP.md)** — 書き込み手順（環境構築・Partition設定・LittleFSアップロード・`secrets.h`・データ構造）
- **[docs/SPEC.md](docs/SPEC.md)** — 仕様書（場所のハイブリッド指定、dtベース判定、API/エンドポイント、Mixed Content対策、複数台運用）

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
