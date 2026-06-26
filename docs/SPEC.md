# Umbrella Alert 仕様書（おでかけ傘アラート）

> このドキュメントは **現在のコードを「正」** とした設計書です。
> 場所のハイブリッド指定（プリセット都道府県＋カスタム緯度経度/地名）、外部GitHub Pages地図、
> dtベースの雨判定、設定の再起動なし即時反映までを反映しています。
> ステータスは原則すべて **実装済み ✅**（将来拡張のみ 🛠️ で明記）。

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

## 3. 場所指定モデル（ハイブリッド）✅
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

## 4. 初期設定フロー ✅
**WiFi → 場所** の順。場所は本体の状態ページ（mDNS到達）で完結し、より細かい指定は外部地図ページを使う。

### 4-1. WiFi設定（共通・1画面キャプティブポータル）✅
1. 起動時、Preferencesに保存済みWiFiが無ければ **APモード**（SSID:`Umbrella-Alert-XXXX` / PASS:`12345678` / IP:`192.168.4.1`）。
2. スマホがAPに接続すると、キャプティブポータルが**1画面のWiFi設定フォームを直接表示**（`onNotFound`／`/settings`）。
   併せて「保存後の場所設定」ステップ案内と**本体URL（`http://umbrella-alert-xxxx.local`）のコピー導線**を表示。
3. SSID/パスワードを入力して保存（`/setap`）→ 再起動 → 自宅WiFiへ接続。
   - Display版は本体LCDにもAP接続用QR/URLを表示。

### 4-2. 場所設定（本体の状態ページ）✅
WiFi接続済みの状態で、スマホを自宅WiFiに戻し **`http://umbrella-alert-xxxx.local`**（状態ページ）を開く。
- **47都道府県のプルダウン**から選ぶ（`/setpref`）。Display/NanoC6共通。
- 変更時のみ保存ボタンが有効化。保存は **Ajax＋トースト通知**で、**ページ遷移も再起動もなし**（その場で天気再取得＝§9）。
- 「🌧️ 雨の通知（1〜24h）」もここで設定（`/setnotify`）。

### 4-3. 場所設定（外部GitHub Pages地図・任意座標）✅
都道府県より細かく指定したい場合。GPS（ブラウザGeolocation）とLeaflet地図は**HTTPS必須**のため、
ページは外部の **GitHub Pages（HTTPS）** に置く（[`web/`](../web/)、`.github/workflows/deploy-pages.yml` で自動デプロイ）。
1. 状態ページの「🗺 地図から指定する」リンク（`?ip=umbrella-alert-xxxx.local`）から**同一タブ**で地図ページへ。
2. 地図中央のレティクルを目的地に合わせる（中心が常に選択地点）。「現在地へ移動」でGPSジャンプ可。地名は逆ジオコーディングで自動入力（編集可）。
3. 「本体に設定を保存」→ 本体の `/save?lat=&lon=&name=&tz=` に送信（§6）。
4. 本体は受信値を保存し、**再起動せず**に天気を再取得。`/save` は本体の状態ページ（`/`）へリダイレクトするので、**同一タブのまま本体ページに戻る**。
   - カスタム座標は都道府県とは別スロット（`cust_*`）にも保存され、プルダウンの「カスタム（地名）」でいつでも復元可能（§5）。

---

## 5. データモデル（Preferences）
不揮発メモリ（名前空間 `weather_app` ／ WiFiは `wifi-config`）に以下を保存する。

| 名前空間 | キー | 型 | 用途 | 状態 |
|----------|------|----|------|------|
| `wifi-config` | `WIFI_SSID` / `WIFI_PASSWD` | String | WiFi資格情報 | ✅ |
| `weather_app` | `city_index` | int | プリセット都市の選択（Display LCDの都市選択用） | ✅ |
| `weather_app` | `loc_mode` | int | 0=PRESET / 1=CUSTOM（Display。NanoC6は常にlat/lon） | ✅ |
| `weather_app` | `lat` / `lon` | String | アクティブな場所の座標 | ✅ |
| `weather_app` | `loc_name` | String | アクティブな場所の地名（表示用） | ✅ |
| `weather_app` | `tz_offset` | long | アクティブな場所のタイムゾーン補正(秒) | ✅ |
| `weather_app` | `cust_lat` / `cust_lon` | String | **カスタム場所スロット**の座標（地図で指定。都道府県へ切替後も保持） | ✅ |
| `weather_app` | `cust_name` / `cust_tz` | String/long | カスタム場所スロットの地名・TZ（「カスタム」で復元） | ✅ |
| `weather_app` | `notify_hrs` | int | 雨の通知＝直近何時間先まで判定するか(1〜24h。既定`FORECAST_CHECK_HOURS`) | ✅ |
| `weather_app` | `beep_idx` | int | ビープ音量プリセット番号（Display） | ✅ |

---

