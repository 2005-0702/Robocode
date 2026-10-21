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
        .text("公開範囲 %1 %2 変数名 %3 型 %4 初期値 %5")
        .dropdown("ACCESS", {
            {"指定なし", ""},
            {"どこからでも使える", "public"},
            {"このクラス内だけ", "private"}
            })
        .dropdown("STATIC", {
            {"個別の変数", ""},
            {"クラスで共有", "static"}
            })
        .textField("NAME", "item")
        .dropdown("TYPE", {
            {"整数", "int"},
            {"小数", "float"},
            {"倍精度の小数", "double"},
            {"文字列", "String"},
            {"真偽値", "boolean"}
            })
        .textField("INITIAL_VALUE", "0")
        .javaFunction([](BlockData& data) -> std::string {
        const std::string accessText = data.text("ACCESS");
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
            // null は「文字列 null」ではなく、値がまだないことを表す Java の特別な値です。
            // そのため、ダブルクォーテーションで囲まず、そのまま出力します。
            if (initialValue.compare("null") != 0 &&
                (initialValue.size() < 2 ||
                 initialValue.front() != '"' ||
                 initialValue.back() != '"')) {
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
        .tooltip("変数に新しい値を代入します");

    // ③ 変数の値を取得する
    Block("robot_variable_get")
        .category("変数")
        .text("変数 %1")
        .textField("NAME", "item")
        .output("Number")
        .java("{NAME}")
        .tooltip("変数に保存されている値を取得し、計算や比較に使います");



    // ④-② 特殊な初期値を持つ変数（Double.MAX_VALUE）
    // Java クラス直下のフィールドとして出力します。
    Block("robot_variable_target_distance")
        .category("変数")
        .text("敵までの距離の初期値を最大値にする")
        .classMember()
        .java("private double targetDistance = Double.MAX_VALUE;")
        .tooltip(
            "targetDistance を double 型で宣言し、初期値に Double.MAX_VALUE を設定します。\n"
            "Double.MAX_VALUE は double 型で表せる最大の有限値です。"
        );

    // ④-③ 敵の情報をまとめる内部クラス
    // Java クラス直下に配置する必要があるため、classMember を指定します。
    Block("robot_enemy_info_class")
        .category("変数")
        .text("敵の情報をまとめる EnemyInfo クラス")
        .classMember()
        .java(
            "private static class EnemyInfo {\n"
            "    String name;\n"
            "    double v_x;\n"
            "    double v_y;\n"
            "    double v_distance;\n"
            "    double v_energy;\n"
            "    long lastSeen;\n"
            "}"
        )
        .tooltip(
            "敵の名前、位置、距離、エネルギー、最後に確認した時刻をまとめるクラスです。\n"
            "ロボット本体のクラスの中に配置してください。"
        );

    // ④-④ EnemyInfo を複数保存するリスト
    Block("robot_enemy_info_list")
        .category("変数")
        .text("敵の情報を保存する enemies リスト")
        .classMember()
        .java(
            "private final List<EnemyInfo> enemies = new ArrayList<EnemyInfo>();"
        )
        .tooltip(
            "EnemyInfo を複数保存するリストです。\n"
            "使用する Java ファイルの先頭に java.util.List と java.util.ArrayList の import が必要です。"
        );

}