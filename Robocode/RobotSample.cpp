// =====================================================================
//  RobotSample.cpp  ―  「サンプル戦術」のブロック
// 
// 2026/10/15 片山　ロボットのサンプル作成
// 2026/10/28 片山　サンプル戦術の整理
//
// =====================================================================

/* ======== BlockDef ― ブロック設定関数一覧 =============
*
*【1. 見た目の設定】
*category(name)     : カテゴリを設定                    例: .category("動き")
*text(message)      : ブロックに表示する文字            例: .text("前に %1 進む")
*tooltip(message)   : マウスを乗せたときの説明          例: .tooltip("前に進みます")
*color(hue)         : Arg::Statements, value, min, max)  : 最小値・最大値付きの数値入力欄  例: .number("ANGLE", 90, 0, 360)
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

ROBOCODE_BLOCKS(RobotSample)
{  
   
    // 前進・後退しながら砲塔を回す
    Block("robot_sample_aheadAndBack")
        .category("サンプル戦術")
        .text("前進・後退しながら砲塔を回す")
        .tooltip("前進、砲塔の回転、後退を組み合わせたサンプルです")
        .java(
            "ahead(100);\n"
            "turnGunRight(360);\n"
            "back(100);\n"
            "turnGunRight(360);"
        );

    //曲がりながら動く
    Block("robot_sample_limpRunnning")
        .category("サンプル戦術")
        .text("曲がりながら動く")
        .tooltip("前進、車体の回転、待機を組み合わせたサンプルです")
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


    // 回転してから前進
    Block("robot_sample_spinRunning")
        .category("サンプル戦術")
        .text("回転してから前進")
        .tooltip("車体を旋回させながら前進するサンプルです")
        .java("turnRight(30);\n"
            "ahead(60);\n");

    //  弾の方向を向いてスキャン
    Block("robot_sample_turnFlewBullet")
        .category("サンプル戦術")
        .text("弾の方向を向いてスキャン")
        .tooltip("弾が当たった方向をもとに向きを調整し、\n"
            "敵ロボットをスキャンするサンプルです")
        .java(R"(
    turnRight(e.getHeading() - getHeading() + 180);
    turnGunRight(getHeading() - getGunHeading());
    scan();
    )")
        .onlyIn("robot_event_hitByBullet");

    // 相手に車体を向けてスキャン
    Block("robot_sample_turnBodyToEnemy")
        .category("サンプル戦術")
        .text("相手に車体を向ける")
        .tooltip("敵ロボットにぶつかったとき、\n"
            "車体の向きを調整してスキャンするサンプルです")
        .java(R"(
    double bodyTurnAmt = normalRelativeAngleDegrees(e.getBearing() + getHeading() - getGunHeading());
    
    turnRight(bodyTurnAmt);
    
    if (getHeading() - getGunHeading() != 0) {
        turnGunRight(getHeading() - getGunHeading());
    }
    
    scan();
    )");

    // 相手に砲塔を向けてスキャン
    Block("robot_sample_turnGunToEnemy")
        .category("サンプル戦術")
        .text("相手に砲塔を向ける")
        .tooltip("敵ロボットにぶつかったとき、\n"
                 "砲塔を相手の方向に向けてスキャンするサンプルです")
        .java(R"(
    double gunTurnAmt = normalRelativeAngleDegrees(e.getBearing() + getHeading() - getGunHeading());
    
    turnGunRight(gunTurnAmt);
    scan();
    )");

    // 距離と残りエネルギーに合わせて、弾の強さを自動で決めて撃つ
    Block("smart_fire")
        .category("サンプル戦術")
        .text("距離と残りエネルギーで強さを決めて撃つ")   // 入力欄がないので %1 はなし
        .tooltip("敵ロボットが近いほど強い弾を撃ちます\n"
                "エネルギーが少ないときは、弱い弾にして節約するサンプルです")
        .java(R"({
        double power;
        if (e.getDistance() < 150) {                 // 近い → 当たりやすいので強く
            power = 3;
        } else if (e.getDistance() < 400) {          // 中くらい
            power = 2;
        } else {                                     // 遠い → 外れやすいので弱く
            power = 1;
        }
        if (getEnergy() < 20) {                      // エネルギーが少ないときは節約する
            power = Math.min(power, 1);
        }
        if (getEnergy() < 5) {                       // ほとんど残っていないときは、もっと弱く
            power = 0.1;
        }
            fire(power);
        })")
        .onlyIn("robot_event_enemyDetected");        // 「敵を見つけたとき」のイベントの名前に合わせる

}