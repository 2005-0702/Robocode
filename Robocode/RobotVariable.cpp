// =====================================================================
//  RobotVariable.cpp  ―  「変数」のブロック
// 
// 
//  2026/10/19 片山 変数追加
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotVariable)
{
    // ① 変数を宣言する
    Block("robot_variable_declare")
        .category("変数")
        //.classMember()
        .text("%1 変数名 %2 型 %3")
        .dropdown("STATIC", {
        {"個別の変数( )", ""},
        {"クラスで共有(static)", "static"}
            })
        .textField("NAME", "item")
        .dropdown("TYPE", {
            {"整数（int）", "int"},
            {"小数（float）", "float"},
            {"倍精度の小数（double）", "double"},
            {"文字列（String）", "String"},
            {"真偽値（boolean）", "boolean"},
          
            })
        .javaFunction([](BlockData& data) {
        const std::string staticText = data.text("STATIC");
        const std::string name = data.text("NAME");
        const std::string type = data.text("TYPE");

        std::string initialValue;

        if (type == "int") {
            initialValue = "0";
        }
        else if (type == "float") {
            initialValue = "0.0f";
        }
        else if (type == "double") {
            initialValue = "0.0";
        }
        else if (type == "String") {
            initialValue = "\"\"";
        }
        else if (type == "boolean") {
            initialValue = "false";
        }
        else {
            initialValue = "0";
        }

        std::string result;

        if (staticText == "static") {
            result += "static ";
        }

        // 型、変数名、初期値を追加する
        result += type + " " + name + " = " + initialValue + ";";

        // 完成した文字列を返す
        return result;
            })
        .tooltip(
            "変数を宣言するブロックです。\n"
            "static：付けると、同じクラスのオブジェクト間で値を共有します。\n"
            "変数名：値を保存する場所につける名前です。\n"
            "型：保存できる値の種類を指定します。"
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