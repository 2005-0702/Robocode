#include "RobotMove.h"

namespace {

// 距離の入力欄（前進・後進で共通）
json distanceField() {
    json field;
    field["type"] = "field_number";
    field["name"] = "DIST";   // ← generate() と check() で使う名前
    field["value"] = 100;     // 最初に入っている数字
    field["min"] = 0;         // マイナスは入力できないようにする
    return field;
}

// 距離のチェック（前進・後進で共通）
void checkDistance(const json& block, Generator& gen) {
    const double dist = gen.fieldNumber(block, "DIST", 100);
    if (dist < 0) {
        gen.error(block, "距離にマイナスは使えません");
    } else if (dist == 0) {
        gen.warning(block, "距離が0なので、ロボットは動きません");
    }
}

}  // namespace

// ---------------------------------------------------------------------
//  前に〇〇進む
// ---------------------------------------------------------------------
std::string MoveForwardBlock::type() const { return "robot_move_forward"; }

std::string MoveForwardBlock::category() const { return "動き"; }

json MoveForwardBlock::definition() const {
    json def;
    def["message0"] = "前に %1 進む";               // %1 の場所に入力欄が入る
    def["args0"] = json::array({distanceField()});
    def["tooltip"] = "ロボットを前に進めます（単位はピクセル）";
    return def;
}

void MoveForwardBlock::check(const json& block, Generator& gen) const {
    checkDistance(block, gen);
}

std::string MoveForwardBlock::generate(const json& block, Generator& gen, int indent) const {
    const double dist = gen.fieldNumber(block, "DIST", 100);
    return gen.line(indent, "ahead(" + gen.number(dist) + ");");
}

// ---------------------------------------------------------------------
//  後ろに〇〇下がる
// ---------------------------------------------------------------------
std::string MoveBackwardBlock::type() const { return "robot_move_backward"; }

std::string MoveBackwardBlock::category() const { return "動き"; }

json MoveBackwardBlock::definition() const {
    json def;
    def["message0"] = "後ろに %1 下がる";
    def["args0"] = json::array({distanceField()});
    def["tooltip"] = "ロボットを後ろに下げます（単位はピクセル）";
    return def;
}

void MoveBackwardBlock::check(const json& block, Generator& gen) const {
    checkDistance(block, gen);
}

std::string MoveBackwardBlock::generate(const json& block, Generator& gen, int indent) const {
    const double dist = gen.fieldNumber(block, "DIST", 100);
    return gen.line(indent, "back(" + gen.number(dist) + ");");
}

// ---------------------------------------------------------------------
//  登録
// ---------------------------------------------------------------------
void registerRobotMoveBlocks(BlockRegistry& registry) {
    registry.add(std::make_unique<MoveForwardBlock>());
    registry.add(std::make_unique<MoveBackwardBlock>());
}
