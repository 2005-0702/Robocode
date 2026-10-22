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
}
