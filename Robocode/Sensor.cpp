#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotSensor)
{
    // レーダーを敵にロックする
    Block("robot_radar_lock")
        .category("センサー")
        .text("レーダーを敵にロックする")
        .tooltip("見つけた敵の方へレーダーを向け直して、ずっと敵を見続けます")
        .java(R"({
        double radarTurn = robocode.util.Utils.normalRelativeAngleDegrees(
        getHeading() + e.getBearing() - getRadarHeading());
        turnRadarRight(radarTurn + (radarTurn < 0 ? -20 : 20));   // 少し行き過ぎるように回す
        scan();
        })");
        //.onlyIn("robot_event_enemyDetected");
    // 敵との距離
    Block("enemy_distance")
        .category("センサー")
        .text("敵との距離")
        .output("Number")
        .tooltip("見つけた敵までの距離です")
        .java("e.getDistance()")
        .onlyIn("robot_event_enemyDetected");

    // 敵のエネルギー
    Block("enemy_energy")
        .category("センサー")
        .text("敵のエネルギー")
        .output("Number")
        .tooltip("見つけた敵の残りエネルギーです")
        .java("e.getEnergy()")
        .onlyIn("robot_event_enemyDetected");

    // 敵の速さ
    Block("enemy_velocity")
        .category("センサー")
        .text("敵の速さ")
        .output("Number")
        .tooltip("見つけた敵の速さです（止まっていると 0）")
        .java("e.getVelocity()")
        .onlyIn("robot_event_enemyDetected");
}