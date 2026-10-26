// =====================================================================
//  RobotMove.cpp  ―  「動き」のブロック
// 
// 
//  2026/10/16 片山 サンプルコード追加
//  2026/10/22 片山 setAdjustをちゃんと使えるようにする
//  2026/10/23 片山 プログラムの整理
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotMove)
{
    // 進む・下がる
    Block("act_move")
        .category("動き")
        .text("%1 %2")
        .dropdown("DIRECTION", {
            {"進む", "ahead"},
            {"下がる", "back"}
            })
        .valueInput("DIST", "Number")
        .tooltip("進むか下がるかを選び、移動する距離を指定します")
        .java("{DIRECTION}({DIST});");

    // 車体・砲塔・レーダーを右または左に回す
    Block("act_turn")
        .category("動き")
        .text("%1 を %2 に %3 度回す")
        .dropdown("PART", {
            {"車体", "turn"},
            {"砲塔", "turnGun"},
            {"レーダー", "turnRadar"}
            })
        .dropdown("DIRECTION", {
            {"右", "Right"},
            {"左", "Left"}
            })
        .valueInput("ANGLE", "Number")
        .tooltip("車体・砲塔・レーダーを選んだ方向と角度で回します")
        .java("{PART}{DIRECTION}({ANGLE});");

       

    // 車体・砲塔・レーダーを独立して動かす
    Block("robot_keep_direction")
        .category("動き")
        .text("%1 をする %2")
        .dropdown("PART", {
            {"レーダーを車体から独立","RADAR_ROBOT"},
            {"砲塔を車体から独立","GUN_ROBOT"},
            {"レーダーを砲塔から独立","RADAR_GUN"} })
        .dropdown("ENABLED", {
            {"有効", "true"},
            {"無効", "false"}
                })
        .tooltip("車体や砲塔が回っても、選んだ部分は\n"
            "つられて回らなくなります。")
              
        .javaFunction([](BlockData& data) -> std::string {
        const std::string part = data.text("PART");
        const std::string enabled = data.text("ENABLED");

        if (part == "RADAR_ROBOT") {
            return "setAdjustRadarForRobotTurn(" + enabled + ");";
        }
        else if (part == "GUN_ROBOT") {
            return "setAdjustGunForRobotTurn(" + enabled + ");";
        }
        else if (part == "RADAR_GUN") {
            return "setAdjustRadarForGunTurn(" + enabled + ");";
        }

        return "";
        });

  

    //===============
    //サンプルコード
    //===============

    // 前進・後退しながら砲塔を回す
    Block("robot_sample_aheadAndBack")
        .category("動き")
        .text("前進・後退しながら砲塔を回す")
        .tooltip("前進、砲塔の回転、後退を組み合わせたサンプルです")
        .color(210)
        .java(
            "ahead(100);\n"
            "turnGunRight(360);\n"
            "back(100);\n"
            "turnGunRight(360);"
        );

    //曲がりながら動く
    Block("robot_sample_limpRunnning")
        .category("動き")
        .text("曲がりながら動く")
        .tooltip("前進、車体の回転、待機を組み合わせたサンプルです")
        .color(210)
        .java(R"(
for (int step = 0; step < 6; step++) {    // 右に曲がりながら進む（15度 × 6回 = 90度）
    turnRight(15);
    ahead(25);
}
for (int step = 0; step < 12; step++) {   // 左に曲がりながら進む（15度 × 12回 = 180度）
    turnLeft(15);
    ahead(25);
}
for (int step = 0; step < 6; step++) {    // 右に曲がりながら進む（90度。向きが元にもどる）
    turnRight(15);
    ahead(25);
}
    )");


    // ぐるぐる走行
    Block("robot_sample_spinRunning")
        .category("動き")
        .text("ぐるぐる走行")
        .tooltip("車体を旋回させながら前進するサンプルです")
        .color(210)
        .java("turnRight(30);\n"
            "ahead(60);");


    // 勝利の回転
    Block("robot_sample_victorySpin")
        .category("動き")
        .text("勝利の回転")
        .tooltip("車体を大きく回転させて、勝利を表現するサンプルです")
        .java(R"(
         turnRight(360);
     )");


  
}
