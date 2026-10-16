#include "RobocodeBlocks.h"


ROBOCODE_BLOCKS(GetGameSystem)
{
    // バトル場の幅・高さ
    Block("battlefield_size")
        .category("ゲーム情報2")
        .text("バトル場の %1")
        .dropdown("SIZE", { {"横の長さ（幅）", "Width"}, {"縦の長さ（高さ）", "Height"} })
        .output("Number")
        .tooltip("バトル場の大きさです（ふつうは 幅 800、高さ 600）")
        .java("getBattleField{SIZE}()");   // getBattleFieldWidth() か getBattleFieldHeight() になる
    //自分の横幅、縦幅
    Block("robot_size")
        .category("ゲーム情報2")
        .text("ロボットの %1")
        .dropdown("SIZE", { {"横の長さ（幅）", "Width"}, {"縦の長さ（高さ）", "Height"} })
        .output("Number")
        .tooltip("ロボットの大きさです")
        .java("get{SIZE}()");
    //総ラウンド数
    Block("num_rounds")
        .category("ゲーム情報2")
        .text("総ラウンド数")
        .output("Number")
        .tooltip("総ラウンド数です")
        .java("getRoundNum() + 1");
    //現在のラウンド数
    Block("round_num")
        .category("ゲーム情報2")
        .text("現在のラウンド数")
        .output("Number")
        .tooltip("現在のラウンド数です")
        .java("getRoundNum()");
    //ラウンド開始からの経過時間
    Block("time")
        .category("ゲーム情報2")
        .text("ラウンド開始からの経過時間")
        .output("Number")
        .tooltip("ラウンド開始からの経過時間を取得します")
        .java("getTime()");

}
