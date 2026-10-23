// =====================================================================
//  RoboteEvent.cpp  ―  「イベント」のブロック
// 
// 2026/10/14 片山　ロボットイベントの作成
// 2026/10/15 片山　public void の追加
// 2026/10/22 片山  イベントの順番をやりやすくする
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
    //①通常時の処理
    Block("robot_event_run")
        .category("イベント")
        .text("スタートしたら、ずっと %1")
        .tooltip("ゲームがスタートしてからずっと呼び出されます")
        .statements("DO")
        .event()
        .java("public void run() {\n"
            "    while (true) {\n"
            "        {DO}\n"
            "    }\n"
            "}");

    
    //②敵を発見したとき(敵の名前、距離、向き、エネルギーなどの情報を使える)
    Block("robot_event_enemyDetected")
        .category("イベント")
        .text("敵を発見したとき %1")
        .tooltip("敵ロボットを発見したときに実行します")
        .statements("D0")
        .event()
        .java("public void onScannedRobot(ScannedRobotEvent e) {\n"
            " {D0}\n"
            "}");

    //③敵に弾が命中時の処理
    Block("robot_event_bulletHit")
        .category("イベント")
        .text("弾が敵に命中したとき %1")
        .tooltip("自分の弾が敵ロボットに命中したときに実行します")
        .statements("D0")
        .event()
        .java("public void onBulletHit(BulletHitEvent e) {\n"
            " {D0}\n"
            "}");

    //④弾がはずれ時 
    Block("robot_event_bulletMissed")
        .category("イベント")
        .text("弾がはずれたとき %1")
        .tooltip("自分の弾が何にも命中しなかったときに\n"
                 "実行します")
        .statements("D0")
        .event()
        .java("public void onBulletMissed(BulletMissedEvent e) {\n"
            " {D0}\n"
            "}");

    //⑤敵の弾に当たったとき 
    Block("robot_event_hitByBullet")
        .category("イベント")
        .text("敵の弾に当たったとき %1")
        .tooltip("敵ロボットの弾が自分のロボットに\n"
                 "命中したときに実行します")
        .statements("D0")
        .event()
        .java("public void onHitByBullet(HitByBulletEvent e) {\n"
            " {D0}\n"
            "}");

    // ⑥弾同士が当たったとき
    Block("robot_event_bulletHitBullet")
        .category("イベント")
        .text("弾同士がぶつかったとき %1")
        .tooltip("自分のロボットの弾と敵ロボットの弾が\n"
                 "ぶつかったときに実行します")
        .statements("D0")
        .event()
        .java("public void onBulletHitBullet(BulletHitBulletEvent e) {\n"
            "    {D0}\n"
            "}");

    //⑦敵と衝突時 
    Block("robot_event_hitEnemy")
        .category("イベント")
        .text("ロボットにぶつかったとき %1")
        .tooltip("自分のロボットが敵ロボットと衝突したときに\n"
                 "実行します")
        .statements("D0")
        .event()
        .java("public void onHitRobot(HitRobotEvent e) {\n"
            " {D0}\n"
            "}");

    //⑧壁にぶつかったとき
    Block("robot_event_hitWall")
        .category("イベント")
        .text("壁にぶつかったとき %1")
        .tooltip("壁に衝突したときに実行します")
        .statements("D0")
        .event()
        .java("public void onHitWall(HitWallEvent  e) {\n"
            "{D0}\n"
            "}");

    //⑨敵破壊時 
    Block("robot_event_enemyDestroyed")
        .category("イベント")
        .text("敵ロボットが破壊されたとき %1")
        .tooltip("敵ロボットが破壊されたときに実行します")
        .statements("D0")
        .event()
        .java("public void onRobotDeath(RobotDeathEvent e) {\n"
            " {D0}\n"
            "}");

    //⑩自機破壊時 
    Block("robot_event_destroyed")
        .category("イベント")
        .text("自分のロボットが破壊されたとき %1")
        .tooltip("自分のロボットが破壊されたときに実行します")
        .statements("D0")
        .event()
        .java("public void onDeath(DeathEvent e) {\n"
            " {D0}\n"
            "}");

    //⑪勝利時
    Block("robot_event_win")
        .category("イベント")
        .text("勝利したとき %1")
        .tooltip("バトルで勝利したときに実行します")
        .statements("D0")
        .event()
        .java("public void onWin(WinEvent e) {\n"
            " {D0}\n"
            "}");

  
    ///その他の応用イベント
    /*
       onStatus	現在のロボットの状態が通知された	エネルギーや位置を確認する
       onRoundEnded	1ラウンドが終了した	ラウンドごとの結果を記録する
       onBattleEnde バトル全体が終了した	試合全体の結果を確認する
       onSkippedTurn 処理が間に合わず、自分の行動ターンが飛ばされた	処理が重い原因を調べる
     */

    /*
    イベント名	中学生向けの説明
        onCustomEvent	自分で決めた条件が成立したとき
        onMessageReceived	チームの仲間などからメッセージを受け取ったとき
        onPaint	ロボットの画面を描画するとき
        onRoundStarted	新しいラウンドが始まったとき*/
   
}
