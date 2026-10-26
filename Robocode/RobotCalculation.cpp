// =====================================================================
//  RobotCalculation.cpp  ―  「計算」のブロック
// 
// 2026/10/16 片山　計算クラスの作成
//
// %1・・引数の第1引数
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

    // 角度をラジアンに変換する
    Block("math_to_radians")
        .category("計算")
        .text("角度 %1 をラジアンに変換")
        .valueInput("ANGLE", "Number")
        .output("Number")
        .tooltip("度数法の角度をラジアンに変換します")
        .java("Math.toRadians({ANGLE})");

    // 〇〜〇 のランダムな数
    Block("math_random")
        .category("計算")
        .text("%1 から %2 のランダムな数")
        .valueInput("MIN", "Number")
        .valueInput("MAX", "Number")
        .output("Number")
        .tooltip("毎回ちがう数になります。動きを読まれにくくするのに使います")
        .java("({MIN} + Math.random() * ({MAX} - {MIN}))");

    // 絶対値
    Block("math_abs")
        .category("計算")
        .text("%1 の絶対値(0以外の値はプラスになる)")
        .valueInput("A", "Number")
        .output("Number")
        .tooltip("マイナスをとった数です（-30 なら 30）")
        .java("Math.abs({A})");

    // 角度を -180〜180 に直す
    Block("math_normalize_angle")
        .category("計算")
        .text("%1 回る角度を近い方に直す")
        .valueInput("A", "")
        .output("Number")
        .tooltip("自分の車体の正面から見て角度を -180〜180 に直します。"
            "270 なら -90 になるので、近い方に回れます")
        .java("robocode.util.Utils.normalRelativeAngleDegrees({A})");

    // Math：1つの数を使う計算
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
        .tooltip("Java の Math を使った計算です")
        .java("Math.{FN}({A})");

    // Math：2つの数を使う計算
    Block("java_math_2")
        .category("計算")
        .text("%1 と %2 の %3")
        .valueInput("A", "Number")
        .valueInput("B", "Number")
        .dropdown("FN", {
            {"〇の△乗",               "pow"},
            {"斜めの長さ（√(A²+B²)）", "hypot"},
            {"角度（atan2、ラジアン）", "atan2"},
            { "小さいほう",              "min" },
            { "大きいほう",              "max" },
            })
            .output("Number")
        .tooltip("Java の Math を使った計算です")
        .java("Math.{FN}({A}, {B})");

    // 角度（度）で sin・cos・tan
    Block("java_math_trig")
        .category("計算")
        .text("%1 度 の %2")
        .valueInput("A", "Number")
        .dropdown("FN", { {"sin", "sin"}, {"cos", "cos"}, {"tan", "tan"} })
        .output("Number")
        .tooltip("角度（度）の sin・cos・tan です（Java の Math はラジアンなので、中で直しています）")
        .java("Math.{FN}(Math.toRadians({A}))");

    // ラジアンの sin・cos・tan（Java の Math そのまま）
    Block("java_math_trig_rad")
        .category("計算")
        .text("%1 ラジアン の %2")
        .valueInput("A", "Number")
        .dropdown("FN", { {"sin", "sin"}, {"cos", "cos"}, {"tan", "tan"} })
        .output("Number")
        .tooltip("角度（ラジアン）の sin・cos・tan です。getHeadingRadians() などの値に使います")
        .java("Math.{FN}({A})");

    // 決まった数（Math・Double）
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
        .tooltip("Java に最初から用意されている数です")
        .java("{C}");

    // Double：数ではない（計算できなかった）か
    Block("java_double_isnan")
        .category("計算")
        .text("%1 が 数ではない（NaN）")
        .valueInput("A", "Number")
        .output("Boolean")
        .tooltip("0 ÷ 0 のように、計算できなかったときに「はい」になります")
        .java("Double.isNaN({A})");
}

