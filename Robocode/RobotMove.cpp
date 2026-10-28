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

}
