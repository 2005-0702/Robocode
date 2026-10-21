// =====================================================================
//  AdvancedMove.cpp  ―  「同時に動く」のブロック（AdvancedRobot の set〜）
//
//  ふつうの「〇 進む」「〇 度回す」は、1つ終わるまで次に進まない（そのあいだは何もできない）。
//  ここのブロックは「予約」だけして、すぐ次に進む。
//  予約したら、最後に「まとめて動かす」を置くと、予約した動きが同時に始まる。
//    例：進む・車体を回す・砲塔を回す・撃つ を予約 → まとめて動かす → 全部いっしょに動く
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(AdvancedMove)
{
    // 〇 進む／下がる（予約）
    Block("adv_set_ahead")
        .category("予約して動く")
        .text("予約：%1 %2")
        .valueInput("DIST", "Number")
        .dropdown("DIR", { {"進む", "setAhead"}, {"下がる", "setBack"} })
        .tooltip("進む（下がる）動きを予約します。「まとめて動かす」で動き始めます")
        .java("{DIR}({DIST});");

    // 車体・砲塔・レーダーを 〇 度 右に回す（予約）
    Block("adv_set_turn")
        .category("予約して動く")
        .text("予約：%1 を右に %2 度回す")
        .dropdown("PART", { {"車体", "setTurnRight"}, {"砲塔", "setTurnGunRight"}, {"レーダー", "setTurnRadarRight"} })
        .valueInput("ANGLE", "Number")
        .tooltip("回る動きを予約します（マイナスで左に回る）。「まとめて動かす」で動き始めます")
        .java("{PART}({ANGLE});");

    // 〇 のパワーで撃つ（予約）
    Block("adv_set_fire")
        .category("予約して動く")
        .text("予約：%1 のパワーで撃つ")
        .valueInput("POWER", "Number")
        .tooltip("弾を撃つのを予約します（パワー 0.1〜3）。「まとめて動かす」で撃ちます")
        .java("setFire({POWER});");

    // まとめて動かす
    Block("adv_execute")
        .category("予約して動く")
        .text("まとめて動かす（1ターン進める）")
        .tooltip("予約した動きを全部いっしょに始めて、1ターン進めます。「ずっと」の中の最後に置きます")
        .java("execute();");

    // 一番速い速さ
    Block("adv_set_max_velocity")
        .category("同時に動く")
        .text("一番速い速さを %1 にする")
        .number("SPEED", 8, 0, 8)
        .tooltip("ロボットが進む一番速い速さを決めます（0〜8。ふつうは 8）")
        .java("setMaxVelocity({SPEED});");

    // 予約の残り
    Block("adv_remaining")
        .category("予約して動く")
        .text("予約の残り：%1")
        .dropdown("WHAT", {
            {"進む距離", "getDistanceRemaining"},
            {"車体の回転", "getTurnRemaining"},
            {"砲塔の回転", "getGunTurnRemaining"},
            {"レーダーの回転", "getRadarTurnRemaining"},
        })
        .output("Number")
        .tooltip("予約した動きが、あとどれだけ残っているかです（0 なら終わっている）")
        .java("{WHAT}()");
}
