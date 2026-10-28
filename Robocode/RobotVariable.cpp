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
        .text("公開範囲 %1 %2 型 %3 変数名 %4 初期値 %5 %6 %7")
        .dropdown("ACCESS", {
            {"指定なし", ""},
            {"どこからでも使える", "public"},
            {"このクラス内だけ", "private"}
            })
        .dropdown("STATIC", {
            {"個別の変数", ""},
            {"クラスで共有", "static"}
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
        // 数値型はブロック接続、文字列型は直接入力、真偽値型は「はい／いいえ」を使用する。
        .textField("INITIAL_STRING", "")
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
        const std::string stringInitialValue = data.text("INITIAL_STRING");

        // 実際の値ブロックが接続されている場合はそれを優先する。
        // 接続されていない boolean 型では true/false の選択肢を使用する。
        if (type == "boolean" && !hasRealValueBlock) {
            initialValue = booleanInitialValue;
        }
        // String 型は専用のテキスト欄から値を取得する。Java文字列として引用符を付ける。
        else if (type == "String") {
            std::string escaped;
            for (char c : stringInitialValue) {
                if (c == '\\' || c == '"') escaped += '\\';
                if (c == '\n') { escaped += "\\n"; continue; }
                if (c == '\r') { escaped += "\\r"; continue; }
                escaped += c;
            }
            initialValue = stringInitialValue.empty() ? "\"\"" : "\"" + escaped + "\"";
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
            "変数を宣言するブロックです。\n"
            "変数名：値を保存する場所につける名前です。\n"
            "型：保存する値の種類です。\n"
            "初期値：最初に入れておく値です。"
        );


    // ② 変数に値を代入する
    Block("robot_variable_assign")
        .category("変数")
        .text("%1 に %2 を代入")
        .textField("NAME", "item")
        .valueInput("VALUE", "Number")
        .java("{NAME} = {VALUE};")
        .tooltip("宣言した変数を選んで、新しい値を代入します。");

    // ③ 変数の値を取得する
    Block("robot_variable_get")
        .category("変数")
        .text("変数 %1")
        .textField("NAME", "item")
        .output("Number")
        .java("{NAME}")
        .tooltip("宣言した変数を選び、その変数の値を計算や比較に使います。");

 
    // null（何も参照していない状態）
    Block("java_null")
        .category("変数")
        .text("null")
        .output("String")
        .tooltip("変数が何も参照していない状態を表します")
        .java("null");
}