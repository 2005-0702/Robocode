// =====================================================================
//  RobotColor.cpp  ―  「色」のブロック
// 
// 2026/10/14 片山　ロボットの色作成
//
// %1・・引数の第1引数
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
    //本体の色を変更
    Block("robot_color_bodyColor")
        .category("色")
        .text("本体の色を変更 R %1 G %2 B %3")
        .tooltip("ロボット本体の色を変更します(各0～255)")
        .number("R", 0, 0, 255)
        .number("G", 0, 0, 255)
        .number("B", 0, 0, 255)
        .java("setBodyColor(new Color({R}, {G}, {B}));");

    // 砲塔の色を変更
    Block("robot_color_gunColor")
        .category("色")
        .text("砲塔の色を変更 R %1 G %2 B %3")
        .tooltip("砲塔の色を変更します(各0～255)")
        .number("R", 0, 0, 255)
        .number("G", 0, 0, 255)
        .number("B", 0, 0, 255)
        .java("setGunColor(new Color({R}, {G}, {B}));");

    // レーダーの色を変更
    Block("robot_color_radarColor")
        .category("色")
        .text("レーダーの色を変更 R %1 G %2 B %3")
        .tooltip("レーダーの色を変更します(各0～255)")
        .number("R", 0, 0, 255)
        .number("G", 0, 0, 255)
        .number("B", 0, 0, 255)
        .java("setRadarColor(new Color({R}, {G}, {B}));");

    // 弾の色を変更
    Block("robot_color_bulletColor")
        .category("色")
        .text("弾の色を変更 R %1 G %2 B %3")
        .tooltip("弾の色を変更します(各0～255)")
        .number("R", 0, 0, 255)
        .number("G", 0, 0, 255)
        .number("B", 0, 0, 255)
        .java("setBulletColor(new Color({R}, {G}, {B}));");


    // 索敵時の色を変更
    Block("robot_color_scanColor")
        .category("色")
        .text("索敵の色を変更 R %1 G %2 B %3")
        .tooltip("索敵時の色を変更します(各0～255)")
        .number("R", 0, 0, 255)
        .number("G", 0, 0, 255)
        .number("B", 0, 0, 255)
        .java("setScanColor(new Color({R}, {G}, {B}));");

}
