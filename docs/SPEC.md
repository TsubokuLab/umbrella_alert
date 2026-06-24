# Umbrella Alert 仕様書（現行実装＋場所カスタム化計画）

> このドキュメントは **現在のコード（`UmbrellaAlert_Display/`）を「正」** とし、
> 旧仕様書（GitHub Pages＋GPS＋Leafletによるlat/lon設定）の有効な部分を統合した設計書です。
> 「実装済み ✅ / 計画 🛠️」を明記します。コードは本ドキュメント時点では未変更（仕様先行）。

- リポジトリ: https://github.com/TsubokuLab/umbrella_alert
- 対象: M5Stack Core2 / CoreS3（LCD版 = `UmbrellaAlert_Display/`） / NanoC6（LED版 = `UmbrellaAlert_M5NanoC6/`、§12）
- 関連: 環境構築は [SETUP.md](SETUP.md)

---

## 1. 概要
12時間以内（OpenWeatherMapの3時間予報×4枠）の雨予報をチェックし、傘の要否を
LCDとNeoPixel LEDで通知する置き型ガジェット。GPS非搭載のため、**場所の指定方法**が
設計の要点になる。本仕様では場所指定を **ハイブリッド（プリセット都市 ＋ カスタム緯度経度/地名）** とする。

---

## 2. 現状の実装（=正）✅
| 機能 | 内容 | 主な実装 |
|------|------|----------|
| WiFi初期設定 | APモード（キャプティブポータル）で内蔵Webページから入力 | [wifi_manager.h](../UmbrellaAlert_Display/wifi_manager.h) / [web_server.h](../UmbrellaAlert_Display/web_server.h) |
| 天気取得 | OpenWeatherMap forecast（3h×N）。`FORECAST_CHECK_HOURS`先までの雨を判定 | [UmbrellaAlert_Display.ino](../UmbrellaAlert_Display/UmbrellaAlert_Display.ino) `checkWeatherForecast()` |
| 場所指定 | プリセット都市リストを本体ボタン/タッチで選択 | [settings.h](../UmbrellaAlert_Display/settings.h) |
| タイムゾーン | 都市ごとに `CityInfo.tzOffset` を保持 | [settings.h](../UmbrellaAlert_Display/settings.h) |
| 設定保存 | Preferences（不揮発メモリ） | 下記§5 |
| 表示 | メイン/予報/詳細/設定 画面、傘要否、降水確率、気温 | [ui_helper.h](../UmbrellaAlert_Display/ui_helper.h) |
| LED | 雨=青の回転、晴れ=橙の明滅。別コアの専用タスクで一定fps | `ledTask()` |

> 旧仕様より進んでいる点（APキャプティブポータル、forecastベースの傘判定、LED専用タスク）は
> そのまま維持する。

---

## 3. 場所指定モデル（ハイブリッド）🛠️
場所は2モードのいずれかを保持する。

| モード | 内容 | OpenWeatherMapクエリ |
|--------|------|----------------------|
| `PRESET` | 既存の都市リスト（東京/大阪…）から選択 | `?id=<cityId>` |
| `CUSTOM` | 緯度経度＋地名をユーザーが設定 | `?lat=<lat>&lon=<lon>` |

- `CityInfo`（プリセット）は既存どおり `id / name / tzOffset` を保持。
- `CUSTOM` は `lat / lon / name / tzOffset` を保持（§5）。
- 表示用の「地名」は、PRESET=都市名、CUSTOM=ユーザー入力名 を使う。
- タイムゾーンは CUSTOM でも保持（GPS地点に対し、当面は手入力 or 既定 `DEFAULT_TIMEZONE_OFFSET`。
  将来 lat/lon からTZ推定APIを使う拡張余地あり）。

---

## 4. 初期設定フロー（2系統）
**①既存＝本体APページ（バックアップとして維持）✅** と
**②新規＝外部GitHub Pages（GPS＋地図でlat/lon設定）🛠️** の2系統を用意する。

