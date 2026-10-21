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
        .text("%1 変数名 %2 型 %3 初期値 %4")
        .dropdown("STATIC", {
            {"個別の変数", ""},
            {"クラスで共有(static)", "static"}
            })
        .textField("NAME", "item")
        .dropdown("TYPE", {
            {"整数（int）", "int"},
            {"小数（float）", "float"},
            {"倍精度の小数（double）", "double"},
            {"文字列（String）", "String"},
            {"真偽値（boolean）", "boolean"}
            })
        .textField("INITIAL_VALUE", "0")
        .javaFunction([](BlockData& data) -> std::string {
        const std::string staticText = data.text("STATIC");
        const std::string name = data.text("NAME");
        const std::string type = data.text("TYPE");
        std::string initialValue = data.text("INITIAL_VALUE");

        // 型に合わせて初期値を整える
        if (type.compare("boolean") == 0) {
            // true または false 以外なら false にする
            if (initialValue.compare("true") != 0 &&
                initialValue.compare("false") != 0) {
                initialValue = "false";
            }
        }
        else if (type.compare("String") == 0) {
            // 文字列をダブルクォーテーションで囲む
            if (initialValue.size() < 2 ||
                initialValue.front() != '"' ||
                initialValue.back() != '"') {
                initialValue = std::string("\"") + initialValue + "\"";
            }
        }
        else if (type.compare("int") == 0) {
            // 整数が空欄なら 0
            if (initialValue.empty()) {
                initialValue = "0";
            }
        }
        else if (type.compare("float") == 0) {
            // 小数が空欄なら 0.0f
            if (initialValue.empty()) {
                initialValue = "0.0f";
            }
            else if (initialValue.find_first_of(".fF") == std::string::npos) {
                // 小数点も f もなければ .0f を追加
                initialValue += ".0f";
            }
        }
        else if (type.compare("double") == 0) {
            // 倍精度の小数が空欄なら 0.0
            if (initialValue.empty()) {
                initialValue = "0.0";
            }
        }

        // Java の変数宣言を作成
        std::string result;

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
        .tooltip("変数に新しい値を代入します");

    // ③ 変数の値を取得する
    Block("robot_variable_get")
        .category("変数")
        .text("変数 %1")
        .textField("NAME", "item")
        .output("Number")
        .java("{NAME}")
        .tooltip("変数に保存されている値を取得し、計算や比較に使います");


}