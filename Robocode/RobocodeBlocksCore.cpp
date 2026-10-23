// =====================================================================
//  RobocodeBlocksCore.cpp  ―  ブロックのしくみ（難しい部分）
// 
// 2026/10/16 片山　カテゴリー色に色とサンプル戦術追加,Javaコードの親クラス名を修正。
// 2026/10/20 片山　staticを使えるようにする
// 2026/10/20 関数（イベントの情報 e を受け取る関数）と、static の変数宣言を1つにまとめた
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
        const char* const kRawStatementBlock = "java_raw_statement";    // Java をそのまま書く（命令）
        const char* const kRawExpressionBlock = "java_raw_expression";  // Java をそのまま書く（式）
        const char* const kRawMemberBlock = "java_raw_member";          // Java をそのまま書く（メソッド・変数）
        const char* const kReturnBlock = "rb_return";       // ここで終わる（return;）
        //const char* const kVariableCategory = "変数";   // Blockly にもともとある変数のしくみを使うカテゴリ
        const char* const kFunctionCategory = "関数";   // Blockly にもともとある関数のしくみを使うカテゴリ

        // ツールボックスに並ぶ順番と色（ここにないカテゴリは後ろに並ぶ）
        struct CategoryColor {
            const char* name;
            int color;
        };
        const CategoryColor kCategories[] = {
            {"イベント", 30},
            {"動き", 210},
            {"同時に動く", 60},  // AdvancedMove.cpp（setAhead などの予約して同時に動くブロック）
            {"攻撃", 0},
            {"センサー", 160},
            {"制御", 120},
            {"計算", 230},
            {"色",270},
            {"ゲーム情報",180},
            {"変数", 330},
            {"関数", 290},
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
                    const BlockDef::Arg& arg = def.args[static_cast<size_t>(n) - 1];
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
                    case BlockDef::Arg::OptionalValue:
                        out += "( )";
                        break;
                    case BlockDef::Arg::Statements:
                        break;
                    case BlockDef::Arg::Text:
                        out += (f && f->is_string()) ? f->get<std::string>() : arg.initialText;
                        break;
                    case BlockDef::Arg::Variable:   // 変数を選ぶ欄
                        out += (f && f->is_string()) ? f->get<std::string>() : arg.initialText;
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

        // 変数の名前を、Java で使える名前にする（例：当たった回数 → v_当たった回数）
        //   Java で使えない文字（空白・記号など）は _ にする。日本語（ひらがな・カタカナ・漢字）はそのまま使える。
        std::string javaSafeName(const std::string& prefix, const std::string& name) {
            std::string out = prefix;
            size_t i = 0;
            while (i < name.size()) {
                unsigned char c = static_cast<unsigned char>(name[i]);
                // UTF-8 の1文字を読む
                unsigned int cp = c;
                size_t len = 1;
                if (c >= 0xF0) { cp = c & 0x07; len = 4; }
                else if (c >= 0xE0) { cp = c & 0x0F; len = 3; }
                else if (c >= 0xC0) { cp = c & 0x1F; len = 2; }
                if (i + len > name.size()) len = name.size() - i;
                for (size_t k = 1; k < len; ++k) cp = (cp << 6) | (static_cast<unsigned char>(name[i + k]) & 0x3F);
                bool ok = (cp < 0x80 && (std::isalnum(c) || c == '_')) ||
                          (cp >= 0x3041 && cp <= 0x3096) ||   // ひらがな
                          (cp >= 0x30A1 && cp <= 0x30FA) ||   // カタカナ
                          cp == 0x30FC || cp == 0x3005 ||     // ー 々
                          (cp >= 0x4E00 && cp <= 0x9FFF) ||   // 漢字
                          (cp >= 0xFF10 && cp <= 0xFF19) ||   // 全角数字
                          (cp >= 0xFF21 && cp <= 0xFF3A) || (cp >= 0xFF41 && cp <= 0xFF5A);  // 全角英字
                out += ok ? name.substr(i, len) : "_";
                i += len;
            }
            return out;
        }
        // 「型 名前」の形（例：int 回数、double[] scores）なら、型と名前に分ける
        //   関数の引数と、値を返す関数の名前で使う。型が書かれていなければ double（false を返す）
        const BlockDef::Info* eventOfClass(const std::string& className);   // 下で作る
        bool splitTypedName(const std::string& raw, std::string& type, std::string& name) {
            static const std::set<std::string> kTypes = { "double", "int", "float", "long", "boolean", "String" };
            // イベントの情報（例：ScannedRobotEvent e）も型として読む
            {
                std::string s0 = trim(raw);
                size_t sp = s0.find(' ');
                if (sp != std::string::npos && eventOfClass(s0.substr(0, sp))) {
                    type = s0.substr(0, sp);
                    name = trim(s0.substr(sp + 1));
                    return true;
                }
            }
            std::string s = trim(raw);
            size_t space = s.find(' ');
            if (space != std::string::npos) {
                std::string t = s.substr(0, space);
                std::string base = t;
                while (base.size() >= 2 && base.compare(base.size() - 2, 2, "[]") == 0) base.resize(base.size() - 2);
                if (kTypes.count(base)) {
                    type = t;
                    name = trim(s.substr(space + 1));
                    return true;
                }
            }
            type = "double";
            name = s;
            return false;
        }
        std::string typeOfName(const std::string& raw) { std::string t, n; splitTypedName(raw, t, n); return t; }
        std::string nameWithoutType(const std::string& raw) { std::string t, n; splitTypedName(raw, t, n); return n; }

        std::string variableJavaName(const std::string& name) { return javaSafeName("v_", nameWithoutType(name)); }
        // 関数の名前を、Java で使える名前にする（例：前に進んで撃つ → f_前に進んで撃つ、int 数える → f_数える）
        std::string functionJavaName(const std::string& name) { return javaSafeName("f_", nameWithoutType(name)); }

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
        //   数字の穴（valueInput の "Number"）に最初から入っている数字ブロック。
        //   これがないと、数字の穴があるブロックのカテゴリを開いたときにエラーになる。消さないこと！
        void addBuiltinBlocks() {
            Block(kNumberBlock)
                .category("計算")
                .text("%1")
                .number("NUM", 0)
                .output("Number")
                .tooltip("数字")
                .java("{NUM}");

            // ---- Java からブロックにもどしたとき、当てはまるブロックがなかった部分 ----
            //   入力した Java がそのまま入る。自分で Java を書きたいときにも使える。
            Block(kRawStatementBlock)
                .category("Java")
                .text("Java %1")
                .textField("CODE", "")
                .tooltip("書いた Java がそのまま入ります（例：ahead(100);）")
                .java("{CODE}");

            Block(kRawExpressionBlock)
                .category("Java")
                .text("Java の式 %1")
                .textField("CODE", "0")
                .output("")                      // どの穴にもはまる
                .tooltip("書いた Java の式がそのまま入ります（例：getX() * 2）")
                .java("{CODE}");

            Block(kRawMemberBlock)
                .category("Java")
                .text("Java（メソッド・変数） %1")
                .textField("CODE", "")
                .event()                         // 一番上に置く（クラスの中にそのまま入る）
                .tooltip("ロボットのクラスの中に、書いた Java がそのまま入ります（メソッドや変数）")
                .java("{CODE}");

            // ---- 関数やイベントを、とちゅうで終わりにする（Java の return;）----
            //   このブロックの下には、ブロックをつなげられない（つなげるとエラー）
            Block(kReturnBlock)
                .category("制御")
                .text("ここで終わる")
                .tooltip("今の関数（またはイベント）を、ここで終わりにします。下にはブロックをつなげられません")
                .java("return;");
        }

        // イベントブロックの Java（public void onHitWall(HitWallEvent e) {）から、
        // イベントの情報の型（HitWallEvent）と名前（e）を取り出す。run() のように何もなければ false
        bool eventParameter(const BlockDef::Info& ev, std::string& className, std::string& name) {
            const std::string& t = ev.javaTemplate;
            size_t open = t.find('(');
            size_t close = open == std::string::npos ? open : t.find(')', open);
            if (close == std::string::npos) return false;
            std::istringstream in(t.substr(open + 1, close - open - 1));
            className.clear();
            name.clear();
            in >> className >> name;
            return !className.empty() && !name.empty();
        }

        // イベントの情報の型（ScannedRobotEvent など）から、そのイベントブロックをさがす（なければ nullptr）
        const BlockDef::Info* eventOfClass(const std::string& className) {
            if (className.empty()) return nullptr;
            for (const auto& p : registry().blocks) {
                std::string cls, prm;
                if (p->info.isEvent && eventParameter(p->info, cls, prm) && cls == className) return &p->info;
            }
            return nullptr;
        }

        // Java のコードの中に、e という名前（e.getBearing() の e など）が出てくるか
        bool usesEventName(const std::string& code) {
            auto identChar = [](char c) {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || static_cast<unsigned char>(c) >= 0x80;
            };
            for (size_t i = 0; i < code.size(); ++i) {
                if (code[i] != 'e') continue;
                bool before = i > 0 && (identChar(code[i - 1]) || code[i - 1] == '.');
                bool after = i + 1 < code.size() && identChar(code[i + 1]);
                if (!before && !after) return true;
            }
            return false;
        }

        // 全部のブロックを集める（最初に1回だけ）
        void ensureLoaded() {
            Registry& r = registry();
            if (r.loaded) return;
            r.loaded = true;
            addBuiltinBlocks();   // 数字ブロックを最初に作る（消さないこと！）
            // 並べかえるためにコピーする（元の順番は変えない）
            std::vector<std::pair<std::string, BlockSetFunction>> sets = r.sets;
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
                case BlockDef::Arg::OptionalValue:
                    a["type"] = "input_value";
                    if (!arg.valueType.empty()) a["check"] = arg.valueType;
                    // optionalValueInput は、接続を外しても編集可能な数字ブロックを初期表示する。
                    // shadow block はユーザーが別の値ブロックを接続すると置き換わる。
                    if (arg.kind == BlockDef::Arg::OptionalValue && arg.valueType == "Number") {
                        a["shadow"] = {
                            {"type", kNumberBlock},
                            {"fields", {{"NUM", 0}}}
                        };
                    }
                    hasValue = true;
                    break;
                case BlockDef::Arg::Statements:
                    a["type"] = "input_statement";
                    hasStatements = true;
                    break;
                case BlockDef::Arg::Text:
                    a["type"] = "field_input";
                    a["text"] = arg.initialText;
                    a["spellcheck"] = false;
                    break;
                case BlockDef::Arg::Variable:   // 変数を選ぶ欄
                    a["type"] = "field_variable";
                    a["variable"] = arg.initialText;
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
                //// 「変数」カテゴリ：Blockly が「変数を作る」ボタンとブロックを自動で出す
                if (cat == kFunctionCategory) {
                    // 「関数」カテゴリ：Blockly が「関数を作る」ブロックと、作った関数を呼ぶブロックを自動で出す
                    contents.push_back({ {"kind", "category"},
                                        {"name", cat},
                                        {"colour", std::to_string(categoryColor(cat))},
                                        {"custom", "PROCEDURE"} });
                    continue;
                }
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

        // クラス直下に出力する変数宣言（static の変数）をためておく場所
        std::string* classMembers = nullptr;

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

        // ---- 変数（Blockly にもともとあるブロック） ----
        //   variables_set：〇 を △ にする     → v_名前 = △;
        //   math_change  ：〇 を △ ずつ変える → v_名前 += △;
        //   variables_get：〇                 → v_名前
        //   math_number  ：数字（変数のブロックの穴に最初から入っている）
        std::map<std::string, std::string> variableJavaNames;  // 変数の id → Java の名前

        std::string variableOf(const json& block) {
            const json* f = fieldOf(block, "VAR");
            std::string id, name;
            if (f && f->is_object()) {
                id = stringOf(*f, "id");
                name = stringOf(*f, "name");
            } else if (f && f->is_string()) {
                id = f->get<std::string>();
            }
            auto it = variableJavaNames.find(id);
            if (it != variableJavaNames.end()) return it->second;
            if (!name.empty()) return variableJavaName(name);
            error(&block, nullptr, "変数が見つかりません");
            return "v_unknown";
        }

        bool isBuiltinValue(const std::string& type) const {
            return type == "variables_get" || type == "math_number" || type == "procedures_callreturn";
        }
        bool isBuiltinStatement(const std::string& type) const {
            return type == "variables_set" || type == "math_change" ||
                   type == "procedures_callnoreturn" || type == "procedures_ifreturn";
        }
        bool isFunctionDefinition(const std::string& type) const {
            return type == "procedures_defnoreturn" || type == "procedures_defreturn";
        }

        // ---- 関数（Blockly にもともとあるブロック） ----
        //   procedures_defnoreturn：関数を作る          → private void f_名前(double v_引数) { … }
        //   （private：このロボットの中だけで使う関数。ほかのクラスからは呼べない）
        //   procedures_defreturn  ：値を返す関数を作る  → private double f_名前(double v_引数) { … return 値; }
        //   procedures_callnoreturn：関数を呼ぶ（命令）  → f_名前(値);
        //   procedures_callreturn ：関数を呼ぶ（値）    → f_名前(値)
        //   procedures_ifreturn   ：もし〇なら値を返す  → if (〇) { return 値; }
        bool insideReturnFunction = false;   // 今、値を返す関数の中を変換しているか

        // ---- イベントの情報（e）を使う関数 ----
        //   関数の中に「敵までの距離」のような、あるイベントの中でしか使えないブロックがあると、
        //   その関数は private void f_名前(ScannedRobotEvent e, …) のように、イベントの情報 e を受け取る。
        //   呼ぶほうも f_名前(e, …) になる（ブロックでは e のことは考えなくてよい）。
        //   どのイベントの e かは、中のブロック → 中で呼んでいる関数 → 呼ばれている場所 の順に決める。
        struct FunctionInfo {
            const json* def = nullptr;
            std::set<std::string> directEvents;   // 中のブロックの onlyIn
            bool rawUsesEvent = false;            // そのまま書く Java や、onlyIn のないブロックの Java で e を使っている
            std::set<std::string> callees;        // 中で呼んでいる関数
            std::set<std::string> callerEvents;   // この関数を呼んでいるイベント
            std::set<std::string> callerFunctions;// この関数を呼んでいる関数
            bool needsEvent = false;
            std::set<std::string> eventTypes;
            const BlockDef::Info* event = nullptr;  // 受け取るイベント（1つに決まったとき）
            bool explicitEventParam = false;       // 引数に「ScannedRobotEvent e」のように書いてある
        };
        std::map<std::string, FunctionInfo> functionInfos;   // Blockly での関数の名前 → ようす

        static std::string nameOfFunctionBlock(const json& block) {
            const json* f = nullptr;
            auto fields = block.find("fields");
            if (fields != block.end() && fields->is_object()) {
                auto it = fields->find("NAME");
                if (it != fields->end()) f = &*it;
            }
            return (f && f->is_string()) ? f->get<std::string>() : "";
        }
        std::string calledName(const json& block) {
            auto it = block.find("extraState");
            return (it != block.end() && it->is_object()) ? stringOf(*it, "name") : "";
        }

        // ブロックの中を全部たどって、onlyIn・e を使うそのまま書く Java・呼んでいる関数を集める
        void collectUses(const json& block, std::set<std::string>& events, bool& rawUsesEvent,
                         std::set<std::string>& callees) {
            if (!block.is_object() || isDisabled(block)) return;
            std::string type = stringOf(block, "type");
            if (type == "procedures_callnoreturn" || type == "procedures_callreturn") callees.insert(calledName(block));
            if (type == kRawStatementBlock || type == kRawExpressionBlock) {
                const json* code = fieldOf(block, "CODE");
                if (code && code->is_string() && usesEventName(code->get<std::string>())) rawUsesEvent = true;
            }
            if (const BlockDef::Info* def = find(type)) {
                if (!def->onlyIn.empty()) events.insert(def->onlyIn);
                // onlyIn が書かれていなくても、Java のひな形で e（e.getBearing() など）を使っているブロック
                else if (usesEventName(def->javaTemplate)) rawUsesEvent = true;
            }
            auto inputs = block.find("inputs");
            if (inputs != block.end() && inputs->is_object()) {
                for (const auto& in : inputs->items()) {
                    if (!in.value().is_object()) continue;
                    auto b = in.value().find("block");
                    if (b != in.value().end()) collectUses(*b, events, rawUsesEvent, callees);
                }
            }
            if (const json* next = nextOf(block)) collectUses(*next, events, rawUsesEvent, callees);
        }

        void analyzeFunctions(const std::vector<std::pair<const BlockDef::Info*, const json*>>& events,
                              const std::vector<const json*>& functions) {
            for (const json* f : functions) {
                FunctionInfo& info = functionInfos[nameOfFunctionBlock(*f)];
                info.def = f;
                // 引数に「ScannedRobotEvent e」と書いてあれば、そのイベントの情報を受け取る関数
                auto st = f->find("extraState");
                if (st != f->end() && st->is_object() && st->contains("params") && (*st)["params"].is_array()) {
                    for (const auto& prm : (*st)["params"]) {
                        std::string t, n;
                        splitTypedName(stringOf(prm, "name"), t, n);
                        if (const BlockDef::Info* ev = eventOfClass(t)) {
                            info.directEvents.insert(ev->type);
                            info.explicitEventParam = true;
                        }
                    }
                }
                collectUses(*f, info.directEvents, info.rawUsesEvent, info.callees);
            }
            for (const auto& e : events) {
                std::set<std::string> dummyEvents, callees;
                bool dummyRaw = false;
                collectUses(*e.second, dummyEvents, dummyRaw, callees);
                std::string cls, prm;
                for (const auto& c : callees) {
                    auto it = functionInfos.find(c);
                    if (it != functionInfos.end() && eventParameter(*e.first, cls, prm)) it->second.callerEvents.insert(e.first->type);
                }
            }
            for (auto& f : functionInfos) {
                for (const auto& c : f.second.callees) {
                    auto it = functionInfos.find(c);
                    if (it != functionInfos.end()) it->second.callerFunctions.insert(f.first);
                }
            }
            // 決まるまでくり返す（関数が関数を呼んでいることがあるため）
            for (int round = 0; round < 50; ++round) {
                bool changed = false;
                for (auto& f : functionInfos) {
                    FunctionInfo& info = f.second;
                    bool needs = !info.directEvents.empty() || info.rawUsesEvent;
                    std::set<std::string> types = info.directEvents;
                    for (const auto& c : info.callees) {
                        auto it = functionInfos.find(c);
                        if (it == functionInfos.end() || !it->second.needsEvent) continue;
                        needs = true;
                        types.insert(it->second.eventTypes.begin(), it->second.eventTypes.end());
                    }
                    if (needs && types.empty()) {   // 中では決まらない → 呼ばれている場所で決める
                        types = info.callerEvents;
                        for (const auto& c : info.callerFunctions) {
                            const FunctionInfo& caller = functionInfos[c];
                            types.insert(caller.eventTypes.begin(), caller.eventTypes.end());
                        }
                    }
                    if (needs != info.needsEvent || types != info.eventTypes) {
                        info.needsEvent = needs;
                        info.eventTypes = types;
                        changed = true;
                    }
                }
                if (!changed) break;
            }
            for (auto& f : functionInfos) {
                FunctionInfo& info = f.second;
                if (!info.needsEvent) continue;
                if (info.eventTypes.size() == 1) {
                    info.event = find(*info.eventTypes.begin());
                    continue;
                }
                if (info.eventTypes.empty()) {
                    error(info.def, nullptr, "関数「" + f.first + "」はイベントの情報（e）を使っていますが、"
                          "どのイベントから呼ばれるのか分かりません。イベントの中から呼んでください");
                    continue;
                }
                std::string names;
                for (const auto& t : info.eventTypes) {
                    const BlockDef::Info* ev = find(t);
                    names += "「" + (ev ? labelOf(*ev, nullptr) : t) + "」";
                }
                error(info.def, nullptr, "関数「" + f.first + "」の中に、ちがうイベント" + names +
                      "でしか使えないブロックがまざっています。1つのイベントのものだけにしてください");
            }
        }

        // 関数を呼ぶ：f_名前(引数1, 引数2)（イベントの情報を使う関数なら f_名前(e, 引数1, …)）
        std::string functionCall(const json& block) {
            const json* state = nullptr;
            auto it = block.find("extraState");
            if (it != block.end() && it->is_object()) state = &*it;
            std::string name = state ? stringOf(*state, "name") : "";
            size_t count = 0;
            if (state && state->contains("params") && (*state)["params"].is_array()) count = (*state)["params"].size();
            std::string args;
            std::string eventArg;   // イベントの情報として渡すもの（e）
            auto info = functionInfos.find(name);
            if (info != functionInfos.end() && info->second.needsEvent && info->second.event) {
                const BlockDef::Info* need = info->second.event;
                std::string cls, prm;
                if (!currentEvent || currentEvent->type != need->type || !eventParameter(*currentEvent, cls, prm)) {
                    error(&block, nullptr, "関数「" + name + "」は「" + labelOf(*need, nullptr) +
                          "」の情報を使うので、「" + labelOf(*need, nullptr) + "」の中でしか呼べません");
                    prm = "e";
                }
                eventArg = insideFunction ? "e" : prm;
                if (!info->second.explicitEventParam) args = eventArg;   // 引数に書いていなければ、最初に e を渡す
            }
            for (size_t i = 0; i < count; ++i) {
                if (!args.empty()) args += ", ";
                // 「ScannedRobotEvent e」の引数には、穴の中ではなく e を渡す（穴は空のままでよい）
                std::string t, n;
                const json& pv = state->at("params").at(i);
                splitTypedName(pv.is_string() ? pv.get<std::string>() : "", t, n);
                if (eventOfClass(t)) {
                    args += eventArg.empty() ? "e" : eventArg;
                    continue;
                }
                std::string input = "ARG" + std::to_string(i);
                args += valueOfInput(block, input.c_str());
            }
            return functionJavaName(name) + "(" + args + ")";
        }
        bool insideFunction = false;   // 今、関数の中を変換しているか

        // 関数を作るブロックを、Java のメソッドにする
        std::string renderFunction(const json& def) {
            bool returns = stringOf(def, "type") == "procedures_defreturn";
            const json* nameField = fieldOf(def, "NAME");
            std::string name = (nameField && nameField->is_string()) ? nameField->get<std::string>() : "";
            std::string params;
            // イベントの情報を使う関数は、最初に e を受け取る
            const FunctionInfo& info = functionInfos[name];
            currentEvent = info.event;
            if (info.needsEvent && info.event && !info.explicitEventParam) {
                std::string cls, prm;
                if (eventParameter(*info.event, cls, prm)) params = cls + " e";
            }
            auto st = def.find("extraState");
            if (st != def.end() && st->is_object() && st->contains("params") && (*st)["params"].is_array()) {
                for (const auto& prm : (*st)["params"]) {
                    if (!params.empty()) params += ", ";
                    auto vt = variableJavaNames.find(stringOf(prm, "id"));
                    // 「ScannedRobotEvent e」なら、そのまま e として受け取る
                    if (eventOfClass(typeOfName(stringOf(prm, "name")))) {
                        params += typeOfName(stringOf(prm, "name")) + " e";
                        continue;
                    }
                    // 引数の型：「int 回数」なら int（型が書かれていなければ double）
                    params += typeOfName(stringOf(prm, "name")) + " " +
                              (vt != variableJavaNames.end() ? vt->second : variableJavaName(stringOf(prm, "name")));
                }
            }
            insideReturnFunction = returns;
            insideFunction = true;
            std::string body = statementChain(inputOf(def, "STACK"));
            if (returns) {
                std::string value = valueOfInput(def, "RETURN");
                body += (body.empty() ? "" : "\n") + std::string("return ") + value + ";";
            }
            insideReturnFunction = false;
            insideFunction = false;
            currentEvent = nullptr;
            // 返す型：「int 数える」なら int（型が書かれていなければ double）。値を返さない関数は void
            std::string typeName, plainName;
            bool typed = splitTypedName(name, typeName, plainName);
            if (typed && !returns) error(&def, nullptr, "関数「" + name + "」：値を返さない関数の名前には、型を書けません");
            std::string out = "private " + (returns ? typeName : std::string("void")) + " " + functionJavaName(name) + "(" + params + ") {\n";
            for (const std::string& line : splitLines(body)) out += (line.empty() ? "" : "    " + line) + "\n";
            return out + "}";
        }

        // 穴の中（block）を値として Java にする。穴が空なら 0
        std::string valueOfInput(const json& block, const char* inputName) {
            const json* child = inputOf(block, inputName);
            if (!child || isDisabled(*child)) {
                error(&block, nullptr, "穴にブロックが入っていません");
                return "0";
            }
            return valueOfBlock(*child);
        }

        std::string valueOfBlock(const json& child) {
            std::string type = stringOf(child, "type");
            if (type == "variables_get") return variableOf(child);
            if (type == "math_number") return formatNumber(numberFrom(fieldOf(child, "NUM"), 0));
            if (type == "procedures_callreturn") return functionCall(child);
            const BlockDef::Info* childDef = find(type);
            if (!childDef) {
                error(&child, nullptr, "知らないブロックです（" + type + "）");
                return "0";
            }
            if (!childDef->isOutput) {
                error(&child, childDef, "このブロックは穴にははめられません");
                return "0";
            }
            return render(*childDef, child);
        }

        std::string builtinStatement(const json& block) {
            std::string type = stringOf(block, "type");
            if (type == "procedures_callnoreturn") return functionCall(block) + ";";
            if (type == "procedures_ifreturn") {
                std::string cond = valueOfInput(block, "CONDITION");
                if (insideReturnFunction) return "if (" + cond + ") {\n    return " + valueOfInput(block, "VALUE") + ";\n}";
                return "if (" + cond + ") {\n    return;\n}";
            }
            std::string var = variableOf(block);
            if (type == "variables_set") return var + " = " + valueOfInput(block, "VALUE") + ";";
            return var + " += " + valueOfInput(block, "DELTA") + ";";
        }

        // つながっている命令ブロックを順番に Java にする
        std::string statementChain(const json* first) {
            std::vector<std::string> parts;
            for (const json* b = first; b != nullptr; b = nextOf(*b)) {
                if (isDisabled(*b)) continue;
                std::string type = stringOf(*b, "type");
                if (isBuiltinStatement(type)) {   // 変数のブロック
                    parts.push_back(builtinStatement(*b));
                    continue;
                }
                const BlockDef::Info* def = find(type);
                if (!def) {
                    error(b, nullptr, isBuiltinValue(type) ? "このブロックはここには置けません"
                                                           : "知らないブロックです（" + type + "）");
                    continue;
                }
                if (def->isEvent || def->isOutput) {
                    error(b, def, "このブロックはここには置けません");
                    continue;
                }
                // 変数宣言ブロック：static の変数はクラス直下に移す（ふつうの変数はその場に残す）
                // .classMember() のブロック（内部クラスやフィールド）は、置いた場所ではなくクラスの中に出す
                if (def->isClassMember) {
                    addClassMember(render(*def, *b));
                    continue;
                }
                if (type == "robot_variable_declare") {
                    std::string code = render(*def, *b);
                    if ((!getBlockText(*b, *def, "ACCESS").empty() ||
                        getBlockText(*b, *def, "STATIC") == "static") && classMembers) {
                        *classMembers += "    " + code + "\n";
                    }
                    else if (!code.empty()) {
                        parts.push_back(code);
                    }
                    continue;
                }
                if (type == kReturnBlock) {
                    const json* next = nextOf(*b);
                    while (next && isDisabled(*next)) next = nextOf(*next);
                    if (next) error(b, def, "このブロックの下には、ブロックをつなげられません（ここで終わるので、下は動きません）");
                    if (insideReturnFunction) error(b, def, "値を返す関数の中では使えません。「もし〇なら値を返す」を使ってください");
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
            return valueOfBlock(*child);
        }

        // 口の中の命令ブロックを Java にする
        //   （static の変数宣言だけが入っているときは、中身が空でも注意を出さない）
        std::string statementInput(const json& block, const BlockDef::Info& def, const BlockDef::Arg& arg) {
            const json* first = inputOf(block, arg.name);
            if (!first || isDisabled(*first)) warning(&block, &def, "中にブロックが入っていません");
            return statementChain(first);
        }

        // クラスの中（メソッドの外）に Java を足す。何行あっても、全部の行を字下げする
        void addClassMember(const std::string& code) {
            if (!classMembers || code.empty()) return;
            for (const std::string& line : splitLines(code)) *classMembers += (line.empty() ? "" : "    " + line) + "\n";
        }

        // 選ぶ欄で選ばれている値（Java に入る文字）
        std::string getBlockText(const json& block, const BlockDef::Info& def, const std::string& name) {
            const BlockDef::Arg* arg = def.findArg(name);
            if (!arg || arg->kind != BlockDef::Arg::Dropdown) {
                mistake(def, "に「" + name + "」という選択欄がありません");
                return "";
            }
            const json* field = fieldOf(block, name);
            if (field && field->is_string()) return field->get<std::string>();
            return arg->options.empty() ? "" : arg->options[0].second;
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
                    //見つかった接続口が、命令を入れる種類かどうかを確認
                    if (arg && arg->kind == BlockDef::Arg::Statements) {
                        std::string indent = line.substr(0, line.find_first_not_of(" \t"));
                        // 接続された命令からJavaコードを取得する
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
        case BlockDef::Arg::OptionalValue: {
            const json* child = inputOf(*impl_->block, name);
            if (!child || isDisabled(*child)) return "";
            return impl_->conv->valueOfBlock(*child);
        }
        case BlockDef::Arg::Statements:
            return impl_->conv->statementInput(*impl_->block, def, *arg);
        case BlockDef::Arg::Text: {
            const json* f = fieldOf(*impl_->block, name);
            return (f && f->is_string()) ? f->get<std::string>() : arg->initialText;
        }
        case BlockDef::Arg::Variable: {   // 変数を選ぶ欄 → その変数の Java の名前
            const json* f = fieldOf(*impl_->block, name);
            if (f && f->is_object()) {
                auto it = impl_->conv->variableJavaNames.find(stringOf(*f, "id"));
                if (it != impl_->conv->variableJavaNames.end()) return it->second;
                std::string shown = stringOf(*f, "name");
                if (!shown.empty()) return variableJavaName(shown);
            }
            if (f && f->is_string()) return f->get<std::string>();
            return arg->initialText;
        }
        }
        return "";
    }

    bool BlockData::hasConnectedValue(const std::string& name) {
        const json* inputs = impl_->block->find("inputs") != impl_->block->end()
            ? &*impl_->block->find("inputs") : nullptr;
        if (!inputs || !inputs->is_object()) return false;
        auto it = inputs->find(name);
        if (it == inputs->end() || !it->is_object()) return false;
        auto blockIt = it->find("block");
        // Blockly の shadow（最初から入っている既定値）は「接続した値」とみなさない。
        return blockIt != it->end() && blockIt->is_object();
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

    BlockDef& BlockDef::optionalValueInput(const std::string& name, const std::string& type) {
        Arg arg;
        arg.kind = Arg::OptionalValue;
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

    BlockDef& BlockDef::textField(const std::string& name, const std::string& initial) {
        Arg arg;
        arg.kind = Arg::Text;
        arg.name = name;
        arg.initialText = initial;
        info.args.push_back(arg);
        return *this;
    }

    BlockDef& BlockDef::variableField(const std::string& name, const std::string& initial) {
        Arg arg;
        arg.kind = Arg::Variable;
        arg.name = name;
        arg.initialText = initial;
        info.args.push_back(arg);
        return *this;
    }

    BlockDef& BlockDef::output(const std::string& type) {
        info.isOutput = true;
        info.outputType = type;
        return *this;
    }

    BlockDef& BlockDef::event() { info.isEvent = true; return *this; }
    BlockDef& BlockDef::classMember() { info.isClassMember = true; return *this; }
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
    std::vector<const BlockDef::Info*> detail::RegisteredBlocks() {
        ensureLoaded();
        std::vector<const BlockDef::Info*> list;
        std::set<std::string> added;
        for (const auto& p : registry().blocks) {
            if (p->info.type.empty() || !added.insert(p->info.type).second) continue;
            list.push_back(&p->info);
        }
        return list;
    }

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

            // 変数（「変数を作る」で作ったもの）を集めて、Java の名前を決める
            std::vector<std::string> fieldNames;
            std::set<std::string> usedNames;
            auto vars = ws.find("variables");
            if (vars != ws.end() && vars->is_array()) {
                for (const auto& v : *vars) {
                    if (!v.is_object()) continue;
                    std::string javaName = variableJavaName(stringOf(v, "name"));
                    std::string unique = javaName;
                    for (int n = 2; !usedNames.insert(unique).second; ++n) unique = javaName + "_" + std::to_string(n);
                    conv.variableJavaNames[stringOf(v, "id")] = unique;
                    // 型を書いた変数（「int 回数」など）は関数の引数なので、クラスの変数は作らない
                    std::string typeName, plainName;
                    if (splitTypedName(stringOf(v, "name"), typeName, plainName)) continue;
                    fieldNames.push_back(unique);
                }
            }

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

            // クラス直下に出力する変数宣言（static の変数の Java をためておく）
            std::string classMembers;
            conv.classMembers = &classMembers;

            // イベントブロックを選び出す
            std::vector<std::pair<const BlockDef::Info*, const json*>> events;
            std::vector<const json*> functions;   // 「関数」カテゴリで作った関数
            std::set<std::string> seen;
            for (const json* b : tops) {
                if (isDisabled(*b)) continue;
                std::string type = stringOf(*b, "type");
                if (conv.isFunctionDefinition(type)) {
                    functions.push_back(b);
                    continue;
                }
                const BlockDef::Info* def = conv.find(type);
                if (!def) {
                    conv.error(b, nullptr, "知らないブロックです（" + type + "）");
                }
                else if (def->isClassMember) {
                    // 一番上に置いた .classMember() のブロック：下につながっているものもまとめてクラスの中に出す
                    for (const json* m = b; m != nullptr; m = nextOf(*m)) {
                        if (isDisabled(*m)) continue;
                        const BlockDef::Info* mdef = conv.find(stringOf(*m, "type"));
                        if (!mdef) { conv.error(m, nullptr, "知らないブロックです（" + stringOf(*m, "type") + "）"); continue; }
                        if (!mdef->isClassMember) { conv.error(m, mdef, "このブロックは、クラスの中に置くブロックの下にはつなげられません"); continue; }
                        conv.addClassMember(conv.render(*mdef, *m));
                    }
                }
                else if (type == "robot_variable_declare") {
                    // 一番上に置いた変数宣言：static ならクラス直下に出す
                    std::string declaration = conv.render(*def, *b);
                    if (!conv.getBlockText(*b, *def, "ACCESS").empty() ||
                        conv.getBlockText(*b, *def, "STATIC") == "static") {

                        classMembers += "    " + declaration + "\n";
                    }
                    else {
                        conv.warning(b, def, "通常の変数宣言はイベント内に置いてください");
                    }
                }
                else if (!def->isEvent) {
                    conv.warning(b, def, "どのイベントにもつながっていないので、動きません");
                }
                else if (type != kRawMemberBlock && !seen.insert(type).second) {   // そのまま書くブロックは何個でも置ける
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

            // 関数がイベントの情報（e）を使うか調べる
            conv.analyzeFunctions(events, functions);

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

            // 関数ごとに Java のメソッドを作る（イベントのあとに並べる）
            for (const json* f : functions) {
                std::string method = conv.renderFunction(*f);
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
                << "import java.awt.Color;\n"
                << "import java.util.ArrayList;\n"
                << "import java.util.List;\n"
                << "import static robocode.util.Utils.normalRelativeAngleDegrees;\n"
                << "\n"
                << "// Generated by Robocode Blocks\n"
                << "public class " << name << " extends AdvancedRobot {\n";
            if (!classMembers.empty()) java << "\n" << classMembers;   // static の変数・クラスの中に置くブロック
            // 「記憶 A／B／C」のブロック（ControlPlus）を使っていたら、その入れ物を用意する
            if (workspaceJson.find("\"memory_") != std::string::npos) {
                java << "\n    double memA = 0, memB = 0, memC = 0;   // 記憶 A・B・C（ラウンドが変わると 0 にもどる）\n";
            }
            if (!fieldNames.empty()) {
                java << "\n";
                java << "    // 「変数」カテゴリで作った変数（ラウンドが変わると 0 にもどる）\n";
                for (const auto& f : fieldNames) java << "    double " << f << " = 0;\n";
            }
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