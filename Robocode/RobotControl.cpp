// =====================================================================
//  RobotControl.cpp  ―  「制御」のブロック
// 
// 2026/10/15 片山　if文,for文,and,orの追加
//
// =====================================================================

/* ======== BlockDef ― ブロック設定関数一覧 =============
*
*【1. 見た目の設定】
*category(name)     : カテゴリを設定                    例: .category("動き")
*text(message)      : ブロックに表示する文字            例: .text("前に %1 進む")
*tooltip(message)   : マウスを乗せたときの説明          例: .tooltip("前に進みます")
*color(hue)         : ブロックの色を設定（色相 0～360） 例: .color(210)
*
*【2. 入力欄・接続口】
*number(name, value)            : 数値入力欄を追加                           例: .number("DIST", 100)
*number(name, value, min)       : 最小値付きの数値入力欄                     例: .number("DIST", 100, 0)
*number(name, value, min, max)  : 最小値・最大値付きの数値入力欄             例: .number("ANGLE", 90, 0, 360)
*dropdown(name, opts)           : 選択肢を追加                               例: .dropdown("DIR", options)
*valueInput(name)               : 値を受け取る接続口を追加                   例: .valueInput("ANGLE")
*valueInput(name,type)          : 値を受け取る接続口を追加、片の種類を指定   例: .valueInput("ANGLE")
* 
*statements(name)               : 命令を入れる接続口を追加        例: .statements("DO")
*
*【3. ブロックの種類】
*output(type)   : 値を出力するブロックに設定     例: .output("Number")
*event()        : イベントブロックとしてマーク   例: .event()
*
*【4. Javaコード生成】
*java(template)     : Javaコードのテンプレートを設定 例: .java("ahead({DIST});")
*javaFunction(func) : 関数を使ってJavaコードを生成   例: .javaFunction(myFunction)
*
*【5. チェック・制限】
*warnIfZero(name, msg)  : 値が0の場合に警告         例: .warnIfZero("DIST", "距離が0です")
*onlyIn(eventType)      : 使用できるイベントを指定  例: .onlyIn("roboteventscanned")
*check(func)            : 独自のチェック処理を追加  例: .check(myCheckFunction)
*/

#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotControl)
{
    
    //もし〇〇なら
    Block("robot_if")
        .category("制御")
        .text("もし %1 なら %2")
        .valueInput("COND", "Boolean")          // 「はい」のブロックをはめる穴
        .statements("DO")                       // 中にブロックを入れる口
        .tooltip("条件に合うときだけ、中の処理を実行します")
        .java("if ({COND}) {\n"
            "    {DO}\n"
            "}");

    // もし〇〇なら○○でなければ
    Block("robot_if_else")
        .category("制御")
        .text("もし %1 なら %2 でなければ %3" )
        .valueInput("COND", "Boolean")          // 「はい／いいえ」のブロックをはめる穴
        .statements("DO")                       // 中にブロックを入れる口
        .statements("ELSE")                       
        .tooltip("条件に応じて、実行する処理を切り替えます")
        .java("if ({COND}) {\n"
            "    {DO}\n"
            "}else {\n"
          "    {ELSE}\n"
            "}");

    // 指定した回数だけ繰り返す（for文）
    Block("robot_for")
        .category("制御")
        .text("%1 回繰り返す %2")
        .number("COUNT", 5, 0)
        .statements("DO")
        .tooltip("指定した回数だけ、中の処理を繰り返します")
        .javaFunction([](BlockData& b) {
        static int counter = 0;
        std::string i = "i" + std::to_string(counter++);   // i0, i1, i2 … と毎回ちがう名前にする
        std::string inner = b.text("DO");
        std::string indented;                               // 中身を4文字ずつ字下げする
        size_t start = 0;
        while (start <= inner.size()) {
            size_t end = inner.find('\n', start);
            if (end == std::string::npos) end = inner.size();
            indented += "    " + inner.substr(start, end - start) + "\n";
            start = end + 1;
        }
        return "for (int " + i + " = 0; " + i + " < " + b.text("COUNT") + "; " + i + "++) {\n" +
            indented + "}";
            });

    //条件が成立している間、中の処理を繰り返す(while文)
    Block("robot_while")
        .category("制御")
        .text("条件 %1 の間繰り返す %2")
        .valueInput("COND", "Boolean")
        .statements("DO")
        .tooltip("条件が成立している間、中の処理を繰り返します")
        .java("while ({COND}) {\n"
            "    {DO}\n"
            "}");

    // 〇〇 < 〇〇（数字を比べる）
    Block("robot_compare")
        .category("制御")
        .text("%1 %2 %3")
        .valueInput("A", "")              // 数字のブロックをはめる穴（左）
        .dropdown("OP", {
            {"<", "<"},
            {">", ">"},
            {"=", "=="},
            {"≦","<="},
            {"≧",">="},
            {"≠","!="},
            })
        .valueInput("B", "")              // 数字のブロックをはめる穴（右）
        .output("Boolean")                      // 比べた結果は「はい／いいえ」
        .tooltip("2つの数字を比べます")
        .java("({A} {OP} {B})");


    // 条件を複数つなげる（AND / OR）
    Block("robot_logic")
        .category("制御")
        .text("%1 %2 %3")
        .valueInput("A", "Boolean")
        .dropdown("OP", {
            {"and", "&&"},
            {"or", "||"}
            })
        .valueInput("B", "Boolean")
        .output("Boolean")
        .tooltip("2つの条件をANDまたはORでつなぎます")
        .java("({A} {OP} {B})");

    // はい / いいえ（true / false）
    Block("logic_boolean_value")
        .category("制御")
        .text("%1")
        .dropdown("BOOL", { {"はい（true）", "true"}, {"いいえ（false）", "false"} })
        .output("Boolean")
        .tooltip("「はい」か「いいえ」です。もしの条件や、くり返しの条件にはめられます")
        .java("{BOOL}");
    
}