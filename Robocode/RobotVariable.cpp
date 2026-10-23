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
        .text("公開範囲 %1 %2 型 %3 変数名 %4 初期値 %5 %6")
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
        // 初期値は編集可能な数字ブロックを最初から表示し、別の値ブロックにも置き換えられる。
        .optionalValueInput("INITIAL_VALUE_BLOCK", "Number")
        .dropdown("INITIAL_BOOLEAN", {
            {"false", "false"},
            {"true", "true"}
            })
        .javaFunction([](BlockData& data) -> std::string {
        const std::string accessText = data.text("ACCESS");
        const std::string staticText = data.text("STATIC");
        const std::string name = data.text("NAME");
        const std::string type = data.text("TYPE");
        std::string initialValue = data.text("INITIAL_VALUE_BLOCK");
        const bool hasRealValueBlock = data.hasConnectedValue("INITIAL_VALUE_BLOCK");
        const std::string booleanInitialValue = data.text("INITIAL_BOOLEAN");

        // 実際の値ブロックが接続されている場合はそれを優先する。
        // 接続されていない boolean 型では true/false の選択肢を使用する。
        if (type == "boolean" && !hasRealValueBlock) {
            initialValue = booleanInitialValue;
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
        else if (type == "String") {
            if (initialValue != "null" &&
                (initialValue.size() < 2 || initialValue.front() != '"' || initialValue.back() != '"')) {
                // 接続式はそのまま使う。通常の数字 shadow などは文字列の初期値として扱わない。
                if (initialValue == "0") initialValue = "\\";
            }
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

    // 真偽値の値ブロック。boolean 型の変数や条件式に接続して使います。
    Block("robot_boolean_true")
        .category("変数")
        .text("真（true）")
        .output("Boolean")
        .java("true")
        .tooltip("条件が正しいことを表す true です。");

    Block("robot_boolean_false")
        .category("変数")
        .text("偽（false）")
        .output("Boolean")
        .java("false")
        .tooltip("条件が正しくないことを表す false です。");

    // null（何も参照していない状態）
    Block("java_null")
        .category("変数")
        .text("null")
        .output("String")
        .tooltip("変数が何も参照していない状態を表します")
        .java("null");
}