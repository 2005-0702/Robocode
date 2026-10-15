// =====================================================================
//  RobotEvent.cpp  ―  「イベント」のブロック
// 
// 2026/10/14 片山　ロボットイベントの作成
// 2026/10/15 片山　public void の追加
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


//%1 ・・
ROBOCODE_BLOCKS(RobotEvent)
{
    //敵を発見したとき
    Block("robot_event_enemyDetected")
        .category("イベント")
        .text("敵を発見したとき %1")
        .tooltip("敵ロボットを発見したときに実行します")
        .statements("D0")
        .event()
        .java("public void onScannedRobot(ScannedRobotEvent event) {\n"
            " {D0}\n"
            "}");

    // 壁にぶつかったとき
    Block("robot_event_hitWall")
        .category("イベント")
        .text("壁にぶつかったとき %1")
        .tooltip("壁に衝突したときに実行します")
        .statements("D0")
        .event()
        .java("onHitWall(HitWallEvevt event) {\n"
            "{D0}\n"
            "}");

    // 命中時の処理
    Block("robot_event_bulletHit")
        .category("イベント")
        .text("弾が命中したとき %1")
        .tooltip("自分の弾が敵ロボットに命中したときに実行します")
        .statements("D0")
        .event()
        .java("public void onBulletHit(BulletHitEvent event) {\n"
            " {D0}\n"
            "}");

    // はずれ時 
    Block("robot_event_bulletMissed")
        .category("イベント")
        .text("弾がはずれたとき %1")
        .tooltip("自分の弾が敵に命中しなかったときに実行します")
        .statements("D0")
        .event()
        .java("onBulletMissed(BulletMissedEvent event) {\n"
            " {D0}\n"
            "}");

    // 弾同士が衝突したとき 
  /*  Block("robot_event_bulletCollision")
        .category("イベント")
        .text("弾同士が衝突したとき %1")
        .tooltip("自分の弾が敵の弾と衝突したときに実行します")
        .statements("D0")
        .event()
        .java("onBulletHitBullet(BulletHitBulletEvent event) {\n"
            " {D0}\n"
            "}");*/

            // 自機破壊時 
    Block("robot_event_destroyed")
        .category("イベント")
        .text("自分のロボットが破壊されたとき %1")
        .tooltip("自分のロボットが破壊されたときに実行します")
        .statements("D0")
        .event()
        .java("public void onDeath(DeathEvent event) {\n"
            " {D0}\n"
            "}");

    // 被弾時 
    Block("robot_event_hitByBullet")
        .category("イベント")
        .text("弾に当たったとき %1")
        .tooltip("敵の弾が自分のロボットに命中したときに実行します")
        .statements("D0")
        .event()
        .java("public void onHitByBullet(HitByBulletEvent event) {\n"
            " {D0}\n"
            "}");

    // 敵と衝突時 
    Block("robot_event_hitEnemy")
        .category("イベント")
        .text("ロボットにぶつかったとき %1")
        .tooltip("ほかのロボットと衝突したときに実行します")
        .statements("D0")
        .event()
        .java("public void onHitRobot(HitRobotEvent event) {\n"
            " {D0}\n"
            "}");

    // 敵破壊時 
    Block("robot_event_enemyDestroyed")
        .category("イベント")
        .text("敵ロボットが破壊されたとき %1")
        .tooltip("ほかのロボットが破壊されたときに実行します")
        .statements("D0")
        .event()
        .java("public void onRobotDeath(RobotDeathEvent event) {\n"
            " {D0}\n"
            "}");

    //// 勝利時
    Block("robot_event_win")
        .category("イベント")
        .text("勝利したとき %1")
        .tooltip("バトルで勝利したときに実行します")
        .statements("D0")
        .event()
        .java("public void onWin(WinEvent event) {\n"
            " {D0}\n"
            "}");
    //通常時の処理
    Block("robot_event_run")
        .category("イベント")
        .text("スタートしたら、ずっと %1")
        .statements("DO")
        .event()
        .java("public void run() {\n"
            "    while (true) {\n"
            "        {DO}\n"
            "    }\n"
            "}");

}