### 4-1. WiFi設定（共通・既存のまま）✅
1. 起動時、Preferencesに保存済みWiFiが無ければ **APモード**（SSID:`Umbrella-Alert` / PASS:`12345678` / IP:`192.168.4.1`）。
2. 本体LCDにQR（WiFi接続用・設定ページ用）を表示。
3. スマホがAPに接続し、`http://192.168.4.1` の内蔵ページからSSID/パスワードを入力（`/setap`）。
4. 保存して再起動 → 自宅WiFiへ接続。

### 4-2. 場所設定（新規・外部GitHub Pages）🛠️
WiFi接続済み（＝本体が自宅LANのIPを持つ）状態で行う。GPS（ブラウザGeolocation）と地図は
**HTTPS必須**のため、設定ページは外部の **GitHub Pages（HTTPS）** に置く。

1. 本体は自分の **LAN IP** を取得し、設定ページURLにIPを引数で付与してQR表示。
   例: `https://tsubokulab.github.io/umbrella_alert/?ip=192.168.10.23`
   （フロントエンドは [`web/`](../web/) に実装。`.github/workflows/deploy-pages.yml` でPagesへ自動デプロイ）
2. スマホでQRを読み、HTTPSページを開く。URL引数から本体IPをJSで取得・保持。
3. 「現在地を取得」ボタン → Geolocation APIで高精度lat/lon取得（HTTPSなので許可される）。
4. Leaflet地図に座標を反映（中心移動・ズーム・ピン表示）。地図タップでピンを微調整し座標を上書き。
5. 地名を入力。
6. 「送信」→ 本体LAN IPの `/save` に lat/lon/name/tz を送信（§6）。
7. 本体は受信値をPreferencesに保存し再起動 → CUSTOMモードで天気取得開始。

### 4-3. 場所設定（バックアップ・本体内蔵ページ）✅→🛠️
GPS/地図が使えない/使いたくない場合のフォールバック。本体APまたはLAN上の内蔵HTTPページに
**地名＋緯度経度の手入力欄**を追加し、同じ保存処理（§5）に流す。完全オフラインで完結。
（既存の都市プリセット選択もこの系統に含む。）

---

## 5. データモデル（Preferences）
不揮発メモリに以下を保存する。既存キーは維持し、CUSTOM用を追加する。

| 名前空間 | キー | 型 | 用途 | 状態 |
|----------|------|----|------|------|
| `wifi-config` | `WIFI_SSID` / `WIFI_PASSWD` | String | WiFi資格情報 | ✅ |
| `weather_app` | `city_index` | int | プリセット都市の選択 | ✅ |
| `weather_app` | `loc_mode` | int | 0=PRESET / 1=CUSTOM | 🛠️ |
| `weather_app` | `lat` / `lon` | float/String | カスタム座標 | 🛠️ |
| `weather_app` | `loc_name` | String | カスタム地名（表示用） | 🛠️ |
| `weather_app` | `tz_offset` | long | カスタムのタイムゾーン補正(秒) | 🛠️ |
| `weather_app` | `notify_hrs` | int | 雨の通知＝直近何時間先まで判定するか(1〜24h。既定`FORECAST_CHECK_HOURS`) | ✅ |

---

## 6. デバイス側API（HTTPエンドポイント）
| メソッド/パス | モード | 役割 | 状態 |
|---------------|--------|------|------|
| `GET /` | 通常 | ステータス表示・リセット導線 | ✅ |
| `GET /settings` | AP | WiFi設定フォーム | ✅ |
| `GET /setap?ssid=..&pass=..` | AP | WiFi保存→再起動 | ✅ |
| `GET /refresh-networks` | AP | ネットワーク再スキャン(JSON) | ✅ |
| `GET /reset` | 通常 | WiFi設定リセット | ✅ |
| `GET /save?lat=..&lon=..&name=..&tz=..` | 通常 | **カスタム場所を保存→再起動** | 🛠️ |
| `GET /setnotify?hours=N` | 通常 | **雨の通知(1〜24h)を保存→再起動**。状態ページの「🌧️雨の通知」プルダウンから | ✅ |
| `GET /setcity?index=..` | 通常 | プリセット都市を保存（[settings.h](../UmbrellaAlert_Display/settings.h) `setCityByIndex()` を呼ぶ） | 🛠️ |

