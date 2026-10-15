// =====================================================================
//  RobotMove.cpp  ―  「動き」のブロック
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotMove)
{
    // 前に〇〇進む
    Block("robot_move_forward")
        .category("動き")              // カテゴリー
        .text("前に %1 進む")          //ブロックに表示する文
        .number("DIST", 100, 0)        //↑%1に表示される数字の入力欄
        .tooltip("ロボットを前に進めます（単位はピクセル）")    //マウスを乗せたときに出る説明
        .warnIfZero("DIST", "距離が0なので、ロボットは動きません")   // 0のときに出す注意
        .java("ahead({DIST});"); 
    

    // 後ろに〇〇下がる
    Block("robot_move_backward")
        .category("動き")
        .text("後ろに %1 下がる")
        .number("DIST", 100, 0)
        .tooltip("ロボットを後ろに下げます（単位はピクセル）")
        .warnIfZero("DIST", "距離が0なので、ロボットは動きません")
        .java("back({DIST});");

    // 右／左に〇〇度回る
    Block("robot_turn")
        .category("動き")
        .text("ロボットが %1 に %2 度回る")
        .dropdown("DIR", {{"右", "turnRight"}, {"左", "turnLeft"}})  // {画面に出る文字, Java に入る文字}
        .number("ANGLE", 90, 0, 360)
        .tooltip("ロボットの向きを変えます")
        .java("{DIR}({ANGLE});");

    //砲塔旋回
    Block("robot_turn_gun")
        .category("動き")
        .text("砲塔が %1 に %2 度回る")
        .dropdown("DIR", { {"右", "turnRight"}, {"左", "turnLeft"} })
        .number("ANGLE", 90, 0, 360)
        .tooltip("砲塔の向きを変えます")
        .java("{DIR}({ANGLE});");

    //レーダー旋回
    Block("robot_turn_rader")
        .category("動き")
        .text("レーダーが %1 に %2 度回る")
        .dropdown("DIR", { {"右", "turnRight"}, {"左", "turnLeft"} })
        .number("ANGLE", 90, 0, 360)
        .tooltip("レーダーの向きを変えます")
        .java("{DIR}({ANGLE});");

    //砲塔固定
    Block("robot_gun_keep_direction")
        .category("動き")
        .text("砲塔が固定される")
        .tooltip("砲塔の向きが固定されます")
        .java("setAdjustGunForRobotTurn(true);");

    //レーダー固定ロボット旋回時
    Block("robot_rader_keep_direction")
        .category("動き")
        .text("ロボット旋回時レーダーが固定される")
        .tooltip("ロボット旋回時レーダーが固定されます")
        .java("setAdjustRadarForRobotTurn(true);");

    //レーダー固定砲塔旋回時
    Block("Gun_rader_keep_direction")
        .category("動き")
        .text("砲塔旋回時レーダーが固定される")
        .tooltip("砲塔旋回時レーダーが固定されます")
        .java("setAdjustRadarForGunTurn(true);");
}
