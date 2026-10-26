// =====================================================================
//  RobotSensor.cpp  ―  組み合わせて使う「センサー」のブロック
//
//  2026/10/23 片山 プログラムの整理
// 
//  ここのブロックは、数字や「はい／いいえ」を出すだけ。
//  「計算」「制御」「数字で動く」ブロックと組み合わせて、作戦を自分で考えられるようにする。
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotSensor)
{
    // ------ 敵を発見したとき に使えるもの -------

    // 敵の方向（自分の向きから見て何度か）
    Block("enemy_bearing")
        .category("センサー")
        .text("敵の方向（自分から見た角度）")
        .output("Number")
        .tooltip("自分の正面を 0 度として、敵がどの方向にいるかを表します\n"
                 "右はプラス、左はマイナス（-180〜180）")
        .java("e.getBearing()")
        .onlyIn("robot_event_enemyDetected");

    // 敵の方位（画面の上を 0 度とした向き）
    Block("enemy_absolute_bearing")
        .category("センサー")
        .text("敵の方位（画面の上が 0 度）")
        .output("Number")
        .tooltip("フィールドの上方向を 0 度として、敵がどの方位にいるかを表します\n"
            "時計回りに角度が増えます（0～360 度）。")
        .java("robocode.util.Utils.normalAbsoluteAngleDegrees(getHeading() + e.getBearing())")
        .onlyIn("robot_event_enemyDetected");

    //敵にロック
    Block("robot_lock")
        .category("センサー")
        .text("%1 を敵にロックする")
        .dropdown("PART", {
            {"レーダー", R"(
    double radar = getHeading() + e.getBearing() - getRadarHeading();
    while (radar > 180) radar -= 360;
    while (radar < -180) radar += 360;
    turnRadarRight(radar);
        )"},

            {"砲塔", R"(
    double gun = getHeading() + e.getBearing() - getGunHeading();
    while (gun > 180) gun -= 360;
    while (gun < -180) gun += 360;
    turnGunRight(gun);
        )"}
            })
        .tooltip("レーダーまたは砲塔を敵の方向に向けます")
        .java("{PART}")
        .onlyIn("robot_event_enemyDetected");

    // 敵をもう一度探す
    Block("act_scan")
        .category("センサー")
        .text("もう一度レーダーで探す")
        .tooltip("今のレーダーの向きで、もう一度敵を探します\n"
            "※レーダーの向きに敵がいる場合「敵を発見したとき」の\n"
            "イベントが発生します")
        .java("scan();");

    // 敵が止まっている？
    Block("enemy_is_stopped")
        .category("センサー")
        .text("敵が止まっている")
        .output("Boolean")
        .tooltip("見つけた敵の速さが 0 のときに「はい」になります")
        .java("(e.getVelocity() == 0)")
        .onlyIn("robot_event_enemyDetected");

    // ---------------- ほかのイベントで使えるもの ----------------

    // 弾が飛んできた方向
    Block("bullet_from_bearing")
        .category("センサー")
        .text("弾が飛んできた方向")
        .output("Number")
        .tooltip("自分の正面を 0 度として、弾が飛んできた方向を表します\n"
                 "右はプラス、左はマイナスです（-180～180 度）")
        .java("e.getBearing()")
        .onlyIn("robot_event_hitByBullet");

    // 当たった弾の向き
    Block("bullet_heading")
        .category("センサー")
        .text("当たった弾の向き")
        .output("Number")
        .tooltip("自分に当たった弾が進んでいた向きを表します\n"
                "フィールドの上方向が 0 度で、時計回りに増えます（0～360 度）\n"
                "※ 弾が飛んできた方角は、この角度に 180 度足して求められます")
        .java("e.getHeading()")
        .onlyIn("robot_event_hitByBullet");

    // ぶつかった壁の方向
    Block("wall_bearing")
        .category("センサー")
        .text("ぶつかった壁の方向")
        .output("Number")
        .tooltip("自分の正面を 0 度として、ぶつかった壁の方向を表します\n"
                 "右はプラス、左はマイナスです（-180～180 度）")
        .java("e.getBearing()")
        .onlyIn("robot_event_hitWall");

    // ぶつかったロボットの方向
    Block("hit_robot_bearing")
        .category("センサー")
        .text("ぶつかったロボットの方向")
        .output("Number")
        .tooltip("自分の正面を 0 度として、ぶつかった敵ロボットの方向を表します\n"
                 "右はプラス、左はマイナスです（-180～180 度）")
        .java("e.getBearing()")
        .onlyIn("robot_event_hitEnemy");

    // ---------------- いつでも使えるもの ----------------

    // 壁までの距離
    Block("wall_distance")
        .category("センサー")
        .text("いちばん近い壁までの距離")
        .output("Number")
        .tooltip("ロボットの中心から、上下左右の壁までの距離を計算し、\n"
                    "最も近い壁までの距離を返します")
        .java("Math.min(Math.min(getX(), getBattleFieldWidth() - getX()), "
              "Math.min(getY(), getBattleFieldHeight() - getY()))");

    // バトル場のまん中の方向
    Block("center_bearing")
        .category("センサー")
        .text("バトル場の真ん中の方向")
        .output("Number")
        .tooltip("自分の正面を 0 度として、バトル場の中心がどの方向にあるかを表します\n"
                 "右はプラス、左はマイナスです（-180～180 度）")
        .java("robocode.util.Utils.normalRelativeAngleDegrees(Math.toDegrees(Math.atan2("
              "getBattleFieldWidth() / 2 - getX(), getBattleFieldHeight() / 2 - getY())) - getHeading())");


}
