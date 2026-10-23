// =====================================================================
//  RobotAttack.cpp  ―  「攻撃」のブロック
// 
//  2026/10/23 片山 プログラムの整理
// =====================================================================
#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotAttack)
{
    // パワー〇〇で撃つ  
    Block("robot_fire")                                   
        .category("攻撃")                                 
        .text("パワー %1 で撃つ")                         
        .valueInput("POWER", "Number")                    
        .tooltip("弾を撃ちます。パワーが大きいほど、\n"
            "相手に与えるダメージが大きくなるります\n"
            "※大きいと自分のエネルギーも多く消費します")
        .check([](BlockData& b) {     
            std::string t = b.text("POWER");
            char* end = nullptr;
            double v = std::strtod(t.c_str(), &end);
            if (end != t.c_str() && *end == '\0' && (v < 0.1 || v > 3))
                b.error("パワーは0.1～3にしてください");
        })
        .java("fire({POWER});");                           // 作られるJava（{POWER} に入力した数字が入る）
}