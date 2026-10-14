// =====================================================================
//  RobotEvent.cpp  ―  「イベント」のブロック（一番上に置くブロック）
// =====================================================================
//  イベントブロックの java には、Robocode のメソッドをまるごと書く。
//  {DO} だけの行に、中に入れたブロックが字下げされて入る。
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotEvent)
{
    // スタートしたら、ずっとくりかえす
    Block("robot_event_run")
        .category("イベント")
        .text("スタートしたら、ずっと %1")
        .statements("DO")
        .event()
        .tooltip("バトルが始まったら、中のブロックをずっとくりかえします")
        .java("public void run() {\n"
              "    while (true) {\n"
              "        {DO}\n"
              "    }\n"
              "}");

    // 敵を見つけたとき
    Block("robot_event_scanned")
        .category("イベント")
        .text("敵を見つけたとき %1")
        .statements("DO")
        .event()
        .tooltip("レーダーで敵を見つけたときに動きます")
        .java("public void onScannedRobot(ScannedRobotEvent e) {\n"
              "    {DO}\n"
              "}");

    // 弾が当たったとき
    Block("robot_event_hit_by_bullet")
        .category("イベント")
        .text("弾が当たったとき %1")
        .statements("DO")
        .event()
        .tooltip("敵の弾が当たったときに動きます")
        .java("public void onHitByBullet(HitByBulletEvent e) {\n"
              "    {DO}\n"
              "}");

    // 壁にぶつかったとき
    Block("robot_event_hit_wall")
        .category("イベント")
        .text("壁にぶつかったとき %1")
        .statements("DO")
        .event()
        .tooltip("壁にぶつかったときに動きます")
        .java("public void onHitWall(HitWallEvent e) {\n"
              "    {DO}\n"
              "}");
}
