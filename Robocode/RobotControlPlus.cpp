// =====================================================================
//  RobotControlPlus.cpp  ―  組み合わせて使う「制御」のブロック
//
//  「記憶」のブロックを使うには、RobocodeBlocksCore.cpp で
//  ロボットのクラスに memA, memB, memC を用意しておく必要がある（説明を参照）。
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotControlPlus)
{
    // 〇 のあいだくり返す
    Block("control_while")
        .category("制御")
        .text("%1 のあいだくり返す %2")
        .valueInput("COND", "Boolean")
        .statements("DO")
        .tooltip("条件が「はい」のあいだ、中のブロックをくり返します。中に動くブロックを入れないと止まってしまいます")
        .java("while ({COND}) {\n"
              "    {DO}\n"
              "}");

    // 〇 ではない
    Block("control_not")
        .category("制御")
        .text("%1 ではない")
        .valueInput("COND", "Boolean")
        .output("Boolean")
        .tooltip("「はい」と「いいえ」を逆にします")
        .java("(!{COND})");

    // 〇 % の確率で
    Block("control_chance")
        .category("制御")
        .text("%1 ％の確率で")
        .number("PERCENT", 50, 0, 100)
        .output("Boolean")
        .tooltip("決めた確率で「はい」になります。毎回ちがう動きをさせたいときに使います")
        .java("(Math.random() * 100 < {PERCENT})");

    // ---------------- 記憶（変数） ----------------
    //   memA, memB, memC はロボットのクラスに用意しておく（ラウンドが変わると 0 にもどる）

    // 記憶 A を 〇 にする
    Block("memory_set")
        .category("制御")
        .text("記憶 %1 を %2 にする")
        .dropdown("MEM", { {"A", "memA"}, {"B", "memB"}, {"C", "memC"} })
        .valueInput("VALUE", "Number")
        .tooltip("数字を覚えておきます。ほかのイベントからも使えます（ラウンドが変わると 0 にもどります）")
        .java("{MEM} = {VALUE};");

    // 記憶 A に 〇 を足す
    Block("memory_add")
        .category("制御")
        .text("記憶 %1 に %2 を足す")
        .dropdown("MEM", { {"A", "memA"}, {"B", "memB"}, {"C", "memC"} })
        .valueInput("VALUE", "Number")
        .tooltip("覚えている数字に足します。マイナスの数を入れると引き算になります")
        .java("{MEM} += {VALUE};");

    // 記憶 A
    Block("memory_get")
        .category("制御")
        .text("記憶 %1")
        .dropdown("MEM", { {"A", "memA"}, {"B", "memB"}, {"C", "memC"} })
        .output("Number")
        .tooltip("覚えている数字です（何も覚えさせていないと 0）")
        .java("{MEM}");
}
