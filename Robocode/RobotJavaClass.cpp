// =====================================================================
//  RobotJavaClass.cpp  ―  Java にもともとある class（Math・Double など）を使うブロック
//
//  class の名前（Math など）は選ぶ。そのあとの「. のうしろ」（sin、PI など）は自分で書く。
//    例：[Math▼] . [sin] ( 〇 )        → Math.sin(〇)
//        [Math▼] . [max] ( 〇 , 〇 )   → Math.max(〇, 〇)
//        [Math▼] . [PI]                → Math.PI
//        ( [int] ) 〇                  → ((int) 〇)
// =====================================================================
#include "RobocodeBlocks.h"

// 選べる class（import しなくても使えるもの・Robocode のもの）
static const rb::Options kJavaClasses = {
    {"Math", "Math"},
    {"Double", "Double"},
    {"Integer", "Integer"},
    {"Rules", "Rules"},   // Robocode のルール（Rules.getBulletSpeed など）
};

// 名前の欄が空ならエラーにする
static void checkName(BlockData& b, const char* field) {
    if (b.text(field).empty()) b.error("名前の欄に、使うものの名前（sin など）を書いてください");
}

ROBOCODE_BLOCKS(RobotJavaClass)
{
    // Math.PI のような、決まった値
    Block("java_class_value")
        .category("計算")
        .text("%1 . %2")
        .dropdown("CLS", kJavaClasses)
        .textField("NAME", "PI")
        .output("")
        .tooltip("Java にもともとある値です（例：Math . PI → 円周率、Double . MAX_VALUE → 一番大きい数）")
        .check([](BlockData& b) { checkName(b, "NAME"); })
        .java("{CLS}.{NAME}");

    // Math.sin(〇) のような、1つの値を渡すもの
    Block("java_class_call1")
        .category("計算")
        .text("%1 . %2 ( %3 )")
        .dropdown("CLS", kJavaClasses)
        .textField("NAME", "sin")
        .valueInput("A", "")
        .output("")
        .tooltip("Java にもともとある計算です（例：Math . sin ( 〇 )、Math . sqrt ( 〇 )）。"
                 "Math の sin・cos などの角度はラジアンです")
        .check([](BlockData& b) { checkName(b, "NAME"); })
        .java("{CLS}.{NAME}({A})");

    // Math.max(〇, 〇) のような、2つの値を渡すもの
    Block("java_class_call2")
        .category("計算")
        .text("%1 . %2 ( %3 , %4 )")
        .dropdown("CLS", kJavaClasses)
        .textField("NAME", "atan2")
        .valueInput("A", "")
        .valueInput("B", "")
        .output("")
        .tooltip("Java にもともとある計算です（例：Math . atan2 ( 〇 , 〇 )、Math . pow ( 〇 , 〇 )）")
        .check([](BlockData& b) { checkName(b, "NAME"); })
        .java("{CLS}.{NAME}({A}, {B})");

    // (int) 〇 のような、型を変えるもの（キャスト）
    Block("java_cast")
        .category("計算")
        .text("( %1 ) %2")
        .textField("TYPE", "int")
        .valueInput("A", "")
        .output("")
        .tooltip("値の型を変えます（キャスト）。例：( int ) 3.7 → 3（小数点以下を切りすてる）")
        .check([](BlockData& b) { checkName(b, "TYPE"); })
        .java("(({TYPE}) {A})");
}
