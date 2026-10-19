#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotSensor)
{
    // レーダーを敵にロックする
    Block("robot_radar_lock")
        .category("センサー")
        .text("レーダーを敵にロックする")
        .tooltip("見つけた敵の方へレーダーを向け直して、ずっと敵を見続けます")
        .java(R"({
         double radar = getHeading() + e.getBearing() - getRadarHeading();
         while (radar > 180) radar -= 360;                           
         while (radar < -180) radar += 360;
         turnRadarRight(radar);
        })")
        .onlyIn("robot_event_enemyDetected");
    // 砲塔を敵にロックする
    Block("robot_gun_lock")
        .category("センサー")
        .text("砲塔を敵にロックする")
        .tooltip("見つけた敵の方へ砲塔を向け直して、ずっと敵を見続けます")
        .java(R"({
        double gun = getHeading() + e.getBearing() - getGunHeading();
        while (gun > 180) gun -= 360;
        while (gun < -180) gun += 360;
        turnGunRight(gun);
        })")
        .onlyIn("robot_event_enemyDetected");
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