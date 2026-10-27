// =====================================================================
//  RobotCalculation.cpp  ―  「計算」のブロック
// 
// 2026/10/16 片山　計算クラスの作成
// 2026/10/27 片山　プログラムの整理
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
*number(name, value)            : 数値入力欄を追加                例: .number("DIST", 100)
*number(name, value, min)       : 最小値付きの数値入力欄          例: .number("DIST", 100, 0)
*number(name, value, min, max)  : 最小値・最大値付きの数値入力欄  例: .number("ANGLE", 90, 0, 360)
*dropdown(name, opts)           : 選択肢を追加                    例: .dropdown("DIR", options)
*valueInput(name)               : 値を受け取る接続口を追加        例: .valueInput("ANGLE")
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


ROBOCODE_BLOCKS(RobotCalculation)
{

    //--------------------
    // 1.基本の計算
    //--------------------
    
    // 演算子
    Block("robot_calculation_operator")
        .category("計算")
        .text("%1 %2 %3")
        .valueInput("A")
        .dropdown("OP", {
            {"＋", "+"},
            {"－", "-"},
            {"×", "*"},
            {"÷", "/"}
            })

        .valueInput("B")
        .output("Number")
        .tooltip("2つの数を指定した演算子で計算します")
        .java("({A} {OP} {B})");


    // ランダムな数
    Block("math_random")
        .category("計算")
        .text("%1 から %2 のランダムな数")
        .valueInput("MIN", "Number")
        .valueInput("MAX", "Number")
        .output("Number")
        .tooltip("指定した範囲からランダムな数を作ります\n"
            "使い方 : 動きを読まれにくくするのに使います")
        .java("({MIN} + Math.random() * ({MAX} - {MIN}))");

    // 絶対値
    Block("math_abs")
        .category("計算")
        .text("%1 の絶対値")
        .valueInput("A", "Number")
        .output("Number")
        .tooltip("マイナスをとった数です\n"
                "（例: -30 → 30、30 → 30）")
        .java("Math.abs({A})");

    // ----------------
    // 2.角度の計算
    // ----------------
    Block("math_normalize_angle")
        .category("計算")
        .text("%1 回る角度を近い向きに直す")
        .valueInput("A", "Number")
        .output("Number")
        .tooltip("自分の車体の正面から見て角度を -180〜180 の範囲に直します\n"
            "使い方 : 270 なら -90 になるので、近い方に回れます")
        .java("robocode.util.Utils.normalRelativeAngleDegrees({A})");

    //-----------------------
    //3. 1つの数を使う計算
    //-----------------------
    Block("java_math_1")
        .category("計算")
        .text("%1 の %2")
        .valueInput("A", "Number")
        .dropdown("FN", {
            {"平方根（√）",           "sqrt"},
            {"四捨五入",              "round"},
            {"切り捨て",              "floor"},
            {"切り上げ",              "ceil"},
            {"度 → ラジアン",          "toRadians"},
            {"ラジアン → 度",          "toDegrees"},
            })
            .output("Number")
        .tooltip("1つの数を使って計算します\n"
            "平方根：2乗すると元の数になる数を求めます\n"
            "四捨五入：整数に近い値に丸めます\n"
            "切り捨て：小数部分を切り捨てます\n"
            "切り上げ：小数部分を切り上げます\n"
            "度 → ラジアン：度で表した角度を変換します\n"
            "ラジアン → 度：ラジアンで表した角度を変換します")
        .java("Math.{FN}({A})");

    //-----------------------
    //4. 2つの数を使う計算
    //-----------------------
    Block("java_math_2")
        .category("計算")
        .text("%1 と %2 の %3")
        .valueInput("A", "Number")
        .valueInput("B", "Number")
        .dropdown("FN", {
            {"〇の△乗",               "pow"},
            {"斜めの長さ（√(A²+B²)）", "hypot"},
            {"方向の角度(ラジアン）", "atan2"},//2つの数から、向きを表す角度を求める
            { "小さいほう",              "min" },
            { "大きいほう",              "max" },
            })
            .output("Number")
        .tooltip("2つの数を使った応用計算です\n"
            "〇の△乗：1つ目の数を、2つ目の数だけ掛け合わせます\n"
            "斜めの長さ：2つの長さから、斜めの長さを求めます\n"
            "方向の角度：X方向とY方向の値から、向きの角度を求めます（ラジアン）\n"
            "※方向の角度は、1つ目にY方向、2つ目にX方向の値を指定します\n"
            "小さいほう：2つの数のうち、小さいほうを求めます\n"
            "大きいほう：2つの数のうち、大きいほうを求めます")
        .java("Math.{FN}({A}, {B})");

    //-------------------------
    // 5. 三角関数（度で指定）
    //-------------------------
    Block("java_math_trig")
        .category("計算")
        .text("%1 度 の %2")
        .valueInput("A", "Number")
        .dropdown("FN", { {"sin", "sin"}, {"cos", "cos"}, {"tan", "tan"} })
        .output("Number")
        .tooltip("角度（度）から sin・cos・tan を計算します")
        .java("Math.{FN}(Math.toRadians({A}))");

    //------------------------------
    // 6. 三角関数（ラジアンで指定）
    //-------------------------------
    Block("java_math_trig_rad")
        .category("計算")
        .text("%1 ラジアン の %2")
        .valueInput("A", "Number")
        .dropdown("FN", { {"sin", "sin"}, {"cos", "cos"}, {"tan", "tan"} })
        .output("Number")
        .tooltip("ラジアンから sin・cos・tan を計算します")
        .java("Math.{FN}({A})");

    //-------------------------
    // 7. 特別な数
    //-------------------------
    Block("java_constant")
        .category("計算")
        .text("%1")
        .dropdown("C", {
            {"円周率 π",        "Math.PI"},
            {"一番大きい数",     "Double.MAX_VALUE"},
            {"一番小さい正の数", "Double.MIN_VALUE"},
            {"無限大",           "Double.POSITIVE_INFINITY"},
            })
            .output("Number")
        .tooltip("計算に使う特別な数です\n"
                 "円周率：円の計算などに使う数（約3.14）\n"
                 "最大の有限数：double型で表せる最大の有限数\n"
                 "最小の正の浮動小数点数：double型で表せる最小の正の値\n"
                 "正の無限大：無限に大きいことを表す特別な値")
        .java("{C}");

    // ================================================================
    // 8. NaN（数値として扱えない計算結果）の確認
    // ================================================================
    Block("java_double_isnan")
        .category("計算")
        .text("%1 がNaNか調べる")
        .valueInput("A", "Number")
        .output("Boolean")
        .tooltip("計算結果がNaNかどうかを調べます。\n"
                  "例：0.0 ÷ 0.0 の計算結果")
        .java("Double.isNaN({A})");
}

