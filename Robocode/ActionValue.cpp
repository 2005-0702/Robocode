// =====================================================================
//  ActionValue.cpp  ―  「穴」に数字や計算をはめられる動き・攻撃のブロック
//
//  センサーや計算のブロックをはめて、自分で考えた角度や距離で動ける。
//  （右に回るブロックにマイナスを入れると左に回る）
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(ActionValue)
{
    // 〇 進む
    Block("act_ahead")
        .category("動き")
        .text("%1 進む")
        .valueInput("DIST", "Number")
        .tooltip("前に進みます。マイナスを入れると後ろに下がります")
        .java("ahead({DIST});");

    // 車体・砲塔・レーダーを 〇 度 右に回す
    Block("act_turn_right")
        .category("動き")
        .text("%1 を右に %2 度回す")
        .dropdown("PART", { {"車体", "turnRight"}, {"砲塔", "turnGunRight"}, {"レーダー", "turnRadarRight"} })
        .valueInput("ANGLE", "Number")
        .tooltip("右に回します。マイナスを入れると左に回ります")
        .java("{PART}({ANGLE});");

    // パワー 〇 で撃つ
    Block("act_fire")
        .category("攻撃")
        .text("%1 のパワーで撃つ")
        .valueInput("POWER", "Number")
        .tooltip("弾を撃ちます。パワーは 0.1〜3。計算した数もはめられます")
        .java("fire({POWER});");

    // 敵をもう一度探す
    Block("act_scan")
        .category("センサー")
        .text("もう一度レーダーで探す")
        .tooltip("今のレーダーの向きで、もう一度敵を探します。見つかると「敵を発見したとき」がまた動きます")
        .java("scan();");
}