> `/save` は外部HTTPSページからの **no-cors** リクエストを受ける（§8）。パラメータはGETクエリで受け、
> 値検証後にPreferencesへ保存し `ESP.restart()`。

---

## 7. 起動時判定アルゴリズム
```
起動
 ├─ WiFi資格情報あり？
 │   ├─ なし → APモード（WiFi設定）       [§4-1]
 │   └─ あり → WiFi接続
 │              ├─ loc_mode == CUSTOM かつ lat/lon あり → CUSTOMで天気取得   [§3]
 │              └─ それ以外 → PRESET（city_index）で天気取得
 └─ 通常動作ループ（§9）
```
※現行は city_index 既定で常にPRESET。CUSTOM分岐の追加が🛠️。

---

## 8. セキュリティ/制約（重要）
外部GitHub Pages(HTTPS) → 本体(HTTP) の通信には **Mixed Content（混在コンテンツ）** の壁がある。
旧仕様の「no-cors で回避」は **CORS の話であり、Mixed Content 制限は別**。実際にはモダンブラウザは
HTTPSページからHTTPへの能動的リクエストをブロックし得る。実装時は以下を前提に検証する：

- `fetch(url, { mode: 'no-cors' })` … CORSプリフライトは回避できるが、Mixed Content でブロックされる場合がある。
- ローカルIP宛は **Private Network Access** の扱いで挙動が変わる（ブラウザ/バージョン依存）。
- 回避策の候補（実装時に最も通るものを採用）:
  1. 画像ビーコン（`new Image().src='http://ip/save?...'`）でGETを飛ばす。
  2. ユーザーに「保存されない場合は本体内蔵ページ(§4-3)で手入力」フォールバックを案内。
  3. 取得した座標を **QR/短文字列で表示** し、ユーザーが本体内蔵ページに貼る方式（混在回避）。
- いずれにせよ **本体内蔵HTTPページ（§4-3）をバックアップとして常備** し、外部ページが失敗しても
  設定を完了できるようにする（今回の方針）。

---

## 9. 通常動作モード（天気表示）
1. 起動後、保存値（PRESET=id / CUSTOM=lat/lon）でリクエストURLを生成。
2. `UPDATE_INTERVAL`（既定30分）ごとにOpenWeatherMap forecastを取得・解析。
3. **雨判定はdtベースの時刻判定**: 各予報枠の `dt`(UTC epoch) が「現在時刻＋`notify_hrs`時間」以内に入るものだけを対象に、`RAIN_THRESHOLD`%以上 or 雨/霧雨/雷雨なら「傘が必要」。
   - `notify_hrs` は本体ページで 1〜24h に変更可（既定`FORECAST_CHECK_HOURS`=12h）。
   - forecastは3時間刻みのpoint予報のため**最小解像度は3時間**（dt窓の長さ自体は1時間単位で指定可）。
   - 現在時刻はNTP同期（Display/NanoC6とも `configTime`）。**未同期時は枠数=ceil(notify_hrs/3)** での判定にフォールバック。
   - 直近N時間に予報枠が1つも入らない小さいN時は「雨なし」。表示用データは次の枠から補完。
4. メイン画面・LED・予報画面に反映。失敗時は `WEATHER_MAX_RETRIES` 回までリトライ。

---

