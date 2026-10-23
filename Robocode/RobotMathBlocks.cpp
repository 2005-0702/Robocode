// =====================================================================
//  RobotMathBlocks.cpp  ―  「計算」のブロック
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotMathBlocks)
{
    // 〇〜〇 のランダムな数
    Block("math_random")
        .category("計算")
        .text("%1 から %2 のランダムな数")
        .valueInput("MIN","Number")
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
                  "270 なら -90 になるので、近い方に回れます" )
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
