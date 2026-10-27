// =====================================================================
//  RobotControl.cpp  ―  「制御」のブロック
// 
// 2026/10/15 片山　if文,for文,and,orの追加
// 2026/10/26 片山　プログラムの整理
// 2026/10/27 片山　ブロックを変数の入る形にする。
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
    // 条件が合うときだけ、中の命令を実行する
    Block("robot_if")
        .category("制御")
        .text("もし %1 なら %2")
        .valueInput("COND", "Boolean")  // 条件を入れる穴
        .statements("DO")              // 実行する命令を入れる穴
        .tooltip("条件に合うときだけ、中の命令を実行します")
        .java("if ({COND}) {\n"
            "    {DO}\n"
            "}");

    // 条件によって、実行する命令を切り替える
    Block("robot_if_else")
        .category("制御")
        .text("もし %1 なら %2 でなければ %3")
        .valueInput("COND", "Boolean")  // 条件を入れる穴
        .statements("DO")              // 条件が「はい」のときの命令
        .statements("ELSE")            // 条件が「いいえ」のときの命令
        .tooltip("条件に応じて、実行する処理を切り替えます")
        .java("if ({COND}) {\n"
            "    {DO}\n"
            "} else {\n"
            "    {ELSE}\n"
            "}");

    // 指定した回数だけ、中の命令を繰り返す
    Block("robot_for")
        .category("制御")
        .text("%1 回繰り返す %2")
        .valueInput("COUNT", "Number")
        .statements("DO")     // 繰り返す命令   
        .tooltip("指定した回数だけ、中の命令を繰り返します")
        .javaFunction([](BlockData& b) {
        static int counter = 0;

        // (ポイント)繰り返しごとに違う変数名を使う
        std::string i = "i" + std::to_string(counter++);

        std::string count = b.text("COUNT");
        std::string inner = b.text("DO");
        std::string indented;

        // 繰り返す命令を読みやすいように字下げする
        size_t start = 0;
        while (start <= inner.size()) {
            size_t end = inner.find('\n', start);
            if (end == std::string::npos) end = inner.size();

            indented += "    " + inner.substr(start, end - start) + "\n";
            start = end + 1;
        }

        // Javaのfor文を作る
        return "for (int " + i + " = 0; " + i + " < "
            + count + "; " + i + "++) {\n"
            + indented + "}";
            });

      

    //条件が成立している間、中の処理を繰り返す(while文)
    Block("robot_while")
        .category("制御")
        .text("条件 %1 の間繰り返す %2")
        .valueInput("COND", "Boolean")
        .statements("DO")
        .tooltip("条件が成立している間、中の処理を繰り返します\n"
                 "※条件が変わらないと、止まらなくなることがあります")
        .java("while ({COND}) {\n"
            "    {DO}\n"
            "}");

    // 2つの値を比べる
    Block("robot_compare")
        .category("制御")
        .text("%1 %2 %3")
        .valueInput("A", "")              // 左側の値
        .dropdown("OP", {
            {"<", "<"},
            {">", ">"},
            {"≦","<="},
            {"≧",">="},
            {"=", "=="},
            {"≠","!="},
            })
        .valueInput("B", "")         // 右側
        .output("Boolean")           // 比べた結果は「はい／いいえ」
        .tooltip("2つの値を比べます")
        .java("({A} {OP} {B})");


    // 条件を複数つなげる（AND / OR）
    Block("robot_logic")
        .category("制御")
        .text("%1 %2 %3")
        .valueInput("A", "")
        .dropdown("OP", {
            {"and", "&&"},
            {"or", "||"}
            })
        .valueInput("B", "")
        .output("Boolean")
        .tooltip("and : 両方の条件が成立とき、\n"
                 "or : どちらかが成立したのときに\n"
                 "「はい」になります")
        .java("({A} {OP} {B})");

    // はい / いいえ
    Block("logic_boolean_value")
        .category("制御")
        .text("%1")
        .dropdown("BOOL", { {"はい", "true"}, {"いいえ", "false"} })
        .output("Boolean")
        .tooltip("「はい」か「いいえ」を選びます\n"
            "条件を指定するときに使います")
        .java("{BOOL}");

    //条件の結果を反対にする
    Block("control_not")
        .category("制御")
        .text("%1 ではない")
        .valueInput("COND", "Boolean")
        .output("Boolean")
        .tooltip("「はい」と「いいえ」を逆にします")
        .java("(!{COND})");

    //確率で成立する
    Block("control_chance")
        .category("制御")
        .text("%1 ％の確率で")
        .valueInput("COUNT", "Number")
        .output("Boolean")
        .tooltip("決めた確率で「はい」になります\n"
            "動きをランダムに変えたいときに使います")
        .java("(Math.random() * 100 < {COUNT})");

    // ---------------- 記憶（変数） ----------------
    //   memA, memB, memC はロボットのクラスに用意しておく（ラウンドが変わると 0 にもどる）

    //選んだ記憶に数字を保存
    Block("memory_set")
        .category("制御")
        .text("記憶 %1 を %2 にする")
        .dropdown("MEM", { 
            {"A", "memA"},
            {"B", "memB"},
            {"C", "memC"} 
            })
        .valueInput("VALUE", "Number")
        .tooltip("選んだ記憶に数字を入れます\n"
                 "※ラウンドが変わると0に戻ります")
        .java("{MEM} = {VALUE};");

    //記憶している数字に、新しい数字を足す
    Block("memory_add")
        .category("制御")
        .text("記憶 %1 に %2 を足す")
        .dropdown("MEM", {
            {"A", "memA"},
            {"B", "memB"},
            {"C", "memC"} 
            })
        .valueInput("VALUE", "Number")
        .tooltip("記憶している数字に数字を足します\n"
                 "マイナスの数字を足すと、引き算になります")
        .java("{MEM} += {VALUE};");

    //記憶している数字を取り出す
    Block("memory_get")
        .category("制御")
        .text("記憶 %1")
        .dropdown("MEM", {
            {"A", "memA"},
            {"B", "memB"},
            {"C", "memC"}
            })
        .output("Number")
        .tooltip("選んだ記憶に入っている数字を使います\n"
                 "計算や条件の判断に使います\n"
                 "まだ数字を保存していない場合は、0です")
        .java("{MEM}");
    

}