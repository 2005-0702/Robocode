#pragma once
// =====================================================================
//  RobocodeBlocks.h  ―  ブロックを作るための道具箱
// =====================================================================
//
//  ブロックを作る人は、このファイルを #include して
//  ROBOCODE_BLOCKS(名前) { ... } の中に Block(...) を並べるだけでOK。
//
//  Blockly 用の JSON づくり・Java への変換・エラーチェックなどの
//  難しい部分は、すべて RobocodeBlocksCore.cpp にまとめてあります。
//  （ブロックを作るだけなら、RobocodeBlocksCore.cpp は読まなくて大丈夫）
//
//  新しいファイルを作ったら、Visual Studio のプロジェクトに追加するのを
//  忘れずに。追加しないとビルドされず、ブロックも出てきません。
//
// ---------------------------------------------------------------------
//  ■ 見本
// ---------------------------------------------------------------------
//
//  #include "RobocodeBlocks.h"
//
//  ROBOCODE_BLOCKS(RobotAttack)              // ← ファイルごとに別の名前にする
//  {
//      // 命令ブロック（上下につながるブロック）
//      Block("robot_fire")                   // ブロックの名前（ほかとかぶらないように）
//          .category("攻撃")                  // ツールボックスのどこに出すか
//          .text("パワー %1 で撃つ")          // %1 の場所に入力欄が入る
//          .number("POWER", 1, 0.1, 3)       // 数字の入力欄：名前, 最初の数字, 最小, 最大
//          .java("fire({POWER});");          // {POWER} の場所に入力した数字が入る
//
//      // 値ブロック（ほかのブロックの穴にはめるブロック）
//      Block("robot_enemy_distance")
//          .category("センサー")
//          .text("敵までの距離")
//          .output("Number")                  // 数字を返すブロックにする
//          .onlyIn("robot_event_scanned")     // 「敵を見つけたとき」の中でしか使えない
//          .java("e.getDistance()");
//
//      // 中にブロックを入れるブロック
//      Block("robot_if")
//          .category("制御")
//          .text("もし %1 なら %2")
//          .valueInput("COND", "Boolean")     // 「はい／いいえ」のブロックをはめる穴
//          .statements("DO")                  // 中にブロックを入れる口
//          .java("if ({COND}) {\n"
//                "    {DO}\n"                 // {DO} だけの行に、中のブロックが字下げされて入る
//                "}");
//  }
//
// ---------------------------------------------------------------------
//  ■ 使える部品（Block("名前") のあとに .〇〇() でつなげて書く）
// ---------------------------------------------------------------------
//
//   見た目
//     .category("動き")            ツールボックスのカテゴリ（色も自動で決まる）
//     .text("前に %1 進む")        ブロックに表示する文。%1, %2 … に入力欄が書いた順に入る
//     .tooltip("説明")             マウスを乗せたときに出る説明
//     .color(210)                  色を自分で決めたいときだけ（0〜360）
//
//   入力欄（書いた順に %1, %2 … に入る）
//     .number("名前", 最初の数字)               数字を入力する欄
//     .number("名前", 最初の数字, 最小)         最小より小さいとエラーになる
//     .number("名前", 最初の数字, 最小, 最大)   最小〜最大の外だとエラーになる
//     .dropdown("名前", {{"右", "turnRight"}, {"左", "turnLeft"}})
//                                  選ぶ欄。{画面に出る文字, Java に入る文字} の組で書く
//     .valueInput("名前", "Number") 値ブロックをはめる穴（"Number" か "Boolean"）
//     .statements("名前")           中にブロックを入れる口
//
//   ブロックの種類（何も書かなければ「命令ブロック」になる）
//     .output("Number")            値ブロックにする（"Number" か "Boolean"）
//     .event()                     イベントブロックにする（一番上に置くブロック）
//
//   Java
//     .java("ahead({DIST});")      Java のひな形。{名前} の場所に入力欄の中身が入る
//     .javaFunction(関数)          ひな形では書けない複雑なときだけ使う（下の「上級」）
//
//   チェック
//     .warnIfZero("名前", "文")    その数字が0のとき「注意」を出す
//     .onlyIn("イベントの名前")    そのイベントブロックの中でしか使えないようにする
//     .check(関数)                 自分でチェックを書く（下の「上級」）
//
//   ※ 次のチェックは自動でやってくれるので、書かなくてOK
//     ・数字が最小〜最大の外になっている
//     ・穴にブロックが入っていない / 口の中にブロックが入っていない
//     ・イベントブロックが1つもない / 同じイベントブロックが2つある
//     ・どのイベントにもつながっていないブロックがある
//     ・text の %1, %2 … の数と入力欄の数が合っていない（設定ミス）
//     ・java の {名前} と入力欄の名前が合っていない（設定ミス）
//
// ---------------------------------------------------------------------
//  ■ 上級：関数で書く（check / javaFunction）
// ---------------------------------------------------------------------
//
//      .check([](BlockData& b) {
//          if (b.number("POWER") > 2) {
//              b.warning("パワーが大きいと、エネルギーがすぐ減ります");
//          }
//      })
//
//      .javaFunction([](BlockData& b) -> std::string {
//          if (b.text("DIR") == "RIGHT") {
//              return "turnRight(" + b.text("ANGLE") + ");";
//          }
//          return "turnLeft(" + b.text("ANGLE") + ");";
//      })
//
//   BlockData で使えるもの
//     b.number("名前")     数字の入力欄の値（double）
//     b.text("名前")       入力欄の中身を、Java に入れる文字として取り出す
//     b.error("文")        エラーを出す（Java は作れなくなる）
//     b.warning("文")      注意を出す（Java は作れる）
//
// =====================================================================

