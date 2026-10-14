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
}
