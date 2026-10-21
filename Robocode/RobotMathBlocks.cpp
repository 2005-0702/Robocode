// =====================================================================
//  RobotMathBlocks.cpp  ―  「計算」のブロック
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotMathBlocks)
{
    // 〇 + 〇
    Block("math_arith")
        .category("計算")
        .text("%1 %2 %3")
        .valueInput("A", "Number")
        .dropdown("OP", { {"+", "+"}, {"-", "-"}, {"×", "*"}, {"÷", "/"} })
        .valueInput("B", "Number")
        .output("Number")
        .tooltip("2つの数字を計算します")
        .java("({A} {OP} {B})");

    // 〇〜〇 のランダムな数
    Block("math_random")
        .category("計算")
        .text("%1 から %2 のランダムな数")
        .number("MIN", 0)
        .number("MAX", 100)
        .output("Number")
        .tooltip("毎回ちがう数になります。動きを読まれにくくするのに使います")
        .java("({MIN} + Math.random() * ({MAX} - {MIN}))");

    // 小さい方・大きい方
    Block("math_minmax")
        .category("計算")
        .text("%1 と %2 の %3")
        .valueInput("A", "Number")
        .valueInput("B", "Number")
        .dropdown("FN", { {"小さい方", "min"}, {"大きい方", "max"} })
        .output("Number")
        .tooltip("2つの数字のうち、小さい方か大きい方です")
        .java("Math.{FN}({A}, {B})");

    // 絶対値
    Block("math_abs")
        .category("計算")
        .text("%1 の絶対値")
        .valueInput("A", "Number")
        .output("Number")
        .tooltip("マイナスをとった数です（-30 なら 30）")
        .java("Math.abs({A})");

    // 角度を -180〜180 に直す
    Block("math_normalize_angle")
        .category("計算")
        .text("%1 を近い方に回る角度に直す")
        .valueInput("A", "Number")
        .output("Number")
        .tooltip("角度を -180〜180 に直します。270 なら -90 になるので、近い方に回れます")
        .java("robocode.util.Utils.normalRelativeAngleDegrees({A})");
}