## 10. 設定パラメータ（config.h 抜粋）
場所カスタム化に関係する既存定数。CUSTOM追加時もここを起点にする。
- `API_KEY` / `UNITS` / `LANGUAGE`
- `CITY_*`（プリセット都市ID）, `DEFAULT_CITY_ID`
- `DEFAULT_TIMEZONE_OFFSET`（カスタムTZ未設定時のフォールバック）
- `FORECAST_CHECK_HOURS` / `FORECAST_DISPLAY_COUNT`
- `UPDATE_INTERVAL` / `RAIN_THRESHOLD` / `WEATHER_MAX_RETRIES` / `WEATHER_RETRY_DELAY`
- ビープ: `BEEP_VOLUME`(OFF/SMALL/MEDIUM/LARGE) / `BEEP_FREQ` / `BEEP_DURATION`

---

## 11. 実装ステップ（提案順）
1. **天気取得をlat/lon対応に**：`checkWeatherForecast()` に CUSTOM 分岐（`?lat=&lon=`）を追加。
2. **Preferences拡張**：`loc_mode/lat/lon/loc_name/tz_offset` の保存・読込。起動時分岐（§7）。
3. **本体内蔵ページに手入力欄＋`/save`＋`/setcity`**（バックアップ系統・オフライン完結）。
4. **外部GitHub Pagesページ**：GPS＋Leaflet＋地名入力。本体IPをQRで受け渡し、§8の方式で `/save` 送信。
5. 既存のプリセット都市選択（本体ボタン）は維持（ハイブリッド）。

> 1〜3は本体ファームのみで完結（オフライン検証可）。4は外部ページ＋Mixed Content検証が必要。
> まず1〜3を固め、4を後段で進めるのが安全。

---

## 12. NanoC6版（画面なし・LEDのみ）✅
`UmbrellaAlert_M5NanoC6/` は LCD版の機能制限版。**画面・タッチが無く、出力はリングLEDのみ**。
WiFi・場所の設定はすべてWeb（AP設定モード）で行う。Display版のWeb部品
（`wifi_manager.h` / `web_server.h` / `styles.h`）を移植し、場所モデル（PRESET/CUSTOM）も共通。

### 12-1. 構成
- 設定保存（Preferences）・場所ロジックは `settings.h`（都市プリセット＋カスタム緯度経度）
- WiFi接続/AP/mDNSは `wifi_manager.h`、設定ページ/エンドポイントは `web_server.h`
- `.ino` は LED表示（別コアtask）＋ブートモード判定＋天気取得のみ（描画系は無し）

### 12-2. 初期設定フロー
1. 未設定で起動 → **設定モード（AP: `Umbrella-Alert`/`12345678`, IP `192.168.4.1`）**
2. スマホでAP接続 → `http://192.168.4.1` の設定ページで **WiFiのみ保存** → 再起動して接続
   （地域選択はフォームから廃止。IPは接続後に確定するため場所設定は後段）
3. **場所設定**：スマホを自宅Wi-Fiへ繋ぎ直し、`http://umbrella-alert.local`（状態ページ）を開く →
   **47都道府県のドロップダウン**から選ぶ、または**本体が実IPを埋めた「🗺 地図で設定」リンク**から
   GitHub Pages地図 → `/save` で細かく指定（ハイブリッド）
4. 本体ボタン**長押し**（`RESET_HOLD_MS`）でWiFiリセット → 設定モードへ

> IPアドレス問題: AP設定中は本体のLAN IPが未確定（再起動後にDHCPで決まる）。
> そのため「IPを添えてGitHub Pagesへ」は**接続後の状態ページ**（mDNS `umbrella-alert.local` で到達、実IPを保持）で行う。

> 画面が無いため、接続情報（AP SSID/PASS、`http://192.168.4.1`、`http://umbrella-alert.local`）は
> **本体に貼るシール**で案内する。AP IPは固定なので事前印刷できる。

### 12-3. LEDによる状態表示（画面の代わり）
**外付けリング**と**本体内蔵RGB LED（インジケーター）**の両方に同じ状態を表示する。
リング未接続でも内蔵LEDだけで状況が分かる（内蔵LEDのピンは `ONBOARD_LED_PIN`/`ONBOARD_LED_POWER_PIN`）。

