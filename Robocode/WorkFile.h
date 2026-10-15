#pragma once
// =====================================================================
//  WorkFile.h  ―  作品（組んだブロック）の保存と読み込み
//
//  作品は「〇〇.rcblocks」というファイルに保存する。中身は JSON：
//    {
//      "format": "robocode-blocks", "version": 1,
//      "robotName": "MyRobot", "package": "student",
//      "workspace": { ...Blockly のブロックのデータ... }
//    }
// =====================================================================
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

// 保存・読み込みの結果
struct WorkFileResult {
    bool ok = false;        // うまくいったら true
    bool canceled = false;  // ファイルを選ぶ画面でキャンセルされたら true
    std::string path;       // 保存した／開いたファイルの場所（UTF-8）
    std::string content;    // 読み込んだファイルの中身（UTF-8）
    std::string error;      // うまくいかなかったときの理由
};

// 作品を保存する
//   path が空なら「名前を付けて保存」の画面を出す（suggestedName が最初のファイル名になる）。
//   path があれば、その場所に上書きする。
WorkFileResult SaveWorkFile(HWND owner, const std::string& path,
                            const std::string& suggestedName, const std::string& content);

// 作品を開く（「開く」の画面を出して、中身を読み込む）
WorkFileResult OpenWorkFile(HWND owner);
