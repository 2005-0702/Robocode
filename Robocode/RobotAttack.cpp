#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotAttack)
{
    // パワー〇〇で撃つ  →  fire(〇〇);
    Block("robot_fire")                                   // ブロックの名前（ほかとかぶらない英語の名前）
        .category("攻撃")                                  // ツールボックスのどのカテゴリに出すか
        .text("パワー %1 で撃つ")                          // ブロックに表示する文（%1 に下の入力欄が入る）
        .number("POWER", 1, 0.1, 3)                        // インプット：名前, 最初の数字, 最小, 最大
        .tooltip("弾を撃ちます。パワーが大きいほど強いけど、自分のエネルギーも多く減ります")
        .check([](BlockData& b) {                          // 自分で書くチェック
        if (b.number("POWER") > 2) {                   //   パワーが2より大きいとき
            b.warning("パワーが大きいと、エネルギーがすぐ減ります");  // 注意を出す
        }
            })
        .java("fire({POWER});");                           // 作られるJava（{POWER} に入力した数字が入る）
    // パワー〇〇で撃つ  →  fire(〇〇);
    Block("keep_away_radar_fire")                                   // ブロックの名前（ほかとかぶらない英語の名前）
        .category("攻撃")                                  // ツールボックスのどのカテゴリに出すか
        .text("レーダーをロックした状態でパワー %1 で撃つ")                          // ブロックに表示する文（%1 に下の入力欄が入る）
        .number("POWER", 1, 0.1, 3)                        // インプット：名前, 最初の数字, 最小, 最大
        .tooltip("敵にレーダーをロックした状態で打ちます")
        .check([](BlockData& b) {                          // 自分で書くチェック
        if (b.number("POWER") > 2) {                   //   パワーが2より大きいとき
            b.warning("パワーが大きいと、エネルギーがすぐ減ります");  // 注意を出す
        }
            })
        .java("{\n"
        "    double radar = getHeading() + e.getBearing() - getRadarHeading();\n"   // レーダーを敵に向ける角度
        "    while (radar > 180) radar -= 360;\n"                                    // -180〜180度にそろえる
        "    while (radar < -180) radar += 360;\n"
        "    turnRadarRight(radar);\n"
        "    double gun = getHeading() + e.getBearing() - getGunHeading();\n"       // 砲台を敵に向ける角度
        "    while (gun > 180) gun -= 360;\n"
        "    while (gun < -180) gun += 360;\n"
        "    turnGunRight(gun);\n"
        "    fire({POWER});\n"                                                       // 撃つ
        "    scan();\n"                                                              // もう一度探す
        "}");                           // 作られるJava（{POWER} に入力した数字が入る）
    //体当たりして打つ
    Block("body_blow_fire")
        .category("攻撃")
        .text("体当たりして打つ")
        .tooltip("体当たりしながら打ちます")
        .java(R"({
        turnRight(e.getBearing());                          //敵を向く
        double gunTurn = getHeading() - getGunHeading();    
        while(gunTurn > 180) gunTurn -= 360;                //近い方向から回る
        while(gunTurn < -180) gunTurn += 360;
        turnGunRight(gunTurn);                              //砲塔を車体にそろえる
        ahead(e.getDistance() + 5);                         //敵までまっすぐ進む
        fire(3);
        scan();})")                                         //もう一度敵を探して追いかける
        .onlyIn("robot_event_enemyDetected");
    // 距離と残りエネルギーに合わせて、弾の強さを自動で決めて撃つ
    Block("smart_fire")
        .category("攻撃")
        .text("距離と残りエネルギーで強さを決めて撃つ")   // 入力欄がないので %1 はなし
        .tooltip("敵が近いほど強い弾を撃ちます。エネルギーが少ないときは、弱い弾にして節約します")
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