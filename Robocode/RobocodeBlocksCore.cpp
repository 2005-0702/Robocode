// =====================================================================
//  RobocodeBlocksCore.cpp  ―  ブロックのしくみ（難しい部分）
// 
// 2026/10/16 片山　カテゴリー色に色とサンプル戦術追加
// =====================================================================
//
//  ブロックを作る人は読まなくてOK。ここでやっていること：
//    1. ROBOCODE_BLOCKS の中で書かれたブロックを集める
//    2. Blockly 用の JSON（ブロックの定義とツールボックス）を作る
//    3. Blockly のブロック（JSON）を Robocode の Java に変換する
//    4. エラー・注意をまとめる
//
// =====================================================================
#include "RobocodeBlocks.h"

#include <nlohmann/json.hpp>  // json.hpp の場所が違うときは、この1行だけ直す

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <sstream>

using json = nlohmann::json;

namespace rb {
    namespace {

        // ---------------------------------------------------------------------
        //  チームで決める設定
        // ---------------------------------------------------------------------
        const char* const kJavaPackage = "student";   // Java の package 名（robots\student に保存する）
        const char* const kNumberBlock = "rb_number";  // 数字ブロック（穴に最初から入っている）

        // ツールボックスに並ぶ順番と色（ここにないカテゴリは後ろに並ぶ）
        struct CategoryColor {
            const char* name;
            int color;
        };
        const CategoryColor kCategories[] = {
            {"イベント", 30},
            {"動き", 210},
            {"攻撃", 0},
            {"センサー", 160},
            {"制御", 120},
            {"計算", 230},
            {"ゲーム情報",180},
            {"ゲーム情報2",180},
            {"色",270},
            {"サンプル戦術",330}
        };
        const int kOtherColor = 290;

        // ---------------------------------------------------------------------
        //  登録されたブロック
        // ---------------------------------------------------------------------
        struct Registry {
            std::vector<std::pair<std::string, BlockSetFunction>> sets;
            std::vector<std::unique_ptr<BlockDef>> blocks;
            std::map<std::string, const BlockDef::Info*> byType;
            std::vector<std::string> problems;  // ブロックの設定ミス
            bool loaded = false;
        };

        Registry& registry() {
            static Registry r;
            return r;
        }

        // ---------------------------------------------------------------------
        //  ちょっとした道具
        // ---------------------------------------------------------------------
        std::string formatNumber(double v) {
            if (!std::isfinite(v)) return "0";
            if (v == std::floor(v) && std::fabs(v) < 1e15) {
                return std::to_string(static_cast<long long>(v));
            }
            std::ostringstream os;
            os.imbue(std::locale::classic());
            os << std::setprecision(12) << v;
            return os.str();
        }

        std::string trim(const std::string& s) {
            const char* ws = " \t\r\n";
            size_t b = s.find_first_not_of(ws);
            if (b == std::string::npos) return "";
            size_t e = s.find_last_not_of(ws);
            return s.substr(b, e - b + 1);
        }

        std::vector<std::string> splitLines(const std::string& s) {
            std::vector<std::string> lines;
            if (s.empty()) return lines;
            std::string line;
            for (char c : s) {
                if (c == '\n') {
                    lines.push_back(line);
                    line.clear();
                }
                else if (c != '\r') {
                    line += c;
                }
            }
            lines.push_back(line);
            return lines;
        }

