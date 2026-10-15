#pragma once
// =====================================================================
//  RobotBuilder.h  ―  できた Java を Robocode で使えるようにする
//
//    1. Robocode のフォルダを決める（最初の1回だけ選んでもらい、覚えておく）
//    2. robots\パッケージ名\ロボット名.java に保存する
//    3. javac でコンパイルして、ロボット名.class を作る
// =====================================================================
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

// 保存とコンパイルの結果
struct BuildResult {
    bool ok = false;        // 保存とコンパイルが両方うまくいったら true
    std::string message;    // 画面に出す説明（例：「Robocode で使えるようになりました」）
    std::string detail;     // javac が出したエラーなど、くわしい内容
    std::string javaPath;   // 保存した .java の場所
};

// Robocode のフォルダを返す
//   まだ決まっていなければ、フォルダを選ぶ画面を出す。キャンセルされたら空の文字列。
//   forceChoose を true にすると、決まっていても選び直してもらう。
std::wstring GetRobocodeFolder(HWND owner, bool forceChoose = false);

// Java を保存して、javac でコンパイルする
BuildResult BuildRobot(const std::wstring& robocodeFolder,
                       const std::string& packageName,
                       const std::string& robotName,
                       const std::string& javaCode);

// 名前のチェック（だめなら理由を返す。よければ空の文字列）
std::string CheckRobotName(const std::string& name);
std::string CheckPackageName(const std::string& name);

// Robocode を起動する
//   このアプリから起動した Robocode がまだ開いていたら、いったん閉じてから起動し直す。
//   message には画面に出す説明が入る。起動できたら true。
bool LaunchRobocode(const std::wstring& robocodeFolder, std::string& message);
