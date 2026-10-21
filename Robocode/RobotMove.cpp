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

    // 車体・砲塔・レーダーを独立して動かす
    Block("robot_keep_direction")
        .category("動き")
        .text("%1 をする ")
        .dropdown("PART", {
            {"レーダーを車体から独立","setAdjustRadarForRobotTurn"},
            {"砲塔を車体から独立","setAdjustGunForRobotTurn"},
            {"レーダーを砲塔から独立","setAdjustRadarForGunTurn"} })
            .tooltip("車体や砲塔が回っても、えらんだ部分はつられて回らなくなります。"
                "「スタートしたら」の最初に置くと、ずっとその設定になります")
        .java("{PART};");


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

    //ぐにゃぐにゃ動く
    Block("robot_sample_limpRunnning")
        .category("動き")
        .text("ぐにゃぐにゃ動く")
        .tooltip("前進、車体の回転、待機を組み合わせたサンプルです")
        .color(210)
        .java(R"({
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
        })");


    // ぐるぐる走行
    Block("robot_sample_spinRunning")
        .category("動き")
        .text("ぐるぐる走行")
        .tooltip("車体を旋回させながら前進するサンプルです")
        .color(210)
        // ぐるぐる走行（Robot でも動く形）
        .java("turnRight(30);\n"
            "ahead(60);");

    // 勝利のダンス
    Block("robot_sample_victoryDance")
        .category("動き")
        .text("勝利のダンス")
        .tooltip("右回転と左回転を繰り返して、勝利を表現します")
        .java(R"(
     for (int i = 0; i < 50; i++) {
         turnRight(30);
         turnLeft(30);
     }
     )");

    // 勝利の回転
    Block("robot_sample_victorySpin")
        .category("動き")
        .text("勝利の回転")
        .tooltip("車体を大きく回転させて、勝利を表現します")
        .java(R"(
         turnRight(36000);
     )");

}
