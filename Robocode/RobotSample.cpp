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
    // 壁から離れる
    Block("robot_sample_getAwayFromWall")
        .category("サンプル戦術")
        .text("壁から離れる")
        .tooltip("壁にぶつかったとき、壁との位置関係に応じて移動します")
        .color(330)
        .java(R"(
    double bearing = e.getBearing();

    if (bearing > -90 && bearing < 90) {
        setBack(200);
    } else {
        setAhead(200);
    }
    )")
        .onlyIn("robot_event_hitWall");
   
    //  弾の方向を向いてスキャン
    Block("robot_sample_turnFlewBullet")
        .category("サンプル戦術")
        .text("弾の方向を向いてスキャン")
        .tooltip("弾が当たった方向をもとに向きを調整し、敵をスキャンします")
        .color(330)
        .java(R"(
    turnRight(e.getHeading() - getHeading() + 180);
    turnGunRight(getHeading() - getGunHeading());
    scan();
    )")
        .onlyIn("robot_event_hitByBullet");

    // 次の弾を避ける
    Block("robot_sample_hesitateNextBullet")
        .category("サンプル戦術")
        .text("次の弾を避ける")
        .tooltip("飛んできた弾に対して垂直方向に向きを変えて前進します")
        .color(330)
        .java(R"(
    turnRight(normalRelativeAngleDegrees(90 - (getHeading() - e.getHeading())));
    ahead(100);
    )")
        .onlyIn("robot_event_hitByBullet");


    // 相手に車体を向けてスキャン
    Block("robot_sample_turnBodyToEnemy")
        .category("サンプル戦術")
        .text("相手に車体を向ける")
        .tooltip("相手にぶつかったとき、車体の向きを調整してスキャンします")
        .color(330)
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
        .tooltip("相手にぶつかったとき、砲塔を相手の方向に向けてスキャンします")
        .color(330)
        .java(R"(
    double gunTurnAmt = normalRelativeAngleDegrees(e.getBearing() + getHeading() - getGunHeading());
    
    turnGunRight(gunTurnAmt);
    scan();
    )");


}