原則：**点滅＝操作待機 / 呼吸・点灯＝正常**。

| 状態 | 外付けリング | 内蔵LED（インジケーター） |
|------|------|------|
| 設定モード（AP待ち） | シアンのゆっくり呼吸 | **白の呼吸（やや速め）** |
| WiFi接続中（待機） | 黄色のコメットが回転 | **白点滅** |
| 通常・雨（正常） | 青の回転アニメーション | **青の呼吸** |
| 通常・晴れ（正常） | オレンジのゆっくり明滅 | **オレンジの呼吸** |
| リセット長押し中（待機） | 赤点滅（0.25秒） | 赤点滅（0.25秒） |
| リセット確定 | 赤点灯（1秒）→消灯 | 赤点灯（1秒）→消灯 |

> 本体ボタン長押し：押している間は赤点滅、`RESET_HOLD_MS`到達で**赤を1秒点灯→消灯→WiFi設定リセット＆再起動**。
> 途中で離せばキャンセル。内蔵LEDは状態色で常時表示するので、リング未接続でも状況が分かる。

### 12-4. エンドポイント（NanoC6）
| メソッド/パス | モード | 役割 |
|---------------|--------|------|
| `GET /settings` `onNotFound` | AP | **1画面のWiFi設定**（キャプティブポータルが直接表示） |
| `GET /setap` `GET /refresh-networks` | AP | WiFi保存→再起動 / ネット再スキャン |
| `GET /` | 通常 | 状態表示＋**都道府県選択(全47)**＋地図ページ導線＋リセット |
| `GET /setpref?pref=N` | 通常 | 都道府県(代表座標)を保存→再起動 |
| `GET /save?lat=&lon=&name=&tz=` | 通常 | カスタム場所を保存→再起動（§6と同じ） |
| `GET /setnotify?hours=N` | 通常 | 雨の通知(1〜24h)を保存→再起動（状態ページの「🌧️雨の通知」） |
| `GET /reset` | 通常 | WiFi設定リセット |

> 場所はすべて**緯度経度**で扱う（都道府県プリセット＝県庁所在地の座標 / 地図＝任意座標）。
> 画面が無いので場所選択は**Web（状態ページの47都道府県ドロップダウン or 地図）**で行い、反映は再起動。
> キャプティブポータルは1画面（タイトル＋WiFi選択）に集約。

---

## 13. 複数台運用（個体識別）✅
家庭内で複数台使うと全台が同一の AP SSID / mDNS 名で衝突するため、**MAC由来の固有サフィックス**で一意化する（Display/NanoC6共通）。

- 末尾2バイトのMAC→4桁HEX `XXXX`。`wifi_manager.h` の `initDeviceIdentity()` が setup で確定し
  `g_apSsid` / `g_mdnsHost` に保持（`esp_read_mac` 使用）。
- **AP SSID** = `Umbrella-Alert-XXXX`（`WiFi.softAP(g_apSsid)`）
- **mDNSホスト名** = `umbrella-alert-xxxx`（`MDNS.begin(g_mdnsHost)`）→ `umbrella-alert-xxxx.local` で個別到達
  （NanoC6はAPP_MODEでもmDNS起動。Display版も本対応でAPP_MODEにmDNSを追加）
- 本体は自分の名前を**シリアル／設定・完了・状態ページ／画面(Display)**に表示
- NanoC6完了ページは**個体専用の地図URL `?ip=umbrella-alert-xxxx.local`＋コピーボタン**を表示
  （キャプティブポータルはOSが自動で閉じる＆AP上はネット無しのため自動遷移は不採用。コピー/再アクセスで誘導）
- GitHub Pages（`web/`）は送信先が編集可能欄なので変更最小。`?ip=umbrella-alert-xxxx.local` で個体指定
- OWM APIキーは共通でよい（家庭の数台なら無料枠に対し誤差）
