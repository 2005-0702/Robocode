// =====================================================================
//  RobotSample.cpp  ―  「完成済みの動きのサンプル」のブロック
// 
// 2026/10/15 片山　ロボットのサンプル作成
//
// =====================================================================

/* ======== BlockDef ― ブロック設定関数一覧 =============
*
*【1. 見た目の設定】
*category(name)     : カテゴリを設定                    例: .category("動き")
*text(message)      : ブロックに表示する文字            例: .text("前に %1 進む")
*tooltip(message)   : マウスを乗せたときの説明          例: .tooltip("前に進みます")
*color(hue)         : ブロックの色を設定（色相 0～360） 例: .color(210)
*
*【2. 入力欄・接続口】
*number(name, value)            : 数値入力欄を追加                例: .number("DIST", 100)
*number(name, value, min)       : 最小値付きの数値入力欄          例: .number("DIST", 100, 0)
*number(name, value, min, max)  : 最小値・最大値付きの数値入力欄  例: .number("ANGLE", 90, 0, 360)
*dropdown(name, opts)           : 選択肢を追加                    例: .dropdown("DIR", options)
*valueInput(name)               : 値を受け取る接続口を追加        例: .valueInput("ANGLE")
*statements(name)               : 命令を入れる接続口を追加        例: .statements("DO")
*
*【3. ブロックの種類】
*output(type)   : 値を出力するブロックに設定     例: .output("Number")
*event()        : イベントブロックとしてマーク   例: .event()
*
*【4. Javaコード生成】
*java(template)     : Javaコードのテンプレートを設定 例: .java("ahead({DIST});")
*javaFunction(func) : 関数を使ってJavaコードを生成   例: .javaFunction(myFunction)
*
*【5. チェック・制限】
*warnIfZero(name, msg)  : 値が0の場合に警告         例: .warnIfZero("DIST", "距離が0です")
*onlyIn(eventType)      : 使用できるイベントを指定  例: .onlyIn("roboteventscanned")
*check(func)            : 独自のチェック処理を追加  例: .check(myCheckFunction)
*/

#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotEvent)
{
    /*カテゴリー別色一覧
    *{"イベント", 30},
    *{"動き", 210},
    *{"攻撃", 0},
    *{"センサー", 160},
    *{"制御", 120},
    *{"計算", 230},
    *{"ゲーム情報",180},
    *{"色",270},
    *{"サンプル戦術",330}
    *{"サンプル戦術2",330}
    */
   
    //============//
    // 動き
    //============// 
    // 前進・後退しながら砲塔を回す
    Block("robot_sample_aheadAndBack")
        .category("サンプル戦術")
        .text("前進・後退しながら砲塔を回す")
        .tooltip("前進、砲塔の回転、後退を組み合わせたサンプルです")
        .color(210)
        .java(
            "ahead(100);\n"
            "turnGunRight(360);\n"
            "back(100);\n"
            "turnGunRight(360);\n"
        );

    //ぐにゃぐにゃ動く
    Block("robot_sample_limpRunnning")
        .category("サンプル戦術")
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
            "waitFor(new TurnCompleteCondition(this));\n"
        );


    //// ぐるぐる走行
    //Block("robot_sample_spinRunning")
    //    .category("サンプル戦術")
    //    .text("ぐるぐる走行")
    //    .tooltip("車体を旋回させながら前進するサンプルです")
    //    .color(210)
    //    .java(
    //        "setTurnRight(10000);\n"
    //        "setMaxVelocity(5);\n"
    //        "ahead(1000);\n"
    //    );

    //// 勝利のダンス
    //Block("robot_sample_victoryDance")
    //    .category("サンプル戦術")
    //    .text("勝利のダンス")
    //    .tooltip("左右に旋回する動作を50回繰り返します")
    //    .color(210)
    //    .java(
    //        "for (int i = 0; i < 50; i++) {\n"
    //        "    turnRight(30);\n"
    //        "    turnLeft(30);\n"
    //        "}"
    //    );
}