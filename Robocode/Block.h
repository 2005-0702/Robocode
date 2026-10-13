#pragma once
// =====================================================================
//  Block.h  ―― 全員が使う「ブロックの土台」
//
//  ブロック1種類 = Block を継承したクラス1つ。
//  1つのクラスに、次の3つをまとめて書きます。
//
//    1. definition() : Blockly に渡すブロックの形（文言・入力欄・説明）
//    2. check()      : 間違いのチェック（エラー・警告）※必要なときだけ
//    3. generate()   : Java のコードを作る
//
//  色と「上下につながる」設定は土台が自動で付けるので、書かなくてOK。
//  作ったクラスは BlockRegistry に登録すると使えるようになります。
//  （例は RobotMove.h / RobotMove.cpp を見てください）
// =====================================================================

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "json.hpp"   // nlohmann/json（1ファイルのライブラリ）

using json = nlohmann::json;

class Generator;

// ---------------------------------------------------------------------
//  ブロックの基本クラス
// ---------------------------------------------------------------------
class Block {
public:
    virtual ~Block() = default;

    // ブロックの名前。英数字と _ だけで、ほかのブロックと重ならないようにする
    //   例: "robot_move_forward"
    virtual std::string type() const = 0;

    // ツールボックスのカテゴリ名（「動き」「砲台」など）
    // 新しいカテゴリの色は Block.cpp の categoryColour() に足す
    virtual std::string category() const = 0;

    // ブロックの形（message0, args0, tooltip など）
    virtual json definition() const = 0;

    // 間違いのチェック。何もなければ書かなくてよい
    //   gen.error(block, "...")   … 実行を止める間違い
    //   gen.warning(block, "...") … 動くけれど注意したいこと
    virtual void check(const json& block, Generator& gen) const;

    // Java のコードを作る。indent はインデントの段数
    virtual std::string generate(const json& block, Generator& gen, int indent) const = 0;

    // type・色・つながりを足した、Blockly に渡す完成版の定義
    json fullDefinition() const;

protected:
    // ブロックの種類ごとの「つながり」を定義に足す（下の StatementBlock を参照）
    virtual void addConnections(json& def) const;
};

// ---------------------------------------------------------------------
//  上下につながる命令ブロック（前に進む、撃つ など）
//  命令ブロックを作るときは Block ではなくこちらを継承する
// ---------------------------------------------------------------------
class StatementBlock : public Block {
protected:
    void addConnections(json& def) const override;
};

// ---------------------------------------------------------------------
//  チェック結果（ブロックID付きなので、Blockly 側で該当ブロックに警告を出せる）
// ---------------------------------------------------------------------
enum class Severity { Error, Warning };

struct CheckMessage {
    std::string blockId;
    Severity severity;
    std::string text;

    json toJson() const;   // {"blockId": "...", "severity": "error", "text": "..."}
};

// ---------------------------------------------------------------------
//  ブロックの登録簿
// ---------------------------------------------------------------------
class BlockRegistry {
public:
    // ブロックを登録する（同じ type が2つあるとエラー）
    void add(std::unique_ptr<Block> block);

    // type からブロックを探す（なければ nullptr）
    const Block* find(const std::string& type) const;

    // Blockly.defineBlocksWithJsonArray() に渡す配列
    json definitions() const;

    // Blockly のツールボックス（登録した順にカテゴリ分け）
    json toolbox() const;

private:
    std::vector<std::unique_ptr<Block>> blocks_;      // 登録した順
    std::map<std::string, const Block*> byType_;
};

// ---------------------------------------------------------------------
//  Java を作る係。ブロックの generate() / check() から使う便利関数もここ
// ---------------------------------------------------------------------
class Generator {
public:
    explicit Generator(const BlockRegistry& registry);

    // next でつながったブロックを上から順に Java にする（チェックも同時に行う）
    std::string statements(const json& firstBlock, int indent);

    // ブロックの中の「命令を入れる穴」（くりかえしの中身など）を Java にする
    // 穴が空なら "" を返す
    std::string statementInput(const json& block, const std::string& inputName, int indent);

    // インデント付きの1行を作る
    std::string line(int indent, const std::string& code) const;

    // 数字の入力欄（field_number）の値を読む。なければ fallback
    double fieldNumber(const json& block, const std::string& name, double fallback) const;

    // 数字を Java で使える文字にする（100 → "100"、12.5 → "12.5"）
    std::string number(double value) const;

    // チェック結果を記録する
    void error(const json& block, const std::string& text);
    void warning(const json& block, const std::string& text);

    const std::vector<CheckMessage>& messages() const { return messages_; }
    bool hasError() const;

private:
    const BlockRegistry& registry_;
    std::vector<CheckMessage> messages_;
};