        std::string joinLines(const std::vector<std::string>& lines) {
            std::string s;
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i > 0) s += "\n";
                s += lines[i];
            }
            return s;
        }

        bool isNameChar(char c) {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        }

        // text の中の %1, %2 … の番号を集める
        std::vector<int> percentNumbers(const std::string& text) {
            std::vector<int> numbers;
            for (size_t i = 0; i + 1 < text.size(); ++i) {
                if (text[i] == '%' && std::isdigit(static_cast<unsigned char>(text[i + 1]))) {
                    int n = 0;
                    size_t j = i + 1;
                    while (j < text.size() && std::isdigit(static_cast<unsigned char>(text[j]))) {
                        n = n * 10 + (text[j] - '0');
                        ++j;
                    }
                    numbers.push_back(n);
                    i = j - 1;
                }
            }
            return numbers;
        }

        // java のひな形の中の {NAME}（大文字の名前）を集める
        std::vector<std::string> placeholderNames(const std::string& tpl) {
            std::vector<std::string> names;
            for (size_t i = 0; i < tpl.size(); ++i) {
                if (tpl[i] != '{') continue;
                size_t j = i + 1;
                while (j < tpl.size() && isNameChar(tpl[j])) ++j;
                if (j < tpl.size() && tpl[j] == '}' && j > i + 1) {
                    std::string name = tpl.substr(i + 1, j - i - 1);
                    bool upper = std::isupper(static_cast<unsigned char>(name[0])) || name[0] == '_';
                    for (char c : name) {
                        if (std::islower(static_cast<unsigned char>(c))) upper = false;
                    }
                    if (upper) names.push_back(name);
                }
            }
            return names;
        }

        // ---------------------------------------------------------------------
        //  Blockly の JSON を読む道具
        // ---------------------------------------------------------------------
        const json* fieldOf(const json& block, const std::string& name) {
            auto fields = block.find("fields");
            if (fields == block.end() || !fields->is_object()) return nullptr;
            auto f = fields->find(name);
            return f == fields->end() ? nullptr : &*f;
        }

        const json* inputOf(const json& block, const std::string& name) {
            auto inputs = block.find("inputs");
            if (inputs == block.end() || !inputs->is_object()) return nullptr;
            auto in = inputs->find(name);
            if (in == inputs->end() || !in->is_object()) return nullptr;
            auto b = in->find("block");
            if (b != in->end() && b->is_object()) return &*b;
            auto s = in->find("shadow");  // 穴に最初から入っている数字ブロック
            if (s != in->end() && s->is_object()) return &*s;
            return nullptr;
        }

        const json* nextOf(const json& block) {
            auto next = block.find("next");
            if (next == block.end() || !next->is_object()) return nullptr;
            auto b = next->find("block");
            return (b != next->end() && b->is_object()) ? &*b : nullptr;
        }

        std::string stringOf(const json& block, const char* key) {
            auto v = block.find(key);
            return (v != block.end() && v->is_string()) ? v->get<std::string>() : "";
        }

        bool isDisabled(const json& block) {
            auto enabled = block.find("enabled");
            if (enabled != block.end() && enabled->is_boolean() && !enabled->get<bool>()) return true;
            auto reasons = block.find("disabledReasons");
            return reasons != block.end() && reasons->is_array() && !reasons->empty();
        }

        double numberFrom(const json* f, double fallback) {
            if (f == nullptr) return fallback;
            if (f->is_number()) return f->get<double>();
            if (f->is_string()) {
                std::istringstream is(f->get<std::string>());
                is.imbue(std::locale::classic());
                double v = 0;
                if (is >> v) return v;
            }
            return fallback;
        }

        // 例：「前に %1 進む」→「前に 100 進む」
        std::string labelOf(const BlockDef::Info& def, const json* block) {
            std::string out;
            const std::string& t = def.text;
            for (size_t i = 0; i < t.size(); ++i) {
                if (t[i] == '%' && i + 1 < t.size() && std::isdigit(static_cast<unsigned char>(t[i + 1]))) {
                    int n = 0;
                    size_t j = i + 1;
                    while (j < t.size() && std::isdigit(static_cast<unsigned char>(t[j]))) {
                        n = n * 10 + (t[j] - '0');
                        ++j;
                    }
                    i = j - 1;
                    if (n < 1 || n > static_cast<int>(def.args.size())) continue;
                    const BlockDef::Arg& arg = def.args[n - 1];
                    const json* f = block ? fieldOf(*block, arg.name) : nullptr;
                    switch (arg.kind) {
                    case BlockDef::Arg::Number:
                        out += formatNumber(numberFrom(f, arg.initial));
                        break;
                    case BlockDef::Arg::Dropdown: {
                        std::string value = (f && f->is_string()) ? f->get<std::string>() : "";
                        std::string shown = arg.options.empty() ? "" : arg.options[0].first;
                        for (const auto& o : arg.options) {
                            if (o.second == value) shown = o.first;
                        }
                        out += shown;
                        break;
                    }
                    case BlockDef::Arg::Value:
                        out += "( )";
                        break;
                    case BlockDef::Arg::Statements:
                        break;
                    }
                    continue;
                }
                out += t[i];
            }
            // 空白をまとめる
            std::string collapsed;
            for (char c : out) {
                if (c == ' ' && (collapsed.empty() || collapsed.back() == ' ')) continue;
                collapsed += c;
            }
            return trim(collapsed);
        }

        // カテゴリの色（kCategories から探す。なければ kOtherColor）
        int categoryColor(const std::string& category) {
            for (const auto& c : kCategories) {
                if (category == c.name) return c.color;
            }
            return kOtherColor;
        }

        // ブロックの色（.color() があればそれ、なければカテゴリの色）
        int colorOf(const BlockDef::Info& def) {
            if (def.color >= 0) return def.color;
            return categoryColor(def.category);
        }

        // ---------------------------------------------------------------------
        //  ブロックの設定ミスを探す（起動時に1回だけ）
        // ---------------------------------------------------------------------
        void checkDefinitions() {
            Registry& r = registry();
            for (const auto& p : r.blocks) {
                const BlockDef::Info& d = p->info;
                auto problem = [&](const std::string& text) {
                    r.problems.push_back("【ブロックの設定ミス】「" + d.type + "」" + text);
                    };
                if (d.type.empty()) {
                    r.problems.push_back("【ブロックの設定ミス】Block(\"\") の名前が空です");
                    continue;
                }
                if (!r.byType.emplace(d.type, &d).second) {
                    problem("という名前のブロックが2つあります");
                    continue;
                }
                if (d.text.empty()) problem("：.text(...) が書かれていません");

                // %1, %2 … と入力欄の数
                std::vector<int> numbers = percentNumbers(d.text);
                std::set<int> unique(numbers.begin(), numbers.end());
                bool countOk = unique.size() == d.args.size() && numbers.size() == unique.size();
                for (int n : unique) {
                    if (n < 1 || n > static_cast<int>(d.args.size())) countOk = false;
                }
                if (!countOk) {
                    problem("：text の %1, %2 … の数（" + std::to_string(numbers.size()) + "個）と、入力欄の数（" +
                        std::to_string(d.args.size()) + "個）が合っていません");
                }

                // 入力欄の名前
                std::set<std::string> names;
                for (const auto& arg : d.args) {
                    if (arg.name.empty()) problem("：名前が空の入力欄があります");
                    if (!names.insert(arg.name).second) problem("：入力欄の名前「" + arg.name + "」が2つあります");
                    if (arg.kind == BlockDef::Arg::Dropdown && arg.options.empty()) {
                        problem("：選ぶ欄「" + arg.name + "」に選択肢がありません");
                    }
                }

                // Java
                if (d.javaTemplate.empty() && !d.javaFunction) {
                    problem("：.java(...) が書かれていません");
                }
                if (!d.javaTemplate.empty()) {
                    for (const auto& name : placeholderNames(d.javaTemplate)) {
                        if (!d.findArg(name)) problem("：java の {" + name + "} に当たる入力欄がありません");
                    }
                    for (const auto& arg : d.args) {
                        if (d.javaTemplate.find("{" + arg.name + "}") == std::string::npos) {
                            problem("：入力欄「" + arg.name + "」が java の中で使われていません（{" + arg.name + "} を書く）");
                        }
                    }
                }
                if (d.isEvent && d.isOutput) problem("：.event() と .output() は一緒に使えません");
            }

            // onlyIn に書いたイベントがあるか（全部そろってから確かめる）
            for (const auto& p : r.blocks) {
                const BlockDef::Info& d = p->info;
                if (d.onlyIn.empty()) continue;
                auto it = r.byType.find(d.onlyIn);
                if (it == r.byType.end() || !it->second->isEvent) {
                    r.problems.push_back("【ブロックの設定ミス】「" + d.type + "」：onlyIn の「" + d.onlyIn +
                        "」というイベントブロックがありません");
                }
            }
        }

        // 最初から用意しておくブロック
        void addBuiltinBlocks() {
            Block(kNumberBlock)
                .category("計算")
                .text("%1")
                .number("NUM", 0)
                .output("Number")
                .tooltip("数字")
                .java("{NUM}");
        }

        // 全部のブロックを集める（最初に1回だけ）
        void ensureLoaded() {
            Registry& r = registry();
            if (r.loaded) return;
            r.loaded = true;
            addBuiltinBlocks();
            auto sets = r.sets;
            std::stable_sort(sets.begin(), sets.end(),
                [](const std::pair<std::string, BlockSetFunction>& a,
                    const std::pair<std::string, BlockSetFunction>& b) { return a.first < b.first; });
            for (const auto& s : sets) s.second();
            checkDefinitions();
        }

        // ---------------------------------------------------------------------
        //  Blockly 用の JSON をつくる
        // ---------------------------------------------------------------------
        json blockJson(const BlockDef::Info& d) {
            json j;
            j["type"] = d.type;
            j["message0"] = d.text;
            json args = json::array();
            bool hasValue = false;
            bool hasStatements = false;
            for (const auto& arg : d.args) {
                json a;
                a["name"] = arg.name;
                switch (arg.kind) {
                case BlockDef::Arg::Number:
                    a["type"] = "field_number";
                    a["value"] = arg.initial;
                    if (arg.hasMin) a["min"] = arg.min;
                    if (arg.hasMax) a["max"] = arg.max;
                    break;
                case BlockDef::Arg::Dropdown: {
                    a["type"] = "field_dropdown";
                    json options = json::array();
                    for (const auto& o : arg.options) options.push_back(json::array({ o.first, o.second }));
                    a["options"] = options;
                    break;
                }
                case BlockDef::Arg::Value:
                    a["type"] = "input_value";
                    if (!arg.valueType.empty()) a["check"] = arg.valueType;
                    hasValue = true;
                    break;
                case BlockDef::Arg::Statements:
                    a["type"] = "input_statement";
                    hasStatements = true;
                    break;
                }
                args.push_back(a);
            }
            if (!args.empty()) j["args0"] = args;
            j["colour"] = colorOf(d);
            if (!d.tooltip.empty()) j["tooltip"] = d.tooltip;
            if (hasValue && !hasStatements) j["inputsInline"] = true;
            if (d.isOutput) {
                j["output"] = d.outputType.empty() ? json(nullptr) : json(d.outputType);
            }
            else if (!d.isEvent) {
                j["previousStatement"] = nullptr;
                j["nextStatement"] = nullptr;
            }
            return j;
        }

        json toolboxJson() {
            const Registry& r = registry();
            std::vector<std::string> order;
            for (const auto& c : kCategories) order.push_back(c.name);
            for (const auto& p : r.blocks) {
                std::string cat = p->info.category.empty() ? "その他" : p->info.category;
                if (std::find(order.begin(), order.end(), cat) == order.end()) order.push_back(cat);
            }

            json contents = json::array();
            for (const auto& cat : order) {
                json items = json::array();
                int color = categoryColor(cat);   // カテゴリの色は kCategories で決める（ブロックの .color() は関係ない）
                std::set<std::string> added;
                for (const auto& p : r.blocks) {
                    const BlockDef::Info& d = p->info;
                    std::string blockCat = d.category.empty() ? "その他" : d.category;
                    if (blockCat != cat || !added.insert(d.type).second) continue;
                    json item;
                    item["kind"] = "block";
                    item["type"] = d.type;
                    for (const auto& arg : d.args) {
                        if (arg.kind == BlockDef::Arg::Value && arg.valueType == "Number") {
                            item["inputs"][arg.name]["shadow"] = { {"type", kNumberBlock}, {"fields", {{"NUM", 0}}} };
                        }
                    }
                    items.push_back(item);
                }
                if (items.empty()) continue;
                contents.push_back({ {"kind", "category"},
                                    {"name", cat},
                                    {"colour", std::to_string(color)},
                                    {"contents", items} });
            }
            return { {"kind", "categoryToolbox"}, {"contents", contents} };
        }

    }  // namespace

    // =====================================================================
    //  Java への変換
    // =====================================================================
    class Converter {
    public:
        JavaResult result;
        const BlockDef::Info* currentEvent = nullptr;

        const BlockDef::Info* find(const std::string& type) const {
            const auto& m = registry().byType;
            auto it = m.find(type);
            return it == m.end() ? nullptr : it->second;
        }

        void add(std::vector<Message>& list, const json* block, const BlockDef::Info* def, const std::string& text) {
            Message m;
            m.blockId = block ? stringOf(*block, "id") : "";
            m.blockLabel = def ? labelOf(*def, block) : "";
            m.text = text;
            for (const auto& old : list) {
                if (old.blockId == m.blockId && old.blockLabel == m.blockLabel && old.text == m.text) return;
            }
            list.push_back(m);
        }
        void error(const json* block, const BlockDef::Info* def, const std::string& text) {
            add(result.errors, block, def, text);
        }
        void warning(const json* block, const BlockDef::Info* def, const std::string& text) {
            add(result.warnings, block, def, text);
        }
        void mistake(const BlockDef::Info& def, const std::string& text) {
            error(nullptr, nullptr, "【ブロックの設定ミス】「" + def.type + "」" + text);
        }

        // 1つのブロックを Java にする
        std::string render(const BlockDef::Info& def, const json& block);

        // つながっている命令ブロックを順番に Java にする
        std::string statementChain(const json* first) {
            std::vector<std::string> parts;
            for (const json* b = first; b != nullptr; b = nextOf(*b)) {
                if (isDisabled(*b)) continue;
                std::string type = stringOf(*b, "type");
                const BlockDef::Info* def = find(type);
                if (!def) {
                    error(b, nullptr, "知らないブロックです（" + type + "）");
                    continue;
                }
                if (def->isEvent || def->isOutput) {
                    error(b, def, "このブロックはここには置けません");
                    continue;
                }
                std::string code = render(*def, *b);
                if (!code.empty()) parts.push_back(code);
            }
            return joinLines(parts);
        }

        // 穴にはまっている値ブロックを Java にする
        std::string valueInput(const json& block, const BlockDef::Info& def, const BlockDef::Arg& arg) {
            const json* child = inputOf(block, arg.name);
            if (!child || isDisabled(*child)) {
                error(&block, &def, "穴にブロックが入っていません");
                return arg.valueType == "Boolean" ? "false" : "0";
            }
            std::string type = stringOf(*child, "type");
            const BlockDef::Info* childDef = find(type);
            if (!childDef) {
                error(child, nullptr, "知らないブロックです（" + type + "）");
                return "0";
            }
            if (!childDef->isOutput) {
                error(child, childDef, "このブロックは穴にははめられません");
                return "0";
            }
            return render(*childDef, *child);
        }

        // 口の中の命令ブロックを Java にする
        std::string statementInput(const json& block, const BlockDef::Info& def, const BlockDef::Arg& arg) {
            std::string code = statementChain(inputOf(block, arg.name));
            if (code.empty()) warning(&block, &def, "中にブロックが入っていません");
            return code;
        }

    private:
        void runChecks(const BlockDef::Info& def, const json& block, BlockData& data) {
            if (!def.onlyIn.empty() && (!currentEvent || currentEvent->type != def.onlyIn)) {
                const BlockDef::Info* ev = find(def.onlyIn);
                std::string evName = ev ? labelOf(*ev, nullptr) : def.onlyIn;
                error(&block, &def, "このブロックは「" + evName + "」の中でしか使えません");
            }
            for (const auto& arg : def.args) {
                if (arg.kind != BlockDef::Arg::Number) continue;
                double v = numberFrom(fieldOf(block, arg.name), arg.initial);
                if (arg.hasMin && v < arg.min) {
                    error(&block, &def, formatNumber(arg.min) + " より小さい数字は使えません");
                }
                else if (arg.hasMax && v > arg.max) {
                    error(&block, &def, formatNumber(arg.max) + " より大きい数字は使えません");
                }
            }
            for (const auto& check : def.checks) check(data);
        }

        // {名前} を入力欄の中身に置きかえる
        std::string replacePlaceholders(const std::string& line, const BlockDef::Info& def, BlockData& data) {
            std::string out;
            size_t i = 0;
            while (i < line.size()) {
                if (line[i] == '{') {
                    size_t close = line.find('}', i + 1);
                    if (close != std::string::npos) {
                        std::string name = line.substr(i + 1, close - i - 1);
                        if (!name.empty() && def.findArg(name)) {
                            out += data.text(name);
                            i = close + 1;
                            continue;
                        }
                    }
                }
                out += line[i];
                ++i;
            }
            return out;
        }

        std::string fillTemplate(const BlockDef::Info& def, BlockData& data) {
            std::vector<std::string> out;
            for (const std::string& line : splitLines(def.javaTemplate)) {
                // {DO} だけの行 → 中のブロックを、その行の字下げにそろえて入れる
                std::string t = trim(line);
                if (t.size() > 2 && t.front() == '{' && t.back() == '}') {
                    const BlockDef::Arg* arg = def.findArg(t.substr(1, t.size() - 2));
                    if (arg && arg->kind == BlockDef::Arg::Statements) {
                        std::string indent = line.substr(0, line.find_first_not_of(" \t"));
                        for (const std::string& inner : splitLines(data.text(arg->name))) {
                            out.push_back(inner.empty() ? "" : indent + inner);
                        }
                        continue;
                    }
                }
                out.push_back(replacePlaceholders(line, def, data));
            }
            return joinLines(out);
        }
    };

    struct BlockData::Impl {
        Converter* conv;
        const json* block;
        const BlockDef::Info* def;
    };

    std::string Converter::render(const BlockDef::Info& def, const json& block) {
        BlockData::Impl impl{ this, &block, &def };
        BlockData data(&impl);
        runChecks(def, block, data);
        if (def.javaFunction) return def.javaFunction(data);
        return fillTemplate(def, data);
    }

    // =====================================================================
    //  BlockData（check / javaFunction の中で使うもの）
    // =====================================================================
    double BlockData::number(const std::string& name) {
        const BlockDef::Info& def = *impl_->def;
        const BlockDef::Arg* arg = def.findArg(name);
        if (!arg || arg->kind != BlockDef::Arg::Number) {
            impl_->conv->mistake(def, "に「" + name + "」という数字の入力欄がありません");
            return 0;
        }
        return numberFrom(fieldOf(*impl_->block, name), arg->initial);
    }

    std::string BlockData::text(const std::string& name) {
        const BlockDef::Info& def = *impl_->def;
        const BlockDef::Arg* arg = def.findArg(name);
        if (!arg) {
            impl_->conv->mistake(def, "に「" + name + "」という入力欄がありません");
            return "";
        }
        switch (arg->kind) {
        case BlockDef::Arg::Number:
            return formatNumber(number(name));
        case BlockDef::Arg::Dropdown: {
            const json* f = fieldOf(*impl_->block, name);
            if (f && f->is_string()) return f->get<std::string>();
            return arg->options.empty() ? "" : arg->options[0].second;
        }
        case BlockDef::Arg::Value:
            return impl_->conv->valueInput(*impl_->block, def, *arg);
        case BlockDef::Arg::Statements:
            return impl_->conv->statementInput(*impl_->block, def, *arg);
        }
        return "";
    }

    void BlockData::error(const std::string& message) {
        impl_->conv->error(impl_->block, impl_->def, message);
    }

    void BlockData::warning(const std::string& message) {
        impl_->conv->warning(impl_->block, impl_->def, message);
    }

    // =====================================================================
    //  BlockDef（Block("名前").〇〇() で使うもの）
    // =====================================================================
    BlockDef::BlockDef(const std::string& type) { info.type = type; }

    const BlockDef::Arg* BlockDef::Info::findArg(const std::string& name) const {
        for (const auto& arg : args) {
            if (arg.name == name) return &arg;
        }
        return nullptr;
    }

    BlockDef& BlockDef::category(const std::string& name) { info.category = name; return *this; }
    BlockDef& BlockDef::text(const std::string& message) { info.text = message; return *this; }
    BlockDef& BlockDef::tooltip(const std::string& message) { info.tooltip = message; return *this; }
    BlockDef& BlockDef::color(int hue) { info.color = hue; return *this; }

    BlockDef& BlockDef::number(const std::string& name, double initial) {
        Arg arg;
        arg.kind = Arg::Number;
        arg.name = name;
        arg.initial = initial;
        info.args.push_back(arg);
        return *this;
    }

    BlockDef& BlockDef::number(const std::string& name, double initial, double min) {
        number(name, initial);
        info.args.back().hasMin = true;
        info.args.back().min = min;
        return *this;
    }

    BlockDef& BlockDef::number(const std::string& name, double initial, double min, double max) {
        number(name, initial, min);
        info.args.back().hasMax = true;
        info.args.back().max = max;
        return *this;
    }

    BlockDef& BlockDef::dropdown(const std::string& name, const Options& options) {
        Arg arg;
        arg.kind = Arg::Dropdown;
        arg.name = name;
        arg.options = options;
        info.args.push_back(arg);
        return *this;
    }

    BlockDef& BlockDef::valueInput(const std::string& name, const std::string& type) {
        Arg arg;
        arg.kind = Arg::Value;
        arg.name = name;
        arg.valueType = type;
        info.args.push_back(arg);
        return *this;
    }

    BlockDef& BlockDef::statements(const std::string& name) {
        Arg arg;
        arg.kind = Arg::Statements;
        arg.name = name;
        info.args.push_back(arg);
        return *this;
    }

    BlockDef& BlockDef::output(const std::string& type) {
        info.isOutput = true;
        info.outputType = type;
        return *this;
    }

    BlockDef& BlockDef::event() { info.isEvent = true; return *this; }
    BlockDef& BlockDef::java(const std::string& javaTemplate) { info.javaTemplate = javaTemplate; return *this; }
    BlockDef& BlockDef::javaFunction(JavaFunction function) { info.javaFunction = function; return *this; }
    BlockDef& BlockDef::onlyIn(const std::string& eventType) { info.onlyIn = eventType; return *this; }
    BlockDef& BlockDef::check(CheckFunction function) { info.checks.push_back(function); return *this; }

    BlockDef& BlockDef::warnIfZero(const std::string& name, const std::string& message) {
        info.checks.push_back([name, message](BlockData& b) {
            if (b.number(name) == 0) b.warning(message);
            });
        return *this;
    }

    BlockDef& Block(const std::string& type) {
        Registry& r = registry();
        r.blocks.push_back(std::unique_ptr<BlockDef>(new BlockDef(type)));
        r.blocks.back()->info.order = static_cast<int>(r.blocks.size());
        return *r.blocks.back();
    }

    AutoRegister::AutoRegister(const char* name, BlockSetFunction function) {
        registry().sets.emplace_back(name, function);
    }

    // =====================================================================
    //  アプリ側が使う関数
    // =====================================================================
    std::string GetBlocklySetupJson() {
        ensureLoaded();
        const Registry& r = registry();
        json blocks = json::array();
        std::set<std::string> added;
        for (const auto& p : r.blocks) {
            if (p->info.type.empty() || !added.insert(p->info.type).second) continue;
            blocks.push_back(blockJson(p->info));
        }
        json setup;
        setup["blocks"] = blocks;
        setup["toolbox"] = toolboxJson();
        setup["problems"] = r.problems;
        return setup.dump(-1, ' ', false, json::error_handler_t::replace);
    }
    JavaResult ConvertToJava(const std::string& workspaceJson, const std::string& robotName, const std::string& packageName) {
        ensureLoaded();
        Converter conv;
        for (const auto& p : registry().problems) conv.error(nullptr, nullptr, p);

        const std::string name = trim(robotName);
        if (name.empty()) conv.error(nullptr, nullptr, "ロボットの名前が入っていません");

        try {
            json ws = json::parse(workspaceJson);

            // 一番上に置かれているブロックを集める
            std::vector<const json*> tops;
            auto blocks = ws.find("blocks");
            if (blocks != ws.end() && blocks->is_object()) {
                auto list = blocks->find("blocks");
                if (list != blocks->end() && list->is_array()) {
                    for (const auto& b : *list) {
                        if (b.is_object()) tops.push_back(&b);
                    }
                }
            }

            // イベントブロックを選び出す
            std::vector<std::pair<const BlockDef::Info*, const json*>> events;
            std::set<std::string> seen;
            for (const json* b : tops) {
                if (isDisabled(*b)) continue;
                std::string type = stringOf(*b, "type");
                const BlockDef::Info* def = conv.find(type);
                if (!def) {
                    conv.error(b, nullptr, "知らないブロックです（" + type + "）");
                }
                else if (!def->isEvent) {
                    conv.warning(b, def, "どのイベントにもつながっていないので、動きません");
                }
                else if (!seen.insert(type).second) {
                    conv.error(b, def, "このイベントブロックは1つだけにしてください");
                }
                else {
                    events.emplace_back(def, b);
                }
            }
            if (events.empty()) {
                conv.error(nullptr, nullptr, "イベントブロック（「〜とき」などの一番上に置くブロック）を1つ以上置いてください");
            }
            std::stable_sort(events.begin(), events.end(),
                [](const std::pair<const BlockDef::Info*, const json*>& a,
                    const std::pair<const BlockDef::Info*, const json*>& b) {
                        return a.first->order < b.first->order;
                });

            // イベントごとに Java のメソッドを作る
            std::string body;
            for (const auto& e : events) {
                conv.currentEvent = e.first;
                std::string method = conv.render(*e.first, *e.second);
                conv.currentEvent = nullptr;
                if (!body.empty()) body += "\n";
                for (const std::string& line : splitLines(method)) {
                    body += (line.empty() ? "" : "    " + line) + "\n";
                }
            }

            // Java のファイル全体
            std::ostringstream java;
            java << "package " << (trim(packageName).empty() ? kJavaPackage : trim(packageName)) << ";\n"
                << "\n"
                << "import robocode.*;\n"
                << "\n"
                << "// Generated by Robocode Blocks\n"
                << "public class " << name << " extends Robot {\n";
            if (!body.empty()) java << "\n" << body;
            java << "}\n";
            conv.result.java = java.str();
        }
        catch (const json::parse_error&) {
            conv.error(nullptr, nullptr, "ブロックのデータが読み込めませんでした");
        }
        catch (const std::exception& ex) {
            conv.error(nullptr, nullptr, std::string("変換中に問題が起きました：") + ex.what());
        }

        conv.result.ok = conv.result.errors.empty();
        return conv.result;
    }

    std::string JavaResult::messagesText() const {
        std::string s;
        auto put = [&s](const char* title, const std::vector<Message>& list) {
            if (list.empty()) return;
            s += title;
            s += "\n";
            for (const auto& m : list) {
                s += "・";
                if (!m.blockLabel.empty()) s += "【" + m.blockLabel + "】";
                s += m.text + "\n";
            }
            };
        put("エラー（直さないと動きません）", errors);
        if (!errors.empty() && !warnings.empty()) s += "\n";
        put("注意", warnings);
        return s;
    }

}  // namespace rb