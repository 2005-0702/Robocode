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
        .java("ahead({DIST});");        //この動きに対応したJavaの関数名

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
        .text("%1 に %2 度回る")
        .dropdown("DIR", {{"右", "turnRight"}, {"左", "turnLeft"}})  // {画面に出る文字, Java に入る文字}
        .number("ANGLE", 90, 0, 360)
        .tooltip("ロボットの向きを変えます")
        .java("{DIR}({ANGLE});");
}