#include <functional>
#include <string>
#include <utility>
#include <vector>

// 日本語の文字を正しく扱うための確認。
// ここでエラーになったら：プロジェクトのプロパティ → C/C++ → コマンドライン →
// 「追加のオプション」に /utf-8 と書いて「OK」を押してください。
static_assert(sizeof("あ") == 4,
              "Add /utf-8 to Project Properties > C/C++ > Command Line > Additional Options");

namespace rb {

class BlockData;
using JavaFunction  = std::function<std::string(BlockData&)>;
using CheckFunction = std::function<void(BlockData&)>;
using Options       = std::vector<std::pair<std::string, std::string>>;

// ---------------------------------------------------------------------
//  ブロックの設定（Block("名前") がこれを返す）
// ---------------------------------------------------------------------
class BlockDef {
public:
    // 見た目
    BlockDef& category(const std::string& name);
    BlockDef& text(const std::string& message);
    BlockDef& tooltip(const std::string& message);
    BlockDef& color(int hue);

    // 入力欄
    BlockDef& number(const std::string& name, double initial);
    BlockDef& number(const std::string& name, double initial, double min);
    BlockDef& number(const std::string& name, double initial, double min, double max);
    BlockDef& dropdown(const std::string& name, const Options& options);
    BlockDef& valueInput(const std::string& name, const std::string& type = "Number");
    BlockDef& statements(const std::string& name);

    // ブロックの種類
    BlockDef& output(const std::string& type = "Number");
    BlockDef& event();

    // Java
    BlockDef& java(const std::string& javaTemplate);
    BlockDef& javaFunction(JavaFunction function);

    // チェック
    BlockDef& warnIfZero(const std::string& name, const std::string& message);
    BlockDef& onlyIn(const std::string& eventType);
    BlockDef& check(CheckFunction function);

    // ===== ここから下は RobocodeBlocksCore.cpp が使うもの（読まなくてOK）=====
    struct Arg {
        enum Kind { Number, Dropdown, Value, Statements };
        Kind kind = Number;
        std::string name;
        double initial = 0;
        bool hasMin = false;
        bool hasMax = false;
        double min = 0;
        double max = 0;
        Options options;
        std::string valueType;
    };
    struct Info {
        std::string type, category, text, tooltip, outputType, onlyIn, javaTemplate;
        int color = -1;
        int order = 0;
        bool isEvent = false;
        bool isOutput = false;
        std::vector<Arg> args;
        JavaFunction javaFunction;
        std::vector<CheckFunction> checks;
        const Arg* findArg(const std::string& name) const;
    };
    explicit BlockDef(const std::string& type);
    Info info;
};

// ---------------------------------------------------------------------
//  変換中のブロック（check / javaFunction の中で使う）
// ---------------------------------------------------------------------
class BlockData {
public:
    double number(const std::string& name);
    std::string text(const std::string& name);
    void error(const std::string& message);
    void warning(const std::string& message);

    // ===== ここから下は RobocodeBlocksCore.cpp が使うもの（読まなくてOK）=====
    struct Impl;
    explicit BlockData(Impl* impl) : impl_(impl) {}

private:
    Impl* impl_;
};

// 新しいブロックを作る
BlockDef& Block(const std::string& type);

// =====================================================================
//  アプリ側（リーダー・#4）が使うもの
// =====================================================================

// エラー・注意 1つ分
struct Message {
    std::string blockId;     // Blockly のブロックID（画面でブロックを選ぶのに使える）
    std::string blockLabel;  // 例：「前に 100 進む」
    std::string text;        // 例：「距離が0なので、ロボットは動きません」
};

// Java に変換した結果
struct JavaResult {
    bool ok = false;                // エラーがなければ true
    std::string java;               // できた Java のコード
    std::vector<Message> errors;    // エラー（直さないと動かない）
    std::vector<Message> warnings;  // 注意（動くけど気をつけること）
    std::string messagesText() const;  // ダイアログに出す用にまとめた文章
};

// Blockly に渡す JSON（ブロックの定義とツールボックス）
//   JS 側で setupBlocks(この JSON) を呼ぶ
std::string GetBlocklySetupJson();

// Blockly のブロック（Blockly.serialization.workspaces.save の JSON）を Java にする
JavaResult ConvertToJava(const std::string& workspaceJson, const std::string& robotName);

// =====================================================================
//  ROBOCODE_BLOCKS のしくみ（読まなくてOK）
// =====================================================================
using BlockSetFunction = void (*)();
struct AutoRegister {
    AutoRegister(const char* name, BlockSetFunction function);
};

}  // namespace rb

using rb::Block;
using rb::BlockData;

// ROBOCODE_BLOCKS(名前) { ... } と書くと、中のブロックがアプリの起動時に自動で登録される
#define ROBOCODE_BLOCKS(name)                                                   \
    static void RobocodeBlocks_##name();                                        \
    static ::rb::AutoRegister RobocodeBlocksAuto_##name(#name, RobocodeBlocks_##name); \
    static void RobocodeBlocks_##name()
