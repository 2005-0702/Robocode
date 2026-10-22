#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotSensor)
{
    Block("robot_lock")
        .category("センサー")
        .text("%1 を敵にロックする")
        .dropdown("PART", {
            {"レーダー", R"({
            double radar = getHeading() + e.getBearing() - getRadarHeading();
            while (radar > 180) radar -= 360;
            while (radar < -180) radar += 360;
            turnRadarRight(radar);
        })"},

            {"砲塔", R"({
            double gun = getHeading() + e.getBearing() - getGunHeading();
            while (gun > 180) gun -= 360;
            while (gun < -180) gun += 360;
            turnGunRight(gun);
        })"}
            })
        .tooltip("レーダーまたは砲塔を敵の方向に向けます")
        .java("{PART}")
        .onlyIn("robot_event_enemyDetected");
}