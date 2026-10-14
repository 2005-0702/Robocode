#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotControl)
{
    // もし〇〇なら
    Block("robot_if")
        .category("制御")
        .text("もし %1 なら %2 でなければ %3" )
        .valueInput("COND", "Boolean")          // 「はい／いいえ」のブロックをはめる穴
        .statements("DO")                       // 中にブロックを入れる口
        .statements("ELSE")                       
        .tooltip("条件に合うときだけ、中のブロックを実行します")
        .java("if ({COND}) {\n"
            "    {DO}\n"
            "}else {\n"
          "    {ELSE}\n"
            "}");

    // 〇〇 < 〇〇（数字を比べる）
    Block("robot_compare")
        .category("制御")
        .text("%1 %2 %3")
        .valueInput("A", "Number")              // 数字のブロックをはめる穴（左）
        .dropdown("OP", { {"<", "<"}, {">", ">"}, {"=", "=="},{"≦","<="},{"≧",">="},{"≠","!="},{"and","&&"} 
        , { "or","||" } })
        .valueInput("B", "Number")              // 数字のブロックをはめる穴（右）
        .output("Boolean")                      // 比べた結果は「はい／いいえ」
        .tooltip("2つの数字を比べます")
        .java("({A} {OP} {B})");

    
}