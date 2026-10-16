#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(GetGameInfomation)
{
    // 自分のロボットの名前
    Block("robot_name")
        .category("ゲーム情報")                    // カテゴリー
        .text("自分の名前")                         // ブロックに表示する文（入力欄がないので %1 はなし）
        .output("String")                          // 値ブロックにする（名前は文字なので "String"）
        .tooltip("自分のロボットの名前を調べます")   // マウスを乗せたときの説明
        .java("getName()");                        // Robocode の「名前を調べる」関数
    // 〇〇を表示する
    Block("robot_print")
        .category("ゲーム情報")
        .text("%1 を表示する")
        .valueInput("TEXT", "")                 // 種類を "" にすると、数字も文字もはまる穴になる
        .tooltip("Robocode のコンソールに表示します")
        .java("out.println({TEXT});");
    // 自分の X 座標・Y 座標を調べる
    Block("robot_get_position")
        .category("ゲーム情報")
        .text("自分の %1 座標")                         // %1 に選ぶ欄が入る
        .dropdown("AXIS", { {"X", "X"}, {"Y", "Y"} })  // 名前は AXIS。{画面に出す文字, Java に入れる文字}
        .output("Number")                               // 数字を出す値ブロック
        .tooltip("自分のロボットの X 座標（左右の位置）か Y 座標（上下の位置）を調べます")
        .java("get{AXIS}()");                           // X を選ぶと getX()、Y を選ぶと getY() になる
    //自分のエネルギー残量を取得
    Block("robot_energy")
        .category("ゲーム情報")
        .text("エネルギー残量")
        .output("Number")
        .tooltip("自分のエネルギー残量が分かります")
        .java("getEnergy()");
