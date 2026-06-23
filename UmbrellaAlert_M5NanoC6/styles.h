/*
*******************************************************************************
* styles.h - スタイル生成関数
*
* HTMLページのスタイルを生成する関数群
* config.hの設定を基にスタイルを動的に生成
*******************************************************************************
*/

#ifndef STYLES_H
#define STYLES_H

#include "config.h"

// CSSスタイル生成関数
String generateCSS() {
    String css = "<style>";

    // 基本スタイル
    css += "body{";
    css += "font-family:Arial,sans-serif;";
    css += "background:linear-gradient(135deg," + String(THEME_PRIMARY_START) + "," + String(THEME_PRIMARY_END) + ");";
    css += "min-height:100vh;";
    css += "display:flex;";
    css += "align-items:center;";
    css += "justify-content:center;";
    css += "margin:0;";
    css += "padding:20px;";
    css += "box-sizing:border-box;";
    css += "}";

    // コンテナスタイル
    css += ".container{";
    css += "box-sizing:border-box;";
    css += "background:rgba(255,255,255,0.95);";
    css += "border-radius:" + String(BORDER_RADIUS) + ";";
    css += "padding:40px;";
    css += "box-shadow:0 25px 50px rgba(0,0,0,0.25);";
    css += "backdrop-filter:blur(10px);";
    css += "max-width:" + String(CONTAINER_MAX_WIDTH) + ";";
    css += "width:100%;";
    css += "animation:slideIn 0.5s ease-out;";
    css += "}";

    // アニメーション
    css += "@keyframes slideIn{";
    css += "from{opacity:0;transform:translateY(30px);}";
    css += "to{opacity:1;transform:translateY(0);}";
    css += "}";

    // タイトルスタイル
    css += "h1{";
    css += "text-align:center;";
    css += "color:#2d3748;";
    css += "margin-bottom:30px;";
    css += "font-size:28px;";
    css += "font-weight:bold;";
    css += "}";

    // ボタンスタイル
    css += ".btn{";
    css += "width:100%;";
    css += "padding:" + String(BUTTON_PADDING) + " 0px;";
    css += "background:linear-gradient(135deg," + String(THEME_SECONDARY_START) + "," + String(THEME_SECONDARY_END) + ");";
    css += "color:white;";
    css += "border:none;";
    css += "border-radius:12px;";
    css += "font-size:16px;";
    css += "font-weight:600;";
    css += "cursor:pointer;";
    css += "margin-top:10px;";
    css += "transition:all 0.3s ease;";
    css += "text-decoration:none;";
    css += "display:block;";
    css += "text-align:center;";
    css += "}";

    // ボタンホバー効果
    css += ".btn:hover{";
    css += "transform:translateY(-2px);";
    css += "box-shadow:0 8px 25px rgba(0,0,0,0.15);";
    css += "}";

    // 危険ボタン
    css += ".btn-danger{";
    css += "background:linear-gradient(135deg," + String(THEME_DANGER_START) + "," + String(THEME_DANGER_END) + ");";
    css += "}";

    // 入力フィールドスタイル
    css += "input,select{";
    css += "width:100%;";
    css += "padding:" + String(INPUT_PADDING) + ";";
    css += "border:2px solid #e2e8f0;";
    css += "border-radius:12px;";
    css += "font-size:16px;";
    css += "margin-bottom:20px;";
    css += "box-sizing:border-box;";
    css += "transition:border-color 0.3s ease;";
    css += "}";

    // フォーカス時のスタイル
    css += "input:focus,select:focus{";
    css += "outline:none;";
    css += "border-color:" + String(THEME_PRIMARY_START) + ";";
    css += "box-shadow:0 0 0 3px rgba(102,126,234,0.1);";
    css += "}";

    // 情報表示用スタイル
    css += ".info{";
    css += "background:rgba(59,130,246,0.1);";
    css += "border:1px solid rgba(59,130,246,0.2);";
    css += "border-radius:8px;";
    css += "padding:16px;";
    css += "margin:16px 0;";
    css += "color:#1e40af;";
    css += "}";

    // 成功メッセージ
    css += ".success{";
    css += "background:rgba(34,197,94,0.1);";
    css += "border:1px solid rgba(34,197,94,0.2);";
    css += "border-radius:8px;";
    css += "padding:16px;";
    css += "margin:16px 0;";
    css += "color:#166534;";
    css += "}";

    // フッター
    css += ".footer{";
    css += "text-align:center;";
    css += "margin-top:30px;";
    css += "padding-top:20px;";
    css += "border-top:1px solid #e2e8f0;";
    css += "color:#64748b;";
    css += "font-size:14px;";
    css += "}";

    // フッターリンク
    css += ".footer a{";
    css += "color:" + String(THEME_PRIMARY_START) + ";";
    css += "text-decoration:none;";
    css += "}";

    // レスポンシブ対応
    css += "@media (max-width:600px){";
    css += ".container{padding:20px;margin:10px;}";
    css += "h1{font-size:24px;}";
    css += "}";

    css += "</style>";
    return css;
}

// フッター生成関数
String generateFooter() {
    String footer = "<div class='footer'>";
    String author_tag = String(AUTHOR_NAME);
    if(String(AUTHOR_URL) != ""){
        author_tag = "<a href='" + String(AUTHOR_URL) + "' target='_blank'>" + String(AUTHOR_NAME) + "</a>";
    }
    footer += "© 2026 " + author_tag + ". All rights reserved.";
    footer += "</div>";
    return footer;
}

// 基本的なHTMLページ生成関数
String makePage(String title, String contents) {
    String s = "<!DOCTYPE html><html lang='ja'><head>";
    s += "<meta name='viewport' content='width=device-width,user-scalable=0'>";
    s += "<meta charset='UTF-8'>";
    s += "<title>" + title + " - " + String(APP_TITLE) + "</title>";
    s += "<link rel='icon' href='data:image/svg+xml,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\"><text y=\".9em\" font-size=\"90\">📶</text></svg>'>";
    s += generateCSS();
    s += "</head><body>";
    s += "<div class='container'>";
    s += contents;
    s += generateFooter();
    s += "</div>";
    s += "</body></html>";
    return s;
}

#endif // STYLES_H
