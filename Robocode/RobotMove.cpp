// =====================================================================
//  RobotMove.cpp  ―  「動き」のブロック
// 
// 
//  2026/10/16 片山 サンプルコード追加
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
        .dropdown("DIR", {
            {"右", "turnRight"},
            {"左", "turnLeft"}
        })  // {画面に出る文字, Java に入る文字}
        .number("ANGLE", 90, 0, 360)
        .tooltip("ロボットの向きを変えます")
        .java("{DIR}({ANGLE});");

    //砲塔旋回
    Block("robot_turn_gun")
        .category("動き")
        .text("砲塔が %1 に %2 度回る")
        .dropdown("DIR", { 
            {"右", "turnGunRight"}, 
            {"左", "turnGunLeft"} 
        })
        .number("ANGLE", 90, 0, 360)
        .tooltip("砲塔の向きを変えます")
        .java("{DIR}({ANGLE});");

    //レーダー旋回
    Block("robot_turn_rader")
        .category("動き")
        .text("レーダーが %1 に %2 度回る")
        .dropdown("DIR", { 
            {"右", "turnRadarRight"}, 
            {"左", "turnRadarLeft"} 
        })
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


    //===============
    //サンプルコード
    //===============

    // 前進・後退しながら砲塔を回す
    Block("robot_sample_aheadAndBack")
        .category("動き2")
        .text("前進・後退しながら砲塔を回す")
        .tooltip("前進、砲塔の回転、後退を組み合わせたサンプルです")
        .color(210)
        .java(
            "ahead(100);\n"
            "turnGunRight(360);\n"
            "back(100);\n"
            "turnGunRight(360);"
        );

    //ぐにゃぐにゃ動く
    Block("robot_sample_limpRunnning")
        .category("動き2")
        .text("ぐにゃぐにゃ動く")
        .tooltip("前進、車体の回転、待機を組み合わせたサンプルです")
        .color(210)
        .java(
            "setAhead(40000);\n"
            "setTurnRight(90);\n"
            "waitFor(new TurnCompleteCondition(this));\n"
            "setTurnLeft(180);\n"
            "waitFor(new TurnCompleteCondition(this));\n"
            "setTurnRight(180);\n"
            "waitFor(new TurnCompleteCondition(this));"
        );


    // ぐるぐる走行
    Block("robot_sample_spinRunning")
        .category("動き2")
        .text("ぐるぐる走行")
        .tooltip("車体を旋回させながら前進するサンプルです")
        .color(210)
        .java(
            "setTurnRight(10000);\n"
            "setMaxVelocity(5);\n"
            "ahead(1000);"
        );
}