## 6. デバイス側API（HTTPエンドポイント）
| メソッド/パス | モード | 役割 | 状態 |
|---------------|--------|------|------|
| メソッド/パス | モード | 役割 | 状態 |
|---------------|--------|------|------|
| `GET /` `GET /settings` `onNotFound` | AP | **1画面のWiFi設定**（キャプティブポータルが直接表示） | ✅ |
| `GET /setap?ssid=..&pass=..` | AP | WiFi保存→再起動。完了画面に本体URLコピー導線 | ✅ |
| `GET /refresh-networks` | AP | ネットワーク再スキャン(JSON) | ✅ |
| `GET /` | 通常 | 状態表示＋**都道府県選択(全47)**＋地図ページ導線＋雨の通知。稼働時間はJSでライブ更新 | ✅ |
| `GET /setpref?pref=N[&ajax=1]` | 通常 | 都道府県を保存→即時反映。`pref=-1`で保存済みカスタムを復元。`ajax=1`は新地名を返す | ✅ |
| `GET /setnotify?hours=N[&ajax=1]` | 通常 | 雨の通知(1〜24h)を保存→即時反映 | ✅ |
| `GET /save?lat=..&lon=..&name=..&tz=..` | 通常 | カスタム場所を保存→即時反映し、状態ページ(`/`)へ303リダイレクト | ✅ |
| `GET /reset` | 通常 | WiFi設定リセット（Webボタンは廃止。本体操作で実行） | ✅ |

> 保存系（`/setpref` `/setnotify` `/save`）は **`ESP.restart()` せず** `reloadWeatherApi()` でその場反映。
> `/setpref` `/setnotify` は状態ページから **Ajax＋トースト**で呼ばれ、ページ遷移しない。

---

## 7. 起動時判定アルゴリズム ✅
```
起動
 ├─ WiFi資格情報あり？
 │   ├─ なし → APモード（1画面WiFi設定）       [§4-1]
 │   └─ あり → WiFi接続 → NTP同期(configTime)
 │              ├─ Display: loc_mode==CUSTOM かつ lat/lon あり → lat/lonで天気取得
 │              │           それ以外 → プリセット都市ID(city_index)で取得
 │              └─ NanoC6 : 常に lat/lon（都道府県/カスタムとも座標）で取得
 └─ 通常動作ループ（§9）
```

---

## 8. セキュリティ/制約（Mixed Content）✅
外部GitHub Pages(HTTPS) → 本体(HTTP) の通信は、`fetch`/画像などの**サブリソース取得だと Mixed Content でブロック**される。
本実装は **トップレベル遷移（ページ・ナビゲーション）は許可される**性質を使って回避している：

- 地図ページの「保存」は `window.location.href = 'http://<host>/save?...'` で**同一タブのトップレベル遷移**を行う。
- 本体の `/save` は保存後に状態ページ(`/`)へ **303 リダイレクト**するので、同じタブがそのまま本体ページに戻る。
- 本体名は mDNS（`umbrella-alert-xxxx.local`）で到達。`.local` が解決しない端末向けに、地図ページ上部の
  「送信先デバイス」リンクから本体IP指定にも切替できる（`?ip=` で渡す）。
- バックアップとして、場所は**本体の状態ページ（同一LAN・HTTP）の都道府県プルダウン**だけでも完結する（外部ページ不要）。

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

## 11. 実装状況（すべて実装済み ✅）
1. ✅ 天気取得のlat/lon対応（`checkWeatherForecast()` のCUSTOM分岐）。
2. ✅ Preferences拡張（`loc_mode/lat/lon/loc_name/tz_offset` ＋ カスタムスロット`cust_*` ＋ `notify_hrs`）。
3. ✅ 本体の状態ページに**47都道府県プルダウン**＋`/setpref`＋雨の通知`/setnotify`（Display/NanoC6共通）。
4. ✅ 外部GitHub Pages地図（GPS＋Leaflet＋逆ジオコーディング）→ `/save`。同一タブ遷移＋`/`リダイレクト（§8）。
5. ✅ dtベースの雨判定（§9）、設定の**再起動なし即時反映**（Ajax＋トースト）、複数台個体識別（§13）。

> 将来拡張余地 🛠️: lat/lon からのTZ自動推定、予報の3時間より細かい解像度（API依存）。

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
| `GET /setpref?pref=N` | 通常 | 都道府県(代表座標)を保存→即時反映（再起動なし） |
| `GET /save?lat=&lon=&name=&tz=` | 通常 | カスタム場所を保存→即時反映（§6と同じ） |
| `GET /setnotify?hours=N` | 通常 | 雨の通知(1〜24h)を保存→即時反映（状態ページの「🌧️雨の通知」） |
| `GET /reset` | 通常 | WiFi設定リセット |

> 場所はすべて**緯度経度**で扱う（都道府県プリセット＝県庁所在地の座標 / 地図＝任意座標）。
> 画面が無いので場所選択は**Web（状態ページの47都道府県ドロップダウン or 地図）**で行い、反映は即時（再起動なし、その場で天気再取得）。
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
