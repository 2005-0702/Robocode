// =====================================================================
//  RobotVariable.cpp  ―  「変数」のブロック
// 
// 
//  2026/10/19 片山 変数追加
//  2026/10/20 片山 変数の初期化をできるようにする
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotVariable)
{
    // ① 変数を宣言する

    Block("robot_variable_declare")
        .category("変数")
        .text("範囲 %1 %2 型 %3 変数名 %4 初期値 %5 %6")
        .dropdown("ACCESS", {
            {"基本", ""},
            {"ほかから使える", "public"},
            {"この中だけ", "private"}
            })
        .dropdown("STATIC", {
            {"個別", ""},
            {"共有", "static"}
            })
        .dropdown("TYPE", {
            {"整数", "int"},
            {"小数", "float"},
            {"倍精度の小数", "double"},
            {"長整数", "long"},
            {"文字列", "String"},
            {"真偽値", "boolean"}
            })
        .textField("NAME", "item")
        // 数値型は数値ブロック、文字列型は null などの文字列ブロックを接続する。
        // 真偽値型は「はい／いいえ」の選択肢を使う。
        .optionalValueInput("INITIAL_VALUE_BLOCK", "Number")
        .dropdown("INITIAL_BOOLEAN", {
            {"いいえ", "false"},
            {"はい", "true"}
            })
        .javaFunction([](BlockData& data) -> std::string {
        const std::string accessText = data.text("ACCESS");
        const std::string staticText = data.text("STATIC");
        const std::string name = data.text("NAME");
        const std::string type = data.text("TYPE");
        std::string initialValue = data.text("INITIAL_VALUE_BLOCK");
        const bool hasRealValueBlock = data.hasConnectedValue("INITIAL_VALUE_BLOCK");
        const std::string booleanInitialValue = data.text("INITIAL_BOOLEAN");
        // boolean 型は値ブロックではなく、はい／いいえの選択を使う。
        if (type == "boolean") {
            initialValue = booleanInitialValue;
        }
        // String 型で値ブロックが未接続なら null を初期値にする。
        // java_null や文字列ブロックが接続されていれば、その Java 式をそのまま使う。
        else if (type == "String" && !hasRealValueBlock) {
            initialValue = "null";
        }
        // 初期値の shadow がない場合は型に応じた既定値を使う。
        if (initialValue.empty()) {
            if (type == "String") initialValue = "null";
            else if (type == "boolean") initialValue = "false";
            else if (type == "float") initialValue = "0.0f";
            else if (type == "double") initialValue = "0.0";
            else if (type == "long") initialValue = "0L";
            else initialValue = "0";
        }
        // 数字ブロックの値は型に合わせて Java の表記を整える。
        // すでに式ブロック（例：Double.MAX_VALUE）が接続されている場合は式を維持する。
        if (type == "boolean") {
            if (initialValue != "true" && initialValue != "false") initialValue = "false";
        }
        else if (type == "int") {
            // 0 などの整数式はそのまま
        }
        else if (type == "float") {
            if (initialValue == "0") initialValue = "0.0f";
            else if (initialValue.find_first_of(".fF") == std::string::npos &&
                     initialValue.find("MAX_VALUE") == std::string::npos &&
                     initialValue.find("MIN_VALUE") == std::string::npos) initialValue += ".0f";
        }
        else if (type == "double") {
            if (initialValue == "0") initialValue = "0.0";
        }
        else if (type == "long") {
            if (initialValue == "0") initialValue = "0L";
            else if (initialValue.find_first_of("lL") == std::string::npos &&
                     initialValue.find_first_of(".eE") == std::string::npos) initialValue += "L";
        }

        // Java の変数宣言を作成
        std::string result;

        // public / private は、クラスの中に置く変数につける言葉です。
        // そのため、アクセス範囲 → static → 型 → 名前 の順に出力します。
        if (!accessText.empty()) {
            result += accessText + " ";
        }

        if (staticText.compare("static") == 0) {
            result += "static ";
        }

        result += type + " " + name + " = " + initialValue + ";";

        return result;
            })
        .tooltip(
            "値を保存する変数を宣言します\n"
            "範囲：変数を使える範囲を指定します\n"
            "基本：特別な指定をしません\n"
            "ほかから使える：別のクラスからも使えます\n"
            "この中だけ：宣言したクラス内で使えます\n"
            "個別：オブジェクトごとに値を持ちます\n"
            "共有：同じクラスのオブジェクト間で値を共有します\n"
            "型：保存する値の種類です\n"
            "変数名：変数の名前です\n"
            "初期値：変数を作ったときの値です"

        );


    // ② 宣言した変数に値を代入する
    Block("robot_variable_assign")
        .category("変数")
        // NAME：代入先の変数名
        // VALUE：数値や文字列を接続する入力欄
        // VARIABLE_TYPE：選択した変数の型（画面では非表示）
        // VALUE_BOOLEAN：真偽値の「はい／いいえ」
        .text("%1 に %2 %3 %4 を代入")
        .textField("NAME", "item")
        .valueInput("VALUE", "Number")
        .dropdown("VARIABLE_TYPE", {
            {"整数", "int"},
            {"小数", "float"},
            {"倍精度の小数", "double"},
            {"長整数", "long"},
            {"文字列", "String"},
            {"真偽値", "boolean"}
        })
        .dropdown("VALUE_BOOLEAN", {
            {"いいえ", "false"},
            {"はい", "true"}
        })
        .javaFunction([](BlockData& data) -> std::string {
            // 代入先の変数名と、その変数の型を取得する。
            const std::string name = data.text("NAME");
            const std::string type = data.text("VARIABLE_TYPE");

            std::string value;

            // 真偽値の場合は、入力欄ではなく「はい／いいえ」の選択を使う。
            if (type == "boolean") {
                value = data.text("VALUE_BOOLEAN");
                if (value != "true" && value != "false") {
                    value = "false";
                }
            }
            else {
                // 数値や文字列の場合は、接続されたブロックの Java 式を取得する。
                value = data.text("VALUE");

                // 値が空の場合に備えて、型に合った既定値を設定する。
                if (value.empty()) {
                    if (type == "String") value = "null";
                    else if (type == "float") value = "0.0f";
                    else if (type == "double") value = "0.0";
                    else if (type == "long") value = "0L";
                    else value = "0";
                }
            }

            // 例：score = 100;
            //     enemyName = null;
            //     isMoving = true;
            return name + " = " + value + ";";
        })
        .tooltip(
            "選んだ変数に新しい値を入れます。\n"
            "整数や小数：数値ブロックをつなぎます。\n"
            "文字列：文字列ブロックや「値なし」ブロックをつなぎます。\n"
            "真偽値：「はい」か「いいえ」を選びます。\n"
            "変数名を選ぶと、その変数の種類に合った入力欄に切り替わります。"
        );

    // ③ 変数の値を取得する
    Block("robot_variable_get")
        .category("変数")
        .text("変数 %1")
        .textField("NAME", "item")
        .output("Number")
        .java("{NAME}")
        .tooltip("変数に入っている値を、計算や条件判定に使います");

 
    // null（何も参照していない状態）
    Block("java_null")
        .category("変数")
        .text("値なし")
        .output("String")
        .tooltip("値を参照していない状態を表します\n"
            "空文字や数値の0とは異なります")
        .java("null");
}