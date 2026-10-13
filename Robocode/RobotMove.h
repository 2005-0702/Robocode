#pragma once
// =====================================================================
//  RobotMove.h  ―― 「動き」カテゴリのブロック
//
//  新しいブロックを作るときは、このファイルと RobotMove.cpp を
// =====================================================================

#include "Block.h"

// 前に〇〇進む  →  ahead(〇〇);
class MoveForwardBlock : public StatementBlock {
public:
    std::string type() const override;
    std::string category() const override;
    json definition() const override;
    void check(const json& block, Generator& gen) const override;
    std::string generate(const json& block, Generator& gen, int indent) const override;
};

// 後ろに〇〇下がる  →  back(〇〇);
class MoveBackwardBlock : public StatementBlock {
public:
    std::string type() const override;
    std::string category() const override;
    json definition() const override;
    void check(const json& block, Generator& gen) const override;
    std::string generate(const json& block, Generator& gen, int indent) const override;
};

// このファイルのブロックをまとめて登録する（main などから1回呼ぶ）
void registerRobotMoveBlocks(BlockRegistry& registry);
