#include "Block.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------------
//  カテゴリの色（Blockly の色相 0〜360）
//  新しいカテゴリを作ったら、ここに1行足す
// ---------------------------------------------------------------------
static int categoryColour(const std::string& category) {
    static const std::map<std::string, int> colours = {
        {"動き",     210},
        {"砲台",       0},
        {"レーダー", 120},
        {"イベント",  45},
        {"値",       290},
        {"色",        20},  
    };
    auto it = colours.find(category);
    return it != colours.end() ? it->second : 160;   // 未登録のカテゴリは緑っぽい色
}

// ---------------------------------------------------------------------
//  Block
// ---------------------------------------------------------------------
void Block::check(const json&, Generator&) const {
    // 何もチェックしないブロック用（必要なブロックだけ上書きする）
}

void Block::addConnections(json&) const {
    // つながりなし（種類ごとのクラスで上書きする）
}

json Block::fullDefinition() const {
    json def = definition();
    def["type"] = type();
    def["colour"] = categoryColour(category());
    addConnections(def);
    return def;
}

void StatementBlock::addConnections(json& def) const {
    def["previousStatement"] = nullptr;   // 上につながる
    def["nextStatement"] = nullptr;       // 下につながる
}

// ---------------------------------------------------------------------
//  CheckMessage
// ---------------------------------------------------------------------
json CheckMessage::toJson() const {
    return json{
        {"blockId", blockId},
        {"severity", severity == Severity::Error ? "error" : "warning"},
        {"text", text},
    };
}

// ---------------------------------------------------------------------
//  BlockRegistry
// ---------------------------------------------------------------------
void BlockRegistry::add(std::unique_ptr<Block> block) {
    const std::string t = block->type();
    if (byType_.count(t) > 0) {
        throw std::runtime_error("同じ type のブロックが2つ登録されています: " + t);
    }
    byType_[t] = block.get();
    blocks_.push_back(std::move(block));
}

const Block* BlockRegistry::find(const std::string& type) const {
    auto it = byType_.find(type);
    return it != byType_.end() ? it->second : nullptr;
}

json BlockRegistry::definitions() const {
    json list = json::array();
    for (const auto& b : blocks_) {
        list.push_back(b->fullDefinition());
    }
    return list;
}

json BlockRegistry::toolbox() const {
    // カテゴリを、最初に出てきた順に並べる
    std::vector<std::string> order;
    std::map<std::string, json> contents;
    for (const auto& b : blocks_) {
        const std::string c = b->category();
        if (contents.count(c) == 0) {
            order.push_back(c);
            contents[c] = json::array();
        }
        contents[c].push_back(json{{"kind", "block"}, {"type", b->type()}});
    }

    json categories = json::array();
    for (const auto& c : order) {
        categories.push_back(json{
            {"kind", "category"},
            {"name", c},
            {"colour", std::to_string(categoryColour(c))},
            {"contents", contents[c]},
        });
    }
    return json{{"kind", "categoryToolbox"}, {"contents", categories}};
}

// ---------------------------------------------------------------------
//  Generator
// ---------------------------------------------------------------------
Generator::Generator(const BlockRegistry& registry) : registry_(registry) {}

std::string Generator::statements(const json& firstBlock, int indent) {
    std::string code;
    const json* current = &firstBlock;

    while (current != nullptr && current->is_object()) {
        const std::string type = current->value("type", "");
        const Block* block = registry_.find(type);

        if (block == nullptr) {
            error(*current, "このブロックはまだJavaに変換できません（" + type + "）");
        } else {
            block->check(*current, *this);
            code += block->generate(*current, *this, indent);
        }

        // 下につながっているブロックへ進む
        if (current->contains("next") && (*current)["next"].contains("block")) {
            current = &(*current)["next"]["block"];
        } else {
            current = nullptr;
        }
    }
    return code;
}

std::string Generator::statementInput(const json& block, const std::string& inputName, int indent) {
    if (block.contains("inputs") && block["inputs"].contains(inputName) &&
        block["inputs"][inputName].contains("block")) {
        return statements(block["inputs"][inputName]["block"], indent);
    }
    return "";
}

std::string Generator::line(int indent, const std::string& code) const {
    return std::string(indent * 4, ' ') + code + "\n";
}

double Generator::fieldNumber(const json& block, const std::string& name, double fallback) const {
    if (!block.contains("fields") || !block["fields"].contains(name)) {
        return fallback;
    }
    const json& v = block["fields"][name];
    if (v.is_number()) {
        return v.get<double>();
    }
    if (v.is_string()) {
        try {
            return std::stod(v.get<std::string>());
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

std::string Generator::number(double value) const {
    // 整数なら小数点なし、そうでなければそのまま
    if (std::floor(value) == value && std::fabs(value) < 1e15) {
        return std::to_string(static_cast<long long>(value));
    }
    std::ostringstream os;
    os << std::setprecision(12) << value;
    return os.str();
}

void Generator::error(const json& block, const std::string& text) {
    messages_.push_back(CheckMessage{block.value("id", ""), Severity::Error, text});
}

void Generator::warning(const json& block, const std::string& text) {
    messages_.push_back(CheckMessage{block.value("id", ""), Severity::Warning, text});
}

bool Generator::hasError() const {
    for (const auto& m : messages_) {
        if (m.severity == Severity::Error) return true;
    }
    return false;
}
