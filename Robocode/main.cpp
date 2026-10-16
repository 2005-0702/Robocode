// =====================================================================
//  main.cpp  ―― 動作確認用
//  ブロック定義・ツールボックス・Javaコード・チェック結果を表示します。
//  アプリ本体ができたら、この中身は本体側へ移してください。
// =====================================================================
#include <iostream>


#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // コンソールで日本語が文字化けしないように
#endif
    return 0;
}
