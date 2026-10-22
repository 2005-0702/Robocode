#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotAttack)
{
    // パワー〇〇で撃つ  →  fire(〇〇);
    Block("robot_fire")                                   // ブロックの名前（ほかとかぶらない英語の名前）
        .category("攻撃")                                  // ツールボックスのどのカテゴリに出すか
        .text("パワー %1 で撃つ")                          // ブロックに表示する文（%1 に下の入力欄が入る）
        .valueInput("POWER", "Number")                        // インプット：名前, 最初の数字, 最小, 最大
        .tooltip("弾を撃ちます。パワーが大きいほど強いけど、自分のエネルギーも多く減ります")
        .check([](BlockData& b) {     
            std::string t = b.text("POWER");
            char* end = nullptr;
            double v = std::strtod(t.c_str(), &end);
            if (end != t.c_str() && *end == '\0' && (v < 0.1 || v > 3))
                b.error("パワーは0.1～3にしてください");
        
        })
        .java("fire({POWER});");                           // 作られるJava（{POWER} に入力した数字が入る）
 
}