// =====================================================================
//  SensorPlus.cpp  ―  組み合わせて使う「センサー」のブロック
//
//  ここのブロックは、数字や「はい／いいえ」を出すだけ。
//  「計算」「制御」「数字で動く」ブロックと組み合わせて、作戦を自分で考えられるようにする。
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotSensorPlus)
{
    // ---------------- 敵を発見したとき に使えるもの ----------------

    // 敵の方向（自分の向きから見て何度か）
    Block("enemy_bearing")
        .category("センサー")
        .text("敵の方向（自分から見た角度）")
        .output("Number")
        .tooltip("自分の向きから見て、敵が何度の方向にいるかです。正面 0、右がプラス、左がマイナス（-180〜180）")
        .java("e.getBearing()")
        .onlyIn("robot_event_enemyDetected");

    // 敵の方位（画面の上を 0 度とした向き）
    Block("enemy_absolute_bearing")
        .category("センサー")
        .text("敵の方位（画面の上が 0 度）")
        .output("Number")
        .tooltip("画面の上を 0 度として、敵がどの方位にいるかです（0〜360）。砲塔やレーダーを向けるときに使います")
        .java("robocode.util.Utils.normalAbsoluteAngleDegrees(getHeading() + e.getBearing())")
        .onlyIn("robot_event_enemyDetected");

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
        .tooltip("自分の向きから見て、当たった弾が飛んできた方向です（-180〜180）")
        .java("e.getBearing()")
        .onlyIn("robot_event_hitByBullet");

    // 当たった弾の向き
    Block("bullet_heading")
        .category("センサー")
        .text("当たった弾の向き")
        .output("Number")
        .tooltip("当たった弾が進んでいた向きです（画面の上が 0 度）。+180 すると、弾が飛んできたほうの向きになります")
        .java("e.getHeading()")
        .onlyIn("robot_event_hitByBullet");

    // ぶつかった壁の方向
    Block("wall_bearing")
        .category("センサー")
        .text("ぶつかった壁の方向")
        .output("Number")
        .tooltip("自分の向きから見て、ぶつかった壁がある方向です（-180〜180）")
        .java("e.getBearing()")
        .onlyIn("robot_event_hitWall");

    // ぶつかったロボットの方向
    Block("hit_robot_bearing")
        .category("センサー")
        .text("ぶつかったロボットの方向")
        .output("Number")
        .tooltip("自分の向きから見て、ぶつかったロボットがいる方向です（-180〜180）")
        .java("e.getBearing()")
        .onlyIn("robot_event_hitEnemy");

    // ---------------- いつでも使えるもの ----------------

    // 壁までの距離
    Block("wall_distance")
        .category("センサー")
        .text("いちばん近い壁までの距離")
        .output("Number")
        .tooltip("上下左右の壁のうち、いちばん近い壁までの距離です")
        .java("Math.min(Math.min(getX(), getBattleFieldWidth() - getX()), "
              "Math.min(getY(), getBattleFieldHeight() - getY()))");

    // バトル場のまん中の方向
    Block("center_bearing")
        .category("センサー")
        .text("バトル場の真ん中の方向")
        .output("Number")
        .tooltip("自分の向きから見て、バトル場の真ん中が何度の方向にあるかです（-180〜180）")
        .java("robocode.util.Utils.normalRelativeAngleDegrees(Math.toDegrees(Math.atan2("
              "getBattleFieldWidth() / 2 - getX(), getBattleFieldHeight() / 2 - getY())) - getHeading())");
}
