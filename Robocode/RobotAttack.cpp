#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotAttack)
{
    // パワー〇〇で撃つ  →  fire(〇〇);
    Block("robot_fire")                                   // ブロックの名前（ほかとかぶらない英語の名前）
        .category("攻撃")                                  // ツールボックスのどのカテゴリに出すか
        .text("パワー %1 で撃つ")                          // ブロックに表示する文（%1 に下の入力欄が入る）
        .number("POWER", 1, 0.1, 3)                        // インプット：名前, 最初の数字, 最小, 最大
        .tooltip("弾を撃ちます。パワーが大きいほど強いけど、自分のエネルギーも多く減ります")
        .check([](BlockData& b) {                          // 自分で書くチェック
        if (b.number("POWER") > 2) {                   //   パワーが2より大きいとき
            b.warning("パワーが大きいと、エネルギーがすぐ減ります");  // 注意を出す
        }
            })
        .java("fire({POWER})");                           // 作られるJava（{POWER} に入力した数字が入る）
}