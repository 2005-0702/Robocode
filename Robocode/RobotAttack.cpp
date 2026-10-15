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
        .java("fire({POWER});");                           // 作られるJava（{POWER} に入力した数字が入る）
    // パワー〇〇で撃つ  →  fire(〇〇);
    Block("keep_away_radar_fire")                                   // ブロックの名前（ほかとかぶらない英語の名前）
        .category("攻撃")                                  // ツールボックスのどのカテゴリに出すか
        .text("レーダーをロックした状態でパワー %1 で撃つ")                          // ブロックに表示する文（%1 に下の入力欄が入る）
        .number("POWER", 1, 0.1, 3)                        // インプット：名前, 最初の数字, 最小, 最大
        .tooltip("敵にレーダーをロックした状態で打ちます")
        .check([](BlockData& b) {                          // 自分で書くチェック
        if (b.number("POWER") > 2) {                   //   パワーが2より大きいとき
            b.warning("パワーが大きいと、エネルギーがすぐ減ります");  // 注意を出す
        }
            })
        .java("{\n"
        "    double radar = getHeading() + e.getBearing() - getRadarHeading();\n"   // レーダーを敵に向ける角度
        "    while (radar > 180) radar -= 360;\n"                                    // -180〜180度にそろえる
        "    while (radar < -180) radar += 360;\n"
        "    turnRadarRight(radar);\n"
        "    double gun = getHeading() + e.getBearing() - getGunHeading();\n"       // 砲台を敵に向ける角度
        "    while (gun > 180) gun -= 360;\n"
        "    while (gun < -180) gun += 360;\n"
        "    turnGunRight(gun);\n"
        "    fire({POWER});\n"                                                       // 撃つ
        "    scan();\n"                                                              // もう一度探す
        "}");                           // 作られるJava（{POWER} に入力した数字が入る）

}