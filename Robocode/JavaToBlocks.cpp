// =====================================================================
//  JavaToBlocks.cpp  ―  Java（.java ファイル）をブロックにもどす
// =====================================================================
//
//  ブロックを作る人は読まなくてOK。ここでやっていること：
//    1. Java を「単語（トークン）」に分ける（コメントは取りのぞく）
//    2. class の中を、メソッドごとに分ける
//    3. メソッドの中を、文（ahead(100); や if (…) { … }）ごとに分ける
//    4. 各ブロックの .java("…") を「型紙」にして、当てはまるブロックをさがす
//         例：.java("ahead({DIST});") → 「ahead ( 何か ) ;」という型紙
//             ahead(100); → 何か = 100 → 「前に 100 進む」
//    5. 当てはまらないところは「Java をそのまま書く」ブロックにする
//
//  新しくブロックを追加すると、そのブロックの .java("…") も自動で型紙になる。
//  （もどすための設定を別に書く必要はない）
//
// =====================================================================
#include "RobocodeBlocks.h"

#include <nlohmann/json.hpp>  // json.hpp の場所が違うときは、この1行だけ直す

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <locale>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

using json = nlohmann::json;

namespace rb {
namespace {

using Info = BlockDef::Info;
using Arg = BlockDef::Arg;

const char* const kNumberBlock = "rb_number";
const char* const kRawStatement = "java_raw_statement";
const char* const kRawExpression = "java_raw_expression";
const char* const kRawMember = "java_raw_member";

// ---------------------------------------------------------------------
//  1. Java を単語（トークン）に分ける
// ---------------------------------------------------------------------
enum class TokKind { Ident, Number, String, Op };

struct Tok {
    TokKind kind;
    std::string s;
};
using Toks = std::vector<Tok>;

bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$' || (static_cast<unsigned char>(c) >= 0x80); }
bool isIdentChar(char c) { return isIdentStart(c) || std::isdigit(static_cast<unsigned char>(c)); }

// 2文字以上の記号（長いものから順に調べる）
const char* const kMultiOps[] = {
    ">>>=", "<<=", ">>=", ">>>", "==", "!=", "<=", ">=", "&&", "||", "++", "--",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "::", "<<", ">>",
};

// src[i] から始まる、文字列・文字のかたまりの終わりをさがす
size_t skipQuoted(const std::string& src, size_t i) {
    char q = src[i];
    size_t j = i + 1;
    while (j < src.size() && src[j] != q) {
        if (src[j] == '\\') ++j;
        ++j;
    }
    return std::min(j + 1, src.size());
}

// 数字のかたまり（100, 0.5, 1e3, 2.0f, 0xFF など）の終わり
size_t skipNumber(const std::string& src, size_t i) {
    size_t j = i;
    if (src[j] == '0' && j + 1 < src.size() && (src[j + 1] == 'x' || src[j + 1] == 'X')) {
        j += 2;
        while (j < src.size() && std::isxdigit(static_cast<unsigned char>(src[j]))) ++j;
    } else {
        while (j < src.size() && (std::isdigit(static_cast<unsigned char>(src[j])) || src[j] == '.' || src[j] == '_')) ++j;
        if (j < src.size() && (src[j] == 'e' || src[j] == 'E')) {
            ++j;
            if (j < src.size() && (src[j] == '+' || src[j] == '-')) ++j;
            while (j < src.size() && std::isdigit(static_cast<unsigned char>(src[j]))) ++j;
        }
    }
    while (j < src.size() && std::strchr("fFdDlL", src[j]) && src[j] != '\0') ++j;
    return j;
}

Toks tokenize(const std::string& src) {
    Toks out;
    size_t i = 0;
    while (i < src.size()) {
        char c = src[i];
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        // コメント
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '*') {
            size_t end = src.find("*/", i + 2);
            i = (end == std::string::npos) ? src.size() : end + 2;
            continue;
        }
        if (c == '"' || c == '\'') {
            size_t j = skipQuoted(src, i);
            out.push_back({TokKind::String, src.substr(i, j - i)});
            i = j;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '.' && i + 1 < src.size() && std::isdigit(static_cast<unsigned char>(src[i + 1])))) {
            size_t j = skipNumber(src, i);
            out.push_back({TokKind::Number, src.substr(i, j - i)});
            i = j;
            continue;
        }
        if (isIdentStart(c)) {
            size_t j = i;
            while (j < src.size() && isIdentChar(src[j])) ++j;
            out.push_back({TokKind::Ident, src.substr(i, j - i)});
            i = j;
            continue;
        }
        bool matched = false;
        for (const char* op : kMultiOps) {
            size_t n = std::strlen(op);
            if (src.compare(i, n, op) == 0) {
                out.push_back({TokKind::Op, op});
                i += n;
                matched = true;
                break;
            }
        }
        if (!matched) {
            out.push_back({TokKind::Op, std::string(1, c)});
            ++i;
        }
    }
    // this.ahead(100) → ahead(100) のように、this. は取りのぞく
    Toks cleaned;
    for (size_t k = 0; k < out.size(); ++k) {
        if (out[k].kind == TokKind::Ident && out[k].s == "this" && k + 2 < out.size() &&
            out[k + 1].s == "." && out[k + 2].kind == TokKind::Ident) {
            ++k;  // this と . をとばす
            continue;
        }
        cleaned.push_back(out[k]);
    }
    // robocode.util.Utils.normalRelativeAngleDegrees(…) と normalRelativeAngleDegrees(…)（import static したもの）を
    // 同じものとして読む（Java にするときは import static が付いているので、短いほうにそろえる）
    Toks shortened;
    for (size_t k = 0; k < cleaned.size(); ++k) {
        auto is = [&](size_t at, const char* s) { return at < cleaned.size() && cleaned[at].s == s; };
        if (is(k, "robocode") && is(k + 1, ".") && is(k + 2, "util") && is(k + 3, ".") && is(k + 4, "Utils") &&
            is(k + 5, ".") && is(k + 6, "normalRelativeAngleDegrees")) {
            k += 5;
            continue;
        }
        if (is(k, "Utils") && is(k + 1, ".") && is(k + 2, "normalRelativeAngleDegrees") && !(k > 0 && is(k - 1, "."))) {
            k += 1;
            continue;
        }
        shortened.push_back(cleaned[k]);
    }
    return shortened;
}

bool isOpen(const std::string& s) { return s == "(" || s == "[" || s == "{"; }
bool isClose(const std::string& s) { return s == ")" || s == "]" || s == "}"; }

// toks[i] が開きかっこのとき、対応する閉じかっこの位置（なければ toks.size()）
size_t matchBracket(const Toks& toks, size_t i) {
    int depth = 0;
    for (size_t j = i; j < toks.size(); ++j) {
        if (toks[j].kind == TokKind::Op && isOpen(toks[j].s)) ++depth;
        else if (toks[j].kind == TokKind::Op && isClose(toks[j].s)) {
            if (--depth == 0) return j;
        }
    }
    return toks.size();
}

// トークンを、読みやすい Java の文字にもどす（「そのまま書く」ブロック用）
std::string joinTokens(const Toks& toks, size_t from, size_t to) {
    static const std::set<std::string> spaced = {
        "=", "==", "!=", "<", ">", "<=", ">=", "+", "-", "*", "/", "%", "&&", "||", "?", ":",
        "+=", "-=", "*=", "/=", "%=", "&", "|", "^", "->", "<<", ">>", ">>>"};
    std::string s;
    for (size_t k = from; k < to; ++k) {
        const Tok& t = toks[k];
        if (!s.empty()) {
            const Tok& p = toks[k - 1];
            bool space = false;
            if (p.kind != TokKind::Op && t.kind != TokKind::Op) space = true;               // 単語と単語
            if (t.kind == TokKind::Op && spaced.count(t.s)) space = true;                    // 演算子の前
            if (p.kind == TokKind::Op && spaced.count(p.s)) space = true;                    // 演算子のあと
            if (p.s == "," || p.s == ";" || p.s == "{" || p.s == "}") space = true;
            if (t.s == "{" || t.s == "}") space = true;
            if (t.s == "(" && p.kind == TokKind::Ident &&
                (p.s == "if" || p.s == "while" || p.s == "for" || p.s == "switch" || p.s == "catch")) space = true;
            if (p.s == ")" && t.kind != TokKind::Op) space = true;
            if (t.s == ";" || t.s == "," || t.s == ")" || t.s == "]" || t.s == ".") space = false;
            if (p.s == "(" || p.s == "[" || p.s == ".") space = false;
            if (t.s == "}" || p.s == "{") space = true;
            if (space) s += ' ';
        }
        s += t.s;
    }
    return s;
}

// ---------------------------------------------------------------------
//  2・3. 文やメソッドに分ける
// ---------------------------------------------------------------------

// 文の中身が { } で囲まれていなければ、囲む（if (a) b; → if (a) { b; }）
void appendWrapped(Toks& out, const Toks& stmt) {
    if (!stmt.empty() && stmt.front().s == "{") {
        out.insert(out.end(), stmt.begin(), stmt.end());
    } else {
        out.push_back({TokKind::Op, "{"});
        out.insert(out.end(), stmt.begin(), stmt.end());
        out.push_back({TokKind::Op, "}"});
    }
}

// toks[i] から始まる文を1つ読む。読み終わった位置を返し、out に（整えた）文を入れる
size_t readStatement(const Toks& toks, size_t i, size_t end, Toks& out) {
    out.clear();
    if (i >= end) return end;
    const std::string& first = toks[i].s;

    // { … } のかたまり
    if (first == "{") {
        size_t j = std::min(matchBracket(toks, i), end - 1);
        out.assign(toks.begin() + i, toks.begin() + j + 1);
        return j + 1;
    }

    // if / while / for：中身を { } で囲んで整える
    if (toks[i].kind == TokKind::Ident && (first == "if" || first == "while" || first == "for") &&
        i + 1 < end && toks[i + 1].s == "(") {
        size_t close = std::min(matchBracket(toks, i + 1), end - 1);
        out.assign(toks.begin() + i, toks.begin() + close + 1);  // if ( … )
        Toks body;
        size_t next = readStatement(toks, close + 1, end, body);
        appendWrapped(out, body);
        if (first == "if" && next < end && toks[next].s == "else") {
            out.push_back(toks[next]);
            Toks elseBody;
            next = readStatement(toks, next + 1, end, elseBody);
            appendWrapped(out, elseBody);
        }
        return next;
    }

    // do { … } while ( … );
    if (first == "do") {
        Toks body;
        size_t next = readStatement(toks, i + 1, end, body);
        out.push_back(toks[i]);
        appendWrapped(out, body);
        while (next < end) {
            out.push_back(toks[next]);
            if (toks[next].s == ";") return next + 1;
            ++next;
        }
        return next;
    }

    // try { … } catch ( … ) { … } finally { … }
    if (first == "try") {
        size_t j = i + 1;
        out.push_back(toks[i]);
        while (j < end) {
            if (toks[j].s == "(" || toks[j].s == "{") {
                size_t close = std::min(matchBracket(toks, j), end - 1);
                out.insert(out.end(), toks.begin() + j, toks.begin() + close + 1);
                j = close + 1;
                if (toks[close].s == "}" && !(j < end && (toks[j].s == "catch" || toks[j].s == "finally"))) return j;
            } else {
                out.push_back(toks[j]);
                ++j;
            }
        }
        return j;
    }

    // それ以外：; まで（かっこの中の ; は数えない）
    int depth = 0;
    size_t j = i;
    for (; j < end; ++j) {
        const std::string& s = toks[j].s;
        if (toks[j].kind == TokKind::Op && isOpen(s)) ++depth;
        if (toks[j].kind == TokKind::Op && isClose(s)) {
            if (depth == 0) break;  // 外側の } に来た（; が足りない）
            --depth;
            if (depth == 0 && s == "}" && j + 1 < end && toks[j + 1].s != ";" && toks[j + 1].s != ")" &&
                toks[j + 1].s != "," && toks[j + 1].s != ".") {
                // switch ( … ) { … } のように ; なしで終わる文
                out.assign(toks.begin() + i, toks.begin() + j + 1);
                return j + 1;
            }
        }
        if (depth == 0 && s == ";") {
            out.assign(toks.begin() + i, toks.begin() + j + 1);
            return j + 1;
        }
    }
    out.assign(toks.begin() + i, toks.begin() + j);
    return j == i ? j + 1 : j;
}

std::vector<Toks> splitStatements(const Toks& toks, size_t from, size_t to) {
    std::vector<Toks> list;
    size_t i = from;
    while (i < to) {
        if (toks[i].s == ";") { ++i; continue; }  // 空の文
        Toks stmt;
        size_t next = readStatement(toks, i, to, stmt);
        if (!stmt.empty()) list.push_back(stmt);
        i = std::max(next, i + 1);
    }
    return list;
}

// class の中の1つ分（メソッドか変数）
std::vector<Toks> splitMembers(const Toks& toks, size_t from, size_t to) {
    std::vector<Toks> list;
    size_t i = from;
    while (i < to) {
        size_t start = i;
        int depth = 0;
        for (; i < to; ++i) {
            const std::string& s = toks[i].s;
            if (s == "(" || s == "[") ++depth;
            if (s == ")" || s == "]") --depth;
            if (depth == 0 && s == ";") { ++i; break; }
            if (depth == 0 && s == "{") {
                i = std::min(matchBracket(toks, i), to - 1) + 1;
                break;
            }
        }
        if (i > start) list.emplace_back(toks.begin() + start, toks.begin() + i);
    }
    return list;
}

// ---------------------------------------------------------------------
//  4. 型紙（ブロックの .java("…")）
// ---------------------------------------------------------------------
enum class PieceKind { Literal, Hole, IdentHole, Compound };

struct Piece {
    PieceKind kind;
    std::string text;           // Literal のとき
    int arg = -1;               // Hole / IdentHole のとき：何番目の入力欄か
    std::string prefix, suffix; // IdentHole のとき：get{AXIS}() の get と空
    // Compound のとき：1つの単語の中に選ぶ欄が2つ以上あるもの（{PART}{DIRECTION} → turnGunRight など）
    //   partArgs[i] が -1 なら partTexts[i] の文字、そうでなければその番号の選ぶ欄
    std::vector<int> partArgs;
    std::vector<std::string> partTexts;
};

// Compound の単語 s を、決まった文字と選ぶ欄の値に分ける（できたら values[入力欄の番号] に値が入る）
//   例：{PART}{DIRECTION} と turnGunRight → PART = turnGun, DIRECTION = Right
bool splitCompound(const Piece& piece, const Info& info, const std::string& s, size_t pos, size_t part,
                   std::map<int, std::string>& values) {
    if (part == piece.partArgs.size()) return pos == s.size();
    int a = piece.partArgs[part];
    if (a < 0) {
        const std::string& lit = piece.partTexts[part];
        if (s.compare(pos, lit.size(), lit) != 0) return false;
        return splitCompound(piece, info, s, pos + lit.size(), part + 1, values);
    }
    const Arg& arg = info.args[a];
    if (arg.kind != Arg::Dropdown) return false;   // 選ぶ欄だけ（どこで切るかが決まるため）
    for (const auto& o : arg.options) {
        const std::string& v = o.second;
        if (v.empty() || s.compare(pos, v.size(), v) != 0) continue;
        values[a] = v;
        if (splitCompound(piece, info, s, pos + v.size(), part + 1, values)) return true;
    }
    values.erase(a);
    return false;
}

struct Pattern {
    const Info* info = nullptr;
    std::vector<Piece> pieces;
    int literalCount = 0;
    int statementCount = 1;  // 型紙に文がいくつ入っているか（ahead(100); back(100); なら 2）
};

// 型紙の {名前} の名前が入力欄の名前なら、その番号
int argIndex(const Info& info, const std::string& name) {
    for (size_t k = 0; k < info.args.size(); ++k) {
        if (info.args[k].name == name) return static_cast<int>(k);
    }
    return -1;
}

// src[i] が {名前} のとき、その終わりの位置と入力欄の番号
bool readHole(const Info& info, const std::string& src, size_t i, size_t& endPos, int& arg) {
    if (src[i] != '{') return false;
    size_t j = i + 1;
    while (j < src.size() && isIdentChar(src[j])) ++j;
    if (j >= src.size() || src[j] != '}' || j == i + 1) return false;
    arg = argIndex(info, src.substr(i + 1, j - i - 1));
    if (arg < 0) return false;
    endPos = j + 1;
    return true;
}

// 型紙の中のコメント（// … や /* … */）を取りのぞく
std::string stripComments(const std::string& src) {
    std::string out;
    size_t i = 0;
    while (i < src.size()) {
        if (src[i] == '"' || src[i] == '\'') {
            size_t j = skipQuoted(src, i);
            out += src.substr(i, j - i);
            i = j;
        } else if (src.compare(i, 2, "//") == 0) {
            while (i < src.size() && src[i] != '\n') ++i;
        } else if (src.compare(i, 2, "/*") == 0) {
            size_t end = src.find("*/", i + 2);
            i = (end == std::string::npos) ? src.size() : end + 2;
            out += ' ';
        } else {
            out += src[i++];
        }
    }
    return out;
}

bool buildPattern(const Info& info, Pattern& p) {
    p.info = &info;
    const std::string src = stripComments(info.javaTemplate);
    size_t i = 0;
    while (i < src.size()) {
        char c = src[i];
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        size_t holeEnd = 0;
        int arg = -1;
        // 単語の一部に {名前} が入っているもの（get{AXIS}() など）や、{名前} だけのもの
        if (isIdentStart(c) || readHole(info, src, i, holeEnd, arg)) {
            std::string prefix, suffix;
            int holeArg = -1;
            size_t j = i;
            // 単語の中を、決まった文字と {名前} に分けていく（{名前} が2つ以上あってもよい）
            std::vector<int> partArgs;
            std::vector<std::string> partTexts;
            std::string current;
            int holeCount = 0;
            while (j < src.size()) {
                size_t e = 0;
                int a = -1;
                if (readHole(info, src, j, e, a)) {
                    if (!current.empty()) { partArgs.push_back(-1); partTexts.push_back(current); current.clear(); }
                    partArgs.push_back(a);
                    partTexts.push_back("");
                    if (holeArg < 0) holeArg = a;
                    ++holeCount;
                    j = e;
                } else if (isIdentChar(src[j])) {
                    (holeCount == 0 ? prefix : suffix) += src[j];
                    current += src[j];
                    ++j;
                } else {
                    break;
                }
            }
            if (!current.empty()) { partArgs.push_back(-1); partTexts.push_back(current); }
            if (holeCount >= 2) {
                Piece piece{PieceKind::Compound, "", holeArg};
                piece.partArgs = partArgs;
                piece.partTexts = partTexts;
                p.pieces.push_back(piece);
                i = j;
                continue;
            }
            if (holeArg < 0) {
                // ふつうの単語（数字で始まるときは数字）
                Piece piece{PieceKind::Literal, prefix};
                p.pieces.push_back(piece);
            } else if (prefix.empty() && suffix.empty()) {
                Piece piece{PieceKind::Hole, "", holeArg};
                p.pieces.push_back(piece);
            } else {
                Piece piece{PieceKind::IdentHole, "", holeArg, prefix, suffix};
                p.pieces.push_back(piece);
            }
            i = j;
            continue;
        }
        // 文字列・数字は、ひとかたまりにする
        if (c == '"' || c == '\'') {
            size_t j = skipQuoted(src, i);
            p.pieces.push_back({PieceKind::Literal, src.substr(i, j - i)});
            i = j;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t j = skipNumber(src, i);
            p.pieces.push_back({PieceKind::Literal, src.substr(i, j - i)});
            i = j;
            continue;
        }
        // 記号
        bool matched = false;
        for (const char* op : kMultiOps) {
            size_t n = std::strlen(op);
            if (src.compare(i, n, op) == 0) {
                p.pieces.push_back({PieceKind::Literal, op});
                i += n;
                matched = true;
                break;
            }
        }
        if (!matched) {
            p.pieces.push_back({PieceKind::Literal, std::string(1, c)});
            ++i;
        }
    }
    // robocode.util.Utils.normalRelativeAngleDegrees → normalRelativeAngleDegrees（Java を読むときと同じにする）
    {
        std::vector<Piece> shortened;
        auto lit = [&](size_t at, const char* s) {
            return at < p.pieces.size() && p.pieces[at].kind == PieceKind::Literal && p.pieces[at].text == s;
        };
        for (size_t k = 0; k < p.pieces.size(); ++k) {
            if (lit(k, "robocode") && lit(k + 1, ".") && lit(k + 2, "util") && lit(k + 3, ".") && lit(k + 4, "Utils") &&
                lit(k + 5, ".") && lit(k + 6, "normalRelativeAngleDegrees")) {
                k += 5;
                continue;
            }
            if (lit(k, "Utils") && lit(k + 1, ".") && lit(k + 2, "normalRelativeAngleDegrees") && !(k > 0 && lit(k - 1, "."))) {
                k += 1;
                continue;
            }
            shortened.push_back(p.pieces[k]);
        }
        p.pieces = shortened;
    }
    int depth = 0;
    p.statementCount = 0;
    for (const auto& piece : p.pieces) {
        if (piece.kind != PieceKind::Literal) continue;
        ++p.literalCount;
        if (isOpen(piece.text)) ++depth;
        if (isClose(piece.text)) --depth;
        if (depth == 0 && piece.text == ";") ++p.statementCount;  // 一番外側の ; の数 = 文の数
    }
    if (p.statementCount < 1) p.statementCount = 1;
    // 型紙が {NUM} だけ、のようなものは何にでも当てはまってしまうので使わない
    if (p.pieces.size() == 1 && p.pieces[0].kind == PieceKind::Hole) {
        Arg::Kind k = info.args[p.pieces[0].arg].kind;
        if (k != Arg::Dropdown) return false;
    }
    return !p.pieces.empty();
}

// ---------------------------------------------------------------------
//  型紙と Java を見比べる
// ---------------------------------------------------------------------
using Captures = std::vector<std::pair<size_t, size_t>>;  // 入力欄ごとの [始まり, 終わり)

const std::set<std::string>& binaryOps() {
    static const std::set<std::string> ops = {
        "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "&&", "||", "&", "|", "^",
        "?", ":", "<<", ">>", ">>>", "instanceof", "=", "+=", "-=", "*=", "/=", "%="};
    return ops;
}

bool parseNumberLiteral(const Toks& toks, size_t from, size_t to, double& value) {
    bool negative = false;
    if (to - from == 2 && toks[from].s == "-") { negative = true; ++from; }
    if (to - from != 1 || toks[from].kind != TokKind::Number) return false;
    std::string s = toks[from].s;
    while (!s.empty() && std::strchr("fFdDlL", s.back())) s.pop_back();
    s.erase(std::remove(s.begin(), s.end(), '_'), s.end());
    std::istringstream is(s);
    is.imbue(std::locale::classic());
    double v = 0;
    if (s.size() > 1 && (s[1] == 'x' || s[1] == 'X')) {
        v = static_cast<double>(std::strtoll(s.c_str(), nullptr, 16));
    } else if (!(is >> v)) {
        return false;
    }
    value = negative ? -v : v;
    return true;
}

std::string removeSpaces(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (!std::isspace(static_cast<unsigned char>(c))) out += c;
    }
    return out;
}

// 名前の形か（名前 . 名前 … と、[ ] がつながったもの）
bool isNameLike(const Toks& toks, size_t from, size_t to) {
    if (to <= from || toks[from].kind != TokKind::Ident) return false;
    bool needName = false;   // . のあとは名前が要る
    for (size_t k = from; k < to; ++k) {
        const Tok& t = toks[k];
        if (t.kind == TokKind::Ident) { needName = false; continue; }
        if (t.s == "." && !needName) { needName = true; continue; }
        if (t.s == "[" && !needName) {   // [ ] の中は何でもよい（かっこがそろっていれば）
            size_t close = matchBracket(toks, k);
            if (close >= to || toks[close].s != "]") return false;
            k = close;
            continue;
        }
        return false;
    }
    return !needName;
}

// 入力欄に入ったものが、その入力欄に合っているか
bool captureFits(const Pattern& p, size_t pieceIndex, const Toks& toks, size_t from, size_t to) {
    const Piece& piece = p.pieces[pieceIndex];
    const Arg& arg = p.info->args[piece.arg];
    switch (arg.kind) {
    case Arg::Number: {
        double v = 0;
        if (!parseNumberLiteral(toks, from, to, v)) return false;
        if (arg.hasMin && v < arg.min) return false;
        if (arg.hasMax && v > arg.max) return false;
        return true;
    }
    case Arg::Dropdown: {
        std::string text = removeSpaces(joinTokens(toks, from, to));
        for (const auto& o : arg.options) {
            if (removeSpaces(o.second) == text) return true;
        }
        return false;
    }
    case Arg::Value: {
        if (to <= from) return false;
        // 一番外側に , があるものはだめ（f({A}, {B}) の A に「a, b」が入らないように）
        int depth = 0;
        for (size_t k = from; k < to; ++k) {
            if (toks[k].kind == TokKind::Op && isOpen(toks[k].s)) ++depth;
            else if (toks[k].kind == TokKind::Op && isClose(toks[k].s)) --depth;
            else if (depth == 0 && toks[k].s == ",") return false;
        }
        // 計算の順番が変わらないかは、全部当てはめたあとに checkOrder で確かめる
        (void)pieceIndex;
        (void)p;
        return true;
    }
    case Arg::Statements:
        return true;
    case Arg::Text:
        // 文字の欄（名前や型を自分で書く欄）には、名前の形のものだけを入れる
        //   OK：sin  /  int  /  double[]  /  Math.PI  /  scores[g]
        //   NG：a + b  /  max(a, b)  （計算や呼び出しを、名前の欄に入れてしまわないように）
        return isNameLike(toks, from, to);
    case Arg::Variable:   // 変数を選ぶ欄のある型紙は、Java からはもどさない
        return false;
    }
    return false;
}

// 演算子の強さ（大きいほど先に計算される。Java と同じ）
int precedence(const std::string& op) {
    if (op == "*" || op == "/" || op == "%") return 12;
    if (op == "+" || op == "-") return 11;
    if (op == "<<" || op == ">>" || op == ">>>") return 10;
    if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "instanceof") return 9;
    if (op == "==" || op == "!=") return 8;
    if (op == "&") return 7;
    if (op == "^") return 6;
    if (op == "|") return 5;
    if (op == "&&") return 4;
    if (op == "||") return 3;
    if (op == "?" || op == ":") return 2;
    if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") return 1;
    return -1;  // 演算子ではない
}

// 一番外側の演算子のうち、いちばん弱いものの強さ（演算子がなければ 100）
int weakestTopLevel(const Toks& toks, size_t from, size_t to) {
    int weakest = 100;
    int depth = 0;
    for (size_t k = from; k < to; ++k) {
        const std::string& s = toks[k].s;
        if (toks[k].kind == TokKind::Op && isOpen(s)) { ++depth; continue; }
        if (toks[k].kind == TokKind::Op && isClose(s)) { --depth; continue; }
        if (depth != 0 || k == from || !binaryOps().count(s)) continue;
        const Tok& prev = toks[k - 1];
        bool prevIsValue = prev.kind != TokKind::Op || isClose(prev.s) || prev.s == "++" || prev.s == "--";
        if (prevIsValue) weakest = std::min(weakest, precedence(s));
    }
    return weakest;
}

// 全部当てはめたあとに、計算の順番が変わってしまわないか確かめる
//   例：({A} {OP} {B}) に (a - b - c) を当てはめるとき
//       A = a - b, B = c なら OK（Java も (a - b) - c と計算する）
//       A = a, B = b - c はだめ（a - (b - c) になってしまう）
bool checkOrder(const Pattern& p, const Toks& toks, const Captures& caps) {
    static const std::set<std::string> separators = {"(", ")", ",", "[", "]", ";", "{", "}", "return"};
    // 型紙の k 番目が演算子なら、その強さ。区切りなら 0。わからなければ -1
    auto neighbor = [&](long k) -> int {
        if (k < 0 || k >= static_cast<long>(p.pieces.size())) return 0;
        const Piece& piece = p.pieces[k];
        std::string text;
        if (piece.kind == PieceKind::Literal) text = piece.text;
        else if (piece.kind == PieceKind::Hole && p.info->args[piece.arg].kind == Arg::Dropdown)
            text = removeSpaces(joinTokens(toks, caps[piece.arg].first, caps[piece.arg].second));
        if (separators.count(text)) return 0;
        int prec = precedence(text);
        return prec > 1 ? prec : (prec == 1 ? 0 : -1);  // = などの代入は区切りと同じ
    };
    for (size_t k = 0; k < p.pieces.size(); ++k) {
        const Piece& piece = p.pieces[k];
        if (piece.kind != PieceKind::Hole || p.info->args[piece.arg].kind != Arg::Value) continue;
        int weakest = weakestTopLevel(toks, caps[piece.arg].first, caps[piece.arg].second);
        if (weakest == 100) continue;  // 外側に演算子がない（1つの値）
        int left = neighbor(static_cast<long>(k) - 1);
        int right = neighbor(static_cast<long>(k) + 1);
        if (left < 0 || right < 0) return false;          // となりが何かわからないときは、安全のためだめ
        if (left > 0 && weakest <= left) return false;    // 左の演算子より強くないとだめ
        if (right > 0 && weakest < right) return false;   // 右の演算子と同じかより強ければ OK
    }
    return true;
}

bool matchFrom(const Pattern& p, size_t pi, const Toks& toks, size_t pos, size_t end, Captures& caps, int& budget) {
    if (--budget < 0) return false;  // 見比べすぎて時間がかかるときはあきらめる
    if (pi == p.pieces.size()) return pos == end && checkOrder(p, toks, caps);
    const Piece& piece = p.pieces[pi];

    if (piece.kind == PieceKind::Literal) {
        if (pos >= end || toks[pos].s != piece.text) return false;
        return matchFrom(p, pi + 1, toks, pos + 1, end, caps, budget);
    }

    if (piece.kind == PieceKind::Compound) {
        if (pos >= end || toks[pos].kind != TokKind::Ident) return false;
        std::map<int, std::string> values;
        if (!splitCompound(piece, *p.info, toks[pos].s, 0, 0, values)) return false;
        for (const auto& v : values) caps[v.first] = {pos, pos + 1};   // 単語まるごと。あとで中の部分だけ取り出す
        return matchFrom(p, pi + 1, toks, pos + 1, end, caps, budget);
    }

    if (piece.kind == PieceKind::IdentHole) {
        if (pos >= end || toks[pos].kind != TokKind::Ident) return false;
        const std::string& s = toks[pos].s;
        if (s.size() <= piece.prefix.size() + piece.suffix.size()) return false;
        if (s.compare(0, piece.prefix.size(), piece.prefix) != 0) return false;
        if (s.compare(s.size() - piece.suffix.size(), piece.suffix.size(), piece.suffix) != 0) return false;
        std::string middle = s.substr(piece.prefix.size(), s.size() - piece.prefix.size() - piece.suffix.size());
        const Arg& arg = p.info->args[piece.arg];
        if (arg.kind != Arg::Dropdown) return false;
        bool ok = false;
        for (const auto& o : arg.options) {
            if (o.second == middle) ok = true;
        }
        if (!ok) return false;
        caps[piece.arg] = {pos, pos + 1};  // 単語まるごと。あとで中の部分だけ取り出す
        return matchFrom(p, pi + 1, toks, pos + 1, end, caps, budget);
    }

    // Hole：短い方から順に試す
    const Arg& arg = p.info->args[piece.arg];
    size_t minLen = (arg.kind == Arg::Statements) ? 0 : 1;
    const Piece* next = (pi + 1 < p.pieces.size()) ? &p.pieces[pi + 1] : nullptr;
    int depth = 0;
    for (size_t len = 0; pos + len <= end; ++len) {
        if (len > 0) {
            const Tok& t = toks[pos + len - 1];
            if (t.kind == TokKind::Op && isOpen(t.s)) ++depth;
            if (t.kind == TokKind::Op && isClose(t.s)) {
                if (--depth < 0) break;
            }
        }
        if (len < minLen || depth != 0) continue;
        size_t stop = pos + len;
        if (next && next->kind == PieceKind::Literal && (stop >= end || toks[stop].s != next->text)) continue;
        if (!next && stop != end) continue;
        if (!captureFits(p, pi, toks, pos, stop)) continue;
        caps[piece.arg] = {pos, stop};
        if (matchFrom(p, pi + 1, toks, stop, end, caps, budget)) return true;
    }
    return false;
}

bool matchPattern(const Pattern& p, const Toks& toks, Captures& caps) {
    caps.assign(p.info->args.size(), {0, 0});
    int budget = 200000;
    return matchFrom(p, 0, toks, 0, toks.size(), caps, budget);
}

// ---------------------------------------------------------------------
//  5. ブロック（Blockly の JSON）を作る
// ---------------------------------------------------------------------
class Importer {
public:
    std::vector<Pattern> events, statements, values;
    int maxStatementCount = 1;
    // 変数（Java の名前 → Blockly の変数の id）。例：v_当たった回数 → var1
    std::map<std::string, std::string> variableIds;
    json variableList = json::array();

    // Java の名前から、Blockly での名前にする（このアプリが付けた v_ や f_ は取る）
    static std::string displayName(const std::string& javaName, const char* prefix) {
        return javaName.compare(0, 2, prefix) == 0 && javaName.size() > 2 ? javaName.substr(2) : javaName;
    }

    // 関数の引数・返す値に使える型（double 以外は、ブロックでは「int 回数」のように名前の前に書く）
    static bool isTypeName(const std::string& t) {
        static const std::set<std::string> kTypes = {"double", "int", "float", "long", "boolean", "String"};
        std::string base = t;
        while (base.size() >= 2 && base.compare(base.size() - 2, 2, "[]") == 0) base.resize(base.size() - 2);
        return kTypes.count(base) > 0;
    }
    // m[k] から型を読む（double / int / double[] など）。読めたら次の位置、読めなければ k のまま
    static size_t readType(const Toks& m, size_t k, size_t end, std::string& type) {
        if (k >= end || !isTypeName(m[k].s)) return k;
        type = m[k].s;
        size_t j = k + 1;
        while (j + 1 < end && m[j].s == "[" && m[j + 1].s == "]") { type += "[]"; j += 2; }
        return j;
    }
    // 「int 回数」→「回数」（型が書かれていなければそのまま）
    static std::string withoutType(const std::string& display) {
        size_t space = display.find(' ');
        if (space != std::string::npos && isTypeName(display.substr(0, space))) return display.substr(space + 1);
        return display;
    }

    // public / private / static などを飛ばした位置
    static size_t skipModifiers(const Toks& m) {
        static const std::set<std::string> mods = {"public", "private", "protected", "static", "final"};
        size_t k = 0;
        while (k < m.size() && mods.count(m[k].s)) ++k;
        return k;
    }

    // 数字の変数（double 名前; / double 名前 = 0; / int 名前 = 5; など）を、Blockly の変数として登録する
    //   このアプリが作った v_名前 だけでなく、自分で書いた変数も変数ブロックにする
    bool registerVariable(const Toks& member) {
        static const std::set<std::string> numberTypes = {"double", "int", "float", "long"};
        size_t k = skipModifiers(member);
        if (member.size() < k + 3 || !numberTypes.count(member[k].s) || member[k + 1].kind != TokKind::Ident ||
            member.back().s != ";") return false;
        size_t rest = member.size() - k;
        bool plain = rest == 3;                                                    // double 名前;
        bool withNumber = rest == 5 && member[k + 2].s == "=" && member[k + 3].kind == TokKind::Number;
        bool withNegative = rest == 6 && member[k + 2].s == "=" && member[k + 3].s == "-" &&
                            member[k + 4].kind == TokKind::Number;
        if (!plain && !withNumber && !withNegative) return false;
        if (member[k + 1].s.compare(0, 2, "v_") != 0 || member[k].s != "double" || k != 0) {
            // 自分で書いた変数は、ブロックにすると double になり、最初の値は 0 になる
            if (!plain && !(withNumber && member[k + 3].s == "0")) {
                notes.push_back("変数「" + member[k + 1].s + "」の最初の値（" +
                                joinTokens(member, k + 3, member.size() - 1) +
                                "）は、ブロックにすると 0 になります。必要なら「スタートしたら」で入れ直してください");
            }
        }
        variableIdFor(member[k + 1].s);
        return true;
    }

    // 今変換しているメソッドの中で使える変数（関数の引数）。外の変数より先にさがす
    std::map<std::string, std::string> scopeVariables;
    // 今変換しているメソッドの中で、自分で作られている変数（double x = …;）。変数ブロックにしない
    std::set<std::string> shadowedNames;

    bool isVariable(const Tok& t) const {
        if (t.kind != TokKind::Ident || shadowedNames.count(t.s)) return false;
        return scopeVariables.count(t.s) > 0 || variableIds.count(t.s) > 0;
    }
    std::string variableIdOf(const std::string& javaName) const {
        auto it = scopeVariables.find(javaName);
        if (it != scopeVariables.end()) return it->second;
        return variableIds.at(javaName);
    }

    // メソッドの中で「double x = …;」のように作られている変数の名前を集める
    //   double x = 式; の形（1つだけ作っているもの）は、Blockly の変数ブロックにする（scopeVariables に入れる）。
    //   それ以外（int i、for の中、クラスの変数と同じ名前など）は、変数ブロックにしない（shadowedNames）
    void collectShadowed(const Toks& m, size_t from, size_t to) {
        static const std::set<std::string> types = {"double", "int", "float", "long", "boolean", "String"};
        shadowedNames.clear();
        std::set<std::string> fieldIds;
        for (const auto& v : variableIds) fieldIds.insert(v.second);
        std::set<std::string> locals, others;
        for (size_t k = from; k + 2 < to; ++k) {
            bool startOfStatement = k == from || m[k - 1].s == ";" || m[k - 1].s == "{" || m[k - 1].s == "}" ||
                                    m[k - 1].s == "(";
            if (!(startOfStatement && types.count(m[k].s) && m[k + 1].kind == TokKind::Ident &&
                  (m[k + 2].s == "=" || m[k + 2].s == ";" || m[k + 2].s == "," || m[k + 2].s == ":" ||
                   m[k + 2].s == ")"))) continue;
            const std::string& name = m[k + 1].s;
            bool simple = m[k].s == "double" && (k == from || m[k - 1].s != "(") && (m[k + 2].s == "=" || m[k + 2].s == ";");
            if (simple && m[k + 2].s == "=") {
                // ; までに , がない（double a = 1, b = 2; ではない）
                int depth = 0;
                for (size_t j = k + 3; j < to; ++j) {
                    if (m[j].kind == TokKind::Op && isOpen(m[j].s)) ++depth;
                    if (m[j].kind == TokKind::Op && isClose(m[j].s)) --depth;
                    if (depth == 0 && m[j].s == ";") break;
                    if (depth == 0 && m[j].s == ",") { simple = false; break; }
                    if (depth < 0) { simple = false; break; }
                }
            }
            if (simple && !scopeVariables.count(name)) {
                auto same = idByDisplayName.find(displayName(name, "v_"));
                if (variableIds.count(name) || (same != idByDisplayName.end() && fieldIds.count(same->second))) {
                    simple = false;   // クラスの変数と同じ名前
                }
            }
            (simple ? locals : others).insert(name);
        }
        for (const auto& name : others) shadowedNames.insert(name);
        for (const auto& name : locals) {
            if (others.count(name) || scopeVariables.count(name)) { shadowedNames.insert(name); continue; }
            std::string id = newVariableId(displayName(name, "v_"));
            scopeVariables[name] = id;
            localNames.insert(name);
            localIds.insert(id);
        }
    }
    std::set<std::string> localNames;   // 今のメソッドの中で作られていて、変数ブロックにしたもの
    std::set<std::string> localIds;     // そういう変数の id（使われなかったら、最後に消す）
    std::set<std::string> rawUsedIds;   // そのまま書くブロックの中で使われた変数の id

    // メソッドの中で作った変数のうち、ブロックで使われなかったもの（型紙のブロックの中で作っているものなど）を消す
    void removeUnusedLocals(const json& tops) {
        std::string all = tops.dump();
        json kept = json::array();
        for (const auto& v : variableList) {
            std::string id = v.value("id", "");
            bool unused = localIds.count(id) && !rawUsedIds.count(id) &&
                          all.find("\"" + id + "\"") == std::string::npos;
            if (!unused) kept.push_back(v);
        }
        variableList = kept;
    }

    // 変数の命令（v_x = 式; / v_x += 式; / v_x++;）なら、Blockly の変数ブロックにする
    bool convertVariableStatement(const Toks& input, const std::string& eventType, int& rawCount, json& out) {
        // double x = 式; → x = 式;（メソッドの中で作った変数も、変数ブロックにする）
        if (input.size() >= 3 && input[0].s == "double" && localNames.count(input[1].s) && isVariable(input[1])) {
            if (input.size() == 3 && input[2].s == ";") {   // double x; だけ → ブロックはいらない
                out = json();
                return true;
            }
            Toks rest(input.begin() + 1, input.end());
            return convertVariableStatement(rest, eventType, rawCount, out);
        }
        const Toks& toks = input;
        if (toks.size() < 3 || !isVariable(toks[0]) || toks.back().s != ";") return false;
        json var = {{"id", variableIdOf(toks[0].s)}};
        const std::string& op = toks[1].s;
        if ((op == "++" || op == "--") && toks.size() == 3) {
            out = {{"type", "math_change"}, {"fields", {{"VAR", var}}},
                   {"inputs", {{"DELTA", {{"shadow", {{"type", "math_number"}, {"fields", {{"NUM", op == "++" ? 1 : -1}}}}}}}}}};
            ++converted;
            return true;
        }
        if ((op != "=" && op != "+=") || toks.size() < 4) return false;
        Toks expr(toks.begin() + 2, toks.end() - 1);
        double v = 0;
        bool literal = parseNumberLiteral(expr, 0, expr.size(), v);
        if (op == "=") {
            json value = literal ? json{{"type", kNumberBlock}, {"fields", {{"NUM", v}}}}
                                 : convertValue(expr, "", eventType, rawCount);
            out = {{"type", "variables_set"}, {"fields", {{"VAR", var}}}, {"inputs", {{"VALUE", {{"block", value}}}}}};
        } else {
            json delta = {{"shadow", {{"type", "math_number"}, {"fields", {{"NUM", literal ? v : 1}}}}}};
            if (!literal) delta["block"] = convertValue(expr, "", eventType, rawCount);
            out = {{"type", "math_change"}, {"fields", {{"VAR", var}}}, {"inputs", {{"DELTA", delta}}}};
        }
        ++converted;
        return true;
    }
    // ---- 関数（Blockly の「関数」カテゴリ） ----
    //   void f_名前(double v_引数) { … }   → 関数を作るブロック
    //   double f_名前(…) { … return 値; } → 値を返す関数を作るブロック
    struct FunctionSig {
        std::string display;                    // Blockly での名前（f_ を取ったもの）
        bool returns = false;                   // 値を返すか
        std::vector<std::string> paramDisplay;  // 引数の名前（v_ を取ったもの）
        std::vector<std::string> paramIds;      // 引数の変数の id
        std::vector<std::string> paramJava;     // 引数の Java での名前
        size_t bodyOpen = 0;                    // { の位置
        std::string eventType;                  // イベントの情報（ScannedRobotEvent e など）を受け取るなら、そのイベント
        int eventParamIndex = -1;               // そのイベントの情報が、何番目の引数か
        std::string eventParamName;             // その名前（e など）
    };
    std::map<std::string, std::string> eventTypeByClass;   // ScannedRobotEvent → robot_event_enemyDetected
    std::map<std::string, FunctionSig> functions;   // Java の名前 → 関数のようす
    const FunctionSig* currentFunction = nullptr;   // 今、関数の中を変換しているか

    int nextVariableNumber = 1;
    std::map<std::string, std::string> idByDisplayName;   // Blockly での名前 → id（同じ名前は同じ変数）

    std::string newVariableId(const std::string& display) {
        auto it = idByDisplayName.find(display);
        if (it != idByDisplayName.end()) return it->second;
        std::string id = "var" + std::to_string(nextVariableNumber++);
        idByDisplayName[display] = id;
        variableList.push_back({{"name", display}, {"id", id}});
        return id;
    }

    // クラスの変数として登録する
    std::string variableIdFor(const std::string& javaName) {
        auto it = variableIds.find(javaName);
        if (it != variableIds.end()) return it->second;
        std::string id = newVariableId(displayName(javaName, "v_"));
        variableIds[javaName] = id;
        return id;
    }

    // void f_名前(double v_a, double v_b) { … } を見つけたら、関数として登録する
    //   このアプリが作った f_名前 だけでなく、自分で書いたメソッド
    //   （double getDistance(double velocity) { … } など）も関数ブロックにする。
    //   返すのが void か double で、引数が全部 double のものだけ（ほかはそのまま書くブロック）
    bool registerFunction(const Toks& all) {
        size_t start = skipModifiers(all);
        if (all.size() < start + 6) return false;
        Toks m(all.begin() + start, all.end());
        // 返す型（void / double / int / double[] など）
        std::string returnType;
        size_t afterType = m[0].s == "void" ? 1 : readType(m, 0, m.size(), returnType);
        if (afterType == 0) return false;
        if (m[0].s == "void") returnType = "void";
        m.erase(m.begin() + 1, m.begin() + afterType);   // double[] f( → double f( のように1つにまとめる
        start += afterType - 1;
        if (m.size() < 6 || m[1].kind != TokKind::Ident || m[2].s != "(" || m.back().s != "}") return false;
        // Robocode のイベント（run や onScannedRobot など）は関数にしない
        const std::string& name = m[1].s;
        if (name == "run" || (name.size() > 2 && name.compare(0, 2, "on") == 0 &&
                              std::isupper(static_cast<unsigned char>(name[2])))) return false;
        size_t close = matchBracket(m, 2);
        if (close >= m.size() || close + 1 >= m.size() || m[close + 1].s != "{") return false;
        FunctionSig sig;
        sig.display = displayName(name, "f_");
        sig.returns = returnType != "void";
        // double 以外を返す関数は、名前の前に型を書く（例：int 数える）
        if (sig.returns && returnType != "double") sig.display = returnType + " " + sig.display;
        sig.bodyOpen = start + close + 1;
        size_t k = 3;
        int index = 0;
        while (k < close) {
            // イベントの情報（ScannedRobotEvent e など）。1つだけ受け取れる
            auto ev = eventTypeByClass.find(m[k].s);
            if (ev != eventTypeByClass.end() && k + 1 < close && m[k + 1].kind == TokKind::Ident &&
                sig.eventParamIndex < 0) {
                sig.eventType = ev->second;
                sig.eventParamIndex = index++;
                sig.eventParamName = m[k + 1].s;
                // ブロックでも「ScannedRobotEvent e」という引数として見せる（どのイベントの情報かを覚えておくため）
                std::string display = m[k].s + " e";
                sig.paramJava.push_back("e");
                sig.paramDisplay.push_back(display);
                sig.paramIds.push_back(newVariableId(display));
                k += 2;
                if (k < close) {
                    if (m[k].s != ",") return false;
                    ++k;
                }
                continue;
            }
            ++index;
            std::string type;
            size_t nameAt = readType(m, k, close, type);
            if (nameAt == k || nameAt >= close || m[nameAt].kind != TokKind::Ident) return false;
            const std::string& paramName = m[nameAt].s;
            // double 以外の引数は、名前の前に型を書く（例：int 回数、double[] scores）
            std::string display = displayName(paramName, "v_");
            if (type != "double") display = type + " " + display;
            sig.paramJava.push_back(paramName);
            sig.paramDisplay.push_back(display);
            // 同じ名前のクラスの変数があれば、同じ変数にする（Blockly では同じ名前の変数は1つ）
            auto global = variableIds.find(paramName);
            sig.paramIds.push_back(global != variableIds.end() && type == "double" ? global->second : newVariableId(display));
            k = nameAt + 1;
            if (k < close) {
                if (m[k].s != ",") return false;
                ++k;
            }
        }
        functions[name] = sig;
        return true;
    }

    // 数字ならそのまま数字ブロック、そうでなければ値ブロック
    json valueOrNumber(const Toks& expr, const std::string& wantType, const std::string& eventType, int& rawCount) {
        double v = 0;
        if (parseNumberLiteral(expr, 0, expr.size(), v)) return {{"type", kNumberBlock}, {"fields", {{"NUM", v}}}};
        return convertValue(expr, wantType, eventType, rawCount);
    }

    // f_名前(a, b) を、関数を呼ぶブロックにする（形が合わなければ false）
    bool convertFunctionCall(const Toks& toks, size_t end, bool asValue, const std::string& eventType, int& rawCount,
                             json& out) {
        if (end < 3 || toks[0].kind != TokKind::Ident || !functions.count(toks[0].s) || toks[1].s != "(" ||
            matchBracket(toks, 1) != end - 1) return false;
        const FunctionSig& sig = functions[toks[0].s];
        if (sig.returns != asValue) return false;
        // 引数を , で分ける
        std::vector<Toks> args;
        Toks current;
        int depth = 0;
        for (size_t k = 2; k + 1 < end; ++k) {
            const Tok& t = toks[k];
            if (t.kind == TokKind::Op && isOpen(t.s)) ++depth;
            if (t.kind == TokKind::Op && isClose(t.s)) --depth;
            if (depth == 0 && t.s == ",") { args.push_back(current); current.clear(); continue; }
            current.push_back(t);
        }
        if (!current.empty()) args.push_back(current);
        if (args.size() != sig.paramDisplay.size()) return false;
        if (sig.eventParamIndex >= 0) {
            // イベントの情報（e）は、ブロックでは穴を空けておく（Java にするときに自動で e が入る）
            const Toks& ev = args[sig.eventParamIndex];
            if (ev.size() != 1 || ev[0].kind != TokKind::Ident) return false;
        }
        out = {{"type", asValue ? "procedures_callreturn" : "procedures_callnoreturn"},
               {"extraState", {{"name", sig.display}, {"params", sig.paramDisplay}}}};
        for (size_t i = 0; i < args.size(); ++i) {
            if (static_cast<int>(i) == sig.eventParamIndex) continue;
            out["inputs"]["ARG" + std::to_string(i)]["block"] = valueOrNumber(args[i], "", eventType, rawCount);
        }
        ++converted;
        return true;
    }

    // if (〇) { return 値; } を「もし〇なら値を返す」ブロックにする（関数の中だけ）
    bool convertIfReturn(const Toks& t, const std::string& eventType, int& rawCount, json& out) {
        if (!currentFunction || t.size() < 7 || t[0].s != "if" || t[1].s != "(") return false;
        size_t close = matchBracket(t, 1);
        if (close + 4 >= t.size() || t[close + 1].s != "{" || t[close + 2].s != "return" || t.back().s != "}" ||
            matchBracket(t, close + 1) != t.size() - 1 || t[t.size() - 2].s != ";") return false;
        Toks cond(t.begin() + 2, t.begin() + close);
        Toks value(t.begin() + close + 3, t.end() - 2);
        if (currentFunction->returns == value.empty()) return false;   // 値を返す関数なら値が必要
        out = {{"type", "procedures_ifreturn"}};
        out["inputs"]["CONDITION"]["block"] = convertValue(cond, "Boolean", eventType, rawCount);
        if (!value.empty()) out["inputs"]["VALUE"]["block"] = valueOrNumber(value, "", eventType, rawCount);
        ++converted;
        return true;
    }

    // 関数を、関数を作るブロックにする
    json convertFunction(const Toks& original) {
        // 関数の名前は、最初の ( のすぐ前（double[] f( のように型が長いときもあるため）
        size_t paren = 0;
        while (paren < original.size() && original[paren].s != "(") ++paren;
        const FunctionSig& sig = functions[original[paren - 1].s];
        // イベントの情報の名前が e でなければ、e に直す（ブロックの型紙は e.getBearing() のように e を使うため）
        Toks m = original;
        if (sig.eventParamIndex >= 0 && sig.eventParamName != "e") {
            for (size_t k = 0; k < m.size(); ++k) {
                if (m[k].kind == TokKind::Ident && m[k].s == sig.eventParamName && !(k > 0 && m[k - 1].s == ".")) {
                    m[k].s = "e";
                }
            }
        }
        const std::string& eventType = sig.eventType;
        currentFunction = &sig;
        int rawCount = 0;
        // 引数は、この関数の中だけで使える変数
        scopeVariables.clear();
        for (size_t i = 0; i < sig.paramJava.size(); ++i) {
            if (static_cast<int>(i) == sig.eventParamIndex) continue;   // e は変数ブロックにしない
            scopeVariables[sig.paramJava[i]] = sig.paramIds[i];
        }
        collectShadowed(m, sig.bodyOpen + 1, m.size() - 1);
        std::vector<Toks> list = splitStatements(m, sig.bodyOpen + 1, m.size() - 1);
        json returnValue;
        if (sig.returns && !list.empty() && list.back().size() >= 3 && list.back()[0].s == "return") {
            Toks expr(list.back().begin() + 1, list.back().end() - 1);
            returnValue = valueOrNumber(expr, "", eventType, rawCount);
            list.pop_back();
        }
        json block = {{"type", sig.returns ? "procedures_defreturn" : "procedures_defnoreturn"},
                      {"fields", {{"NAME", sig.display}}}};
        if (!sig.paramDisplay.empty()) {
            json params = json::array();
            for (size_t i = 0; i < sig.paramDisplay.size(); ++i) {
                params.push_back({{"name", sig.paramDisplay[i]}, {"id", sig.paramIds[i]}});
            }
            block["extraState"]["params"] = params;
        }
        json chain = chainStatements(list, eventType, rawCount);
        if (!chain.is_null()) block["inputs"]["STACK"]["block"] = chain;
        if (!returnValue.is_null()) block["inputs"]["RETURN"]["block"] = returnValue;
        currentFunction = nullptr;
        scopeVariables.clear();
        shadowedNames.clear();
        localNames.clear();
        ++converted;
        return block;
    }

    int converted = 0;
    int raw = 0;
    std::vector<std::string> notes;

    Importer() {
        for (const Info* info : detail::RegisteredBlocks()) {
            // 「〇 回繰り返す」があるか（数字の欄 COUNT と、口 DO があるもの）
            if (info->type == "robot_for") {
                const Arg* c = info->findArg("COUNT");
                const Arg* d = info->findArg("DO");
                hasForBlock = c && c->kind == Arg::Number && d && d->kind == Arg::Statements;
            }
            if (info->javaTemplate.empty()) continue;  // javaFunction のブロックは型紙がない
            if (info->type == kRawStatement || info->type == kRawExpression || info->type == kRawMember ||
                info->type == kNumberBlock) continue;
            Pattern p;
            if (!buildPattern(*info, p)) continue;
            if (info->isEvent) {
                events.push_back(p);
                // public void onHitWall(HitWallEvent e) { … } の HitWallEvent を覚える
                const std::string& t = info->javaTemplate;
                size_t open = t.find('('), close = t.find(')');
                if (open != std::string::npos && close != std::string::npos && close > open) {
                    std::istringstream in(t.substr(open + 1, close - open - 1));
                    std::string cls, name;
                    if (in >> cls >> name) eventTypeByClass.emplace(cls, info->type);
                }
            }
            else if (info->isOutput) values.push_back(p);
            else statements.push_back(p);
        }
        for (const Pattern& p : statements) maxStatementCount = std::max(maxStatementCount, p.statementCount);
        // 型紙の文字が多い（くわしい）ものから試す
        auto byDetail = [](const Pattern& a, const Pattern& b) { return a.literalCount > b.literalCount; };
        std::stable_sort(statements.begin(), statements.end(), byDetail);
        std::stable_sort(values.begin(), values.end(), byDetail);
    }

    // そのまま書くブロック
    // そのまま書くブロックに入れる Java。
    //   ブロックにした変数・関数は、Java にもどすと名前に v_ や f_ が付くので、ここでも同じ名前に直しておく
    std::string rawJava(const Toks& toks) {
        Toks renamed = toks;
        for (size_t k = 0; k < renamed.size(); ++k) {
            Tok& t = renamed[k];
            if (t.kind != TokKind::Ident) continue;
            bool afterDot = k > 0 && renamed[k - 1].s == ".";   // e.getX の getX などは変えない
            if (afterDot) continue;
            bool isCall = k + 1 < renamed.size() && toks[k + 1].s == "(";
            if (isCall && functions.count(t.s)) {
                t.s = "f_" + withoutType(functions[t.s].display);
            } else if (!isCall && isVariable(t)) {
                std::string id = variableIdOf(t.s);
                rawUsedIds.insert(id);
                for (const auto& v : variableList) {
                    if (v.value("id", "") == id) t.s = "v_" + withoutType(v.value("name", ""));
                }
            }
        }
        return joinTokens(renamed, 0, renamed.size());
    }

    json rawBlock(const char* type, const std::string& code, const char* noteLabel) {
        ++raw;
        std::string shortCode = code.size() > 80 ? code.substr(0, 77) + "..." : code;
        notes.push_back(std::string(noteLabel) + "：" + shortCode);
        return json{{"type", type}, {"fields", {{"CODE", code}}}};
    }

    // 入力欄の中身を JSON に入れる
    //   score：くわしく当てはまったほど大きくなる / rawCount：そのまま書くになった数
    bool fillArgs(const Pattern& p, const Toks& toks, const Captures& caps, const std::string& eventType,
                  json& block, int& rawCount) {
        const Info& info = *p.info;
        for (size_t k = 0; k < info.args.size(); ++k) {
            const Arg& arg = info.args[k];
            size_t from = caps[k].first, to = caps[k].second;
            switch (arg.kind) {
            case Arg::Number: {
                double v = 0;
                parseNumberLiteral(toks, from, to, v);
                block["fields"][arg.name] = v;
                break;
            }
            case Arg::Dropdown: {
                std::string text = removeSpaces(joinTokens(toks, from, to));
                // get{AXIS}() のように単語の一部のとき
                for (const auto& piece : p.pieces) {
                    if (piece.kind == PieceKind::IdentHole && piece.arg == static_cast<int>(k)) {
                        const std::string& s = toks[from].s;
                        text = s.substr(piece.prefix.size(), s.size() - piece.prefix.size() - piece.suffix.size());
                    }
                    // {PART}{DIRECTION} のように、1つの単語に選ぶ欄が2つ以上あるとき
                    if (piece.kind == PieceKind::Compound &&
                        std::find(piece.partArgs.begin(), piece.partArgs.end(), static_cast<int>(k)) != piece.partArgs.end()) {
                        std::map<int, std::string> values;
                        if (splitCompound(piece, info, toks[from].s, 0, 0, values) && values.count(static_cast<int>(k))) {
                            text = values[static_cast<int>(k)];
                        }
                    }
                }
                for (const auto& o : arg.options) {
                    if (removeSpaces(o.second) == text) block["fields"][arg.name] = o.second;
                }
                break;
            }
            case Arg::Text:
                block["fields"][arg.name] = joinTokens(toks, from, to);
                break;
            case Arg::Variable:   // 変数を選ぶ欄（型紙で使うことはないので、何もしない）
                break;
            case Arg::Value: {
                Toks sub(toks.begin() + from, toks.begin() + to);
                double v = 0;
                if (arg.valueType != "Boolean" && parseNumberLiteral(toks, from, to, v)) {
                    // 数字だけなら、数字ブロック（穴に最初から入っている形）
                    json number = {{"type", kNumberBlock}, {"fields", {{"NUM", v}}}};
                    if (arg.valueType == "Number") block["inputs"][arg.name]["shadow"] = number;
                    else block["inputs"][arg.name]["block"] = number;
                } else {
                    json child = convertValue(sub, arg.valueType, eventType, rawCount);
                    block["inputs"][arg.name]["block"] = child;
                    if (arg.valueType == "Number") {
                        block["inputs"][arg.name]["shadow"] = {{"type", kNumberBlock}, {"fields", {{"NUM", 0}}}};
                    }
                }
                break;
            }
            case Arg::Statements: {
                json chain = convertStatementList(toks, from, to, eventType, rawCount);
                if (!chain.is_null()) block["inputs"][arg.name]["block"] = chain;
                break;
            }
            }
        }
        return true;
    }

    // 試しに作るとき用：数とメモを、試す前の状態にもどせるようにする
    struct Snapshot {
        int converted, raw;
        size_t notes;
    };
    Snapshot save() const { return {converted, raw, notes.size()}; }
    void restore(const Snapshot& s) {
        converted = s.converted;
        raw = s.raw;
        notes.resize(s.notes);
    }

    // 型紙のうち、いちばんよく当てはまるものをさがす（なければ nullptr）
    const Pattern* findBest(const std::vector<Pattern>& candidates, const Toks& toks, const std::string& wantType,
                            const std::string& eventType, bool isValue, int statementCount = 0) {
        const Pattern* best = nullptr;
        int bestScore = -1000000;
        for (const Pattern& p : candidates) {
            const Info& info = *p.info;
            if (statementCount > 0 && p.statementCount != statementCount) continue;
            if (!info.onlyIn.empty() && info.onlyIn != eventType) continue;
            if (isValue && !wantType.empty() && !info.outputType.empty() && info.outputType != wantType) continue;
            Captures caps;
            if (!matchPattern(p, toks, caps)) continue;
            // 試しに作って、そのまま書くになる数を数える
            Snapshot snap = save();
            json block = {{"type", info.type}};
            int innerRaw = 0;
            fillArgs(p, toks, caps, eventType, block, innerRaw);
            restore(snap);
            int fieldCount = 0;
            for (const auto& a : info.args) {
                if (a.kind == Arg::Number || a.kind == Arg::Dropdown) ++fieldCount;
            }
            int score = p.literalCount * 10 + fieldCount * 2 - innerRaw * 15;
            if (score > bestScore) {
                bestScore = score;
                best = &p;
            }
        }
        return best;
    }

    // 値ブロック（式）にする。wantType は穴の種類（"Number" / "Boolean" / ""）
    json convertValue(const Toks& toks, const std::string& wantType, const std::string& eventType, int& rawCount) {
        // 値を返す関数を呼ぶ
        json call;
        if (convertFunctionCall(toks, toks.size(), true, eventType, rawCount, call)) return call;
        // 変数
        if (toks.size() == 1 && isVariable(toks[0])) {
            ++converted;
            return {{"type", "variables_get"}, {"fields", {{"VAR", {{"id", variableIdOf(toks[0].s)}}}}}};
        }
        if (const Pattern* p = findBest(values, toks, wantType, eventType, true)) {
            Captures caps;
            matchPattern(*p, toks, caps);
            json block = {{"type", p->info->type}};
            fillArgs(*p, toks, caps, eventType, block, rawCount);
            ++converted;
            return block;
        }
        bool wrapped = toks.size() >= 3 && toks.front().s == "(" && matchBracket(toks, 0) == toks.size() - 1;
        // かっこなしで書かれた式（a > b && c < d など）は、( ) で囲んで試す
        //   ブロックの型紙は ({A} {OP} {B}) のように ( ) 付きのものが多いため
        if (!wrapped) {
            Toks w;
            w.push_back({TokKind::Op, "("});
            w.insert(w.end(), toks.begin(), toks.end());
            w.push_back({TokKind::Op, ")"});
            if (const Pattern* p = findBest(values, w, wantType, eventType, true)) {
                Captures caps;
                matchPattern(*p, w, caps);
                json block = {{"type", p->info->type}};
                fillArgs(*p, w, caps, eventType, block, rawCount);
                ++converted;
                return block;
            }
        }
        // ( 式 ) なら、かっこを外してもう一度
        if (wrapped) {
            Toks inner(toks.begin() + 1, toks.end() - 1);
            return convertValue(inner, wantType, eventType, rawCount);
        }
        ++rawCount;
        return rawBlock(kRawExpression, rawJava(toks), "式");
    }

    // 命令ブロックにする
    // 「〇 回繰り返す」（robot_for）は .javaFunction で Java を作っていて型紙がないので、ここで読む
    //   for (int i = 0; i < 50; i++) { … } → 「50 回繰り返す」（中で i を使っていないときだけ）
    bool hasForBlock = false;
    bool convertCountLoop(const Toks& t, const std::string& eventType, int& rawCount, json& out) {
        if (!hasForBlock || t.size() < 16 || t[0].s != "for" || t[1].s != "(" || t[2].s != "int" ||
            t[3].kind != TokKind::Ident || t[4].s != "=" || t[5].s != "0" || t[6].s != ";") return false;
        const std::string& name = t[3].s;
        if (t[7].s != name || t[8].s != "<" || t[9].kind != TokKind::Number || t[10].s != ";") return false;
        bool post = t[11].s == name && t[12].s == "++";   // i++
        bool pre = t[11].s == "++" && t[12].s == name;    // ++i
        if ((!post && !pre) || t[13].s != ")" || t[14].s != "{" || t.back().s != "}" ||
            matchBracket(t, 14) != t.size() - 1) return false;
        double count = 0;
        if (!parseNumberLiteral(t, 9, 10, count) || count < 0 || count != std::floor(count)) return false;
        // 中で数えている変数（i）を使っていたら、ブロックにはできない（ブロックの Java では名前が変わるため）
        for (size_t k = 15; k + 1 < t.size(); ++k) {
            if (t[k].kind == TokKind::Ident && t[k].s == name && !(k > 0 && t[k - 1].s == ".")) return false;
        }
        json block = {{"type", "robot_for"}, {"fields", {{"COUNT", count}}}};
        json chain = convertStatementList(t, 15, t.size() - 1, eventType, rawCount);
        if (!chain.is_null()) block["inputs"]["DO"]["block"] = chain;
        ++converted;
        out = block;
        return true;
    }

    json convertStatement(const Toks& toks, const std::string& eventType, int& rawCount) {
        json variableBlock;
        if (convertCountLoop(toks, eventType, rawCount, variableBlock)) return variableBlock;
        if (convertVariableStatement(toks, eventType, rawCount, variableBlock)) return variableBlock;
        // 関数を呼ぶ（f_名前(…);）・もし〇なら値を返す
        if (!toks.empty() && toks.back().s == ";" &&
            convertFunctionCall(toks, toks.size() - 1, false, eventType, rawCount, variableBlock)) return variableBlock;
        if (convertIfReturn(toks, eventType, rawCount, variableBlock)) return variableBlock;
        if (const Pattern* p = findBest(statements, toks, "", eventType, false)) {
            Captures caps;
            matchPattern(*p, toks, caps);
            json block = {{"type", p->info->type}};
            fillArgs(*p, toks, caps, eventType, block, rawCount);
            ++converted;
            return block;
        }
        ++rawCount;
        return rawBlock(kRawStatement, rawJava(toks), "命令");
    }

    // 文をいくつか並べて、上から下につながったブロックにする
    json convertStatementList(const Toks& toks, size_t from, size_t to, const std::string& eventType, int& rawCount) {
        return chainStatements(splitStatements(toks, from, to), eventType, rawCount);
    }

    json chainStatements(const std::vector<Toks>& list, const std::string& eventType, int& rawCount) {
        std::vector<json> blocks;
        for (size_t i = 0; i < list.size();) {
            // 文がいくつも入っている型紙（サンプルのブロックなど）を、長いものから先に試す
            bool done = false;
            for (int count = maxStatementCount; count >= 2 && !done; --count) {
                if (i + count > list.size()) continue;
                Toks joined;
                for (int k = 0; k < count; ++k) joined.insert(joined.end(), list[i + k].begin(), list[i + k].end());
                if (const Pattern* p = findBest(statements, joined, "", eventType, false, count)) {
                    Captures caps;
                    matchPattern(*p, joined, caps);
                    json block = {{"type", p->info->type}};
                    fillArgs(*p, joined, caps, eventType, block, rawCount);
                    ++converted;
                    blocks.push_back(block);
                    i += count;
                    done = true;
                }
            }
            if (done) continue;
            blocks.push_back(convertStatement(list[i], eventType, rawCount));
            ++i;
        }
        // 下からつないでいく
        json chain;
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            if (it->is_null()) continue;   // double x; のように、ブロックがいらない文
            json b = *it;
            if (!chain.is_null()) b["next"]["block"] = chain;
            chain = b;
        }
        return chain;
    }

    // class の中の1つ分（メソッドや変数）をブロックにする
    json convertMember(const Toks& toks) {
        // 単純なフィールド宣言は、Java のままブロックにするのではなく
        // 「変数を宣言する」ブロックへ変換する。
        // 例: private static double targetDistance = Double.MAX_VALUE;
        //     private String targetName = null;
        //     private boolean isAlive = true;
        {
            size_t k = 0;
            std::string access;
            std::string staticText;

            while (k < toks.size()) {
                if (toks[k].s == "public" || toks[k].s == "private" ||
                    toks[k].s == "protected") {
                    access = toks[k].s;
                    ++k;
                }
                else if (toks[k].s == "static") {
                    staticText = "static";
                    ++k;
                }
                else {
                    break;
                }
            }

            // 変数ブロックで扱う基本型。複雑な型や内部クラスはここでは扱わない。
            const std::set<std::string> supportedTypes = {
                "int", "float", "double", "long", "String", "boolean"
            };

            if (k + 2 < toks.size() &&
                supportedTypes.count(toks[k].s) != 0 &&
                toks[k + 1].kind == TokKind::Ident) {
                const std::string type = toks[k].s;
                const std::string name = toks[k + 1].s;
                size_t valueStart = k + 2;
                std::string initialValue;

                if (valueStart < toks.size() && toks[valueStart].s == "=") {
                    ++valueStart;
                    size_t valueEnd = valueStart;
                    while (valueEnd < toks.size() && toks[valueEnd].s != ";") {
                        // 配列・ラムダ・複数文などは安全に変換せず、通常のJavaブロックに任せる。
                        if (toks[valueEnd].s == "{" || toks[valueEnd].s == "}") {
                            initialValue.clear();
                            break;
                        }
                        initialValue += toks[valueEnd].s;
                        ++valueEnd;
                    }

                    if (valueEnd < toks.size() && toks[valueEnd].s == ";" &&
                        valueEnd + 1 == toks.size() && !initialValue.empty()) {
                        ++converted;
                        return json{
                            {"type", "robot_variable_declare"},
                            {"fields", {
                                {"ACCESS", access},
                                {"STATIC", staticText},
                                {"NAME", name},
                                {"TYPE", type},
                                {"INITIAL_VALUE", initialValue}
                            }}
                        };
                    }
                }
                else if (valueStart < toks.size() && toks[valueStart].s == ";" &&
                         valueStart + 1 == toks.size()) {
                    // 初期値の省略は型ごとのJava既定値を入れてブロック化する。
                    if (type == "String") initialValue = "null";
                    else if (type == "boolean") initialValue = "false";
                    else if (type == "float") initialValue = "0.0f";
                    else if (type == "double") initialValue = "0.0";
                    else initialValue = "0";

                    ++converted;
                    return json{
                        {"type", "robot_variable_declare"},
                        {"fields", {
                            {"ACCESS", access},
                            {"STATIC", staticText},
                            {"NAME", name},
                            {"TYPE", type},
                            {"INITIAL_VALUE", initialValue}
                        }}
                    };
                }
            }
        }

        int rawCount = 0;
        scopeVariables.clear();
        localNames.clear();
        collectShadowed(toks, 0, toks.size());
        struct ClearShadow {
            std::set<std::string>& s;
            std::map<std::string, std::string>& scope;
            std::set<std::string>& locals;
            ~ClearShadow() { s.clear(); scope.clear(); locals.clear(); }
        } clearShadow{shadowedNames, scopeVariables, localNames};
        // ① イベントの型紙にまるごと当てはまるか
        for (const Pattern& p : events) {
            Captures caps;
            if (!matchPattern(p, toks, caps)) continue;
            json block = {{"type", p.info->type}};
            fillArgs(p, toks, caps, p.info->type, block, rawCount);
            ++converted;
            return block;
        }
        // ② 最初の { までが同じなら、中身を最初の「口」に入れる
        //    例：public void run() { 準備; while (true) { … } } → 「スタートしたら、ずっと」の中に入れる
        size_t bodyOpen = 0;
        while (bodyOpen < toks.size() && toks[bodyOpen].s != "{") ++bodyOpen;
        if (bodyOpen < toks.size() && toks.back().s == "}") {
            for (const Pattern& p : events) {
                size_t k = 0;
                bool same = true;
                for (; k < p.pieces.size() && k < bodyOpen; ++k) {
                    if (p.pieces[k].kind != PieceKind::Literal || p.pieces[k].text != toks[k].s) { same = false; break; }
                }
                if (!same || k != bodyOpen || k >= p.pieces.size() || p.pieces[k].text != "{") continue;
                int holeArg = -1;
                for (const auto& piece : p.pieces) {
                    if (piece.kind == PieceKind::Hole && p.info->args[piece.arg].kind == Arg::Statements) {
                        holeArg = piece.arg;
                        break;
                    }
                }
                if (holeArg < 0) continue;
                json block = {{"type", p.info->type}};
                json chain = convertStatementList(toks, bodyOpen + 1, toks.size() - 1, p.info->type, rawCount);
                if (!chain.is_null()) block["inputs"][p.info->args[holeArg].name]["block"] = chain;
                ++converted;
                notes.push_back("「" + joinTokens(toks, 0, bodyOpen) + "」は形が少しちがうので、中身をイベントのブロックの中に入れました（動きが変わっていないか確かめてください）");
                return block;
            }
        }
        // ③ 当てはまらない → クラスの中にそのまま書くブロック
        return rawBlock(kRawMember, rawJava(toks), "メソッド・変数");
    }
};

// ブロックの高さのめやす（ならべるときに重ならないように）
//   命令ブロック1つを 32 として、中に入っている命令ブロックも全部数える
int countStatementBlocks(const json& block) {
    int n = 0;
    for (const json* b = &block; b && b->is_object();) {
        ++n;
        if (b->contains("inputs")) {
            for (const auto& in : (*b)["inputs"].items()) {
                const json& child = in.value().value("block", json());
                if (child.is_object() && (child.contains("next") || child.contains("inputs"))) {
                    n += countStatementBlocks(child);
                }
            }
        }
        b = (b->contains("next") && (*b)["next"].contains("block")) ? &(*b)["next"]["block"] : nullptr;
    }
    return n;
}

int estimateHeight(const json& block) { return countStatementBlocks(block) * 32 + 40; }

}  // namespace

ImportResult ConvertFromJava(const std::string& javaSource) {
    ImportResult result;
    Toks toks = tokenize(javaSource);

    // package
    for (size_t k = 0; k + 1 < toks.size(); ++k) {
        if (toks[k].s == "package") {
            size_t e = k + 1;
            while (e < toks.size() && toks[e].s != ";") ++e;
            result.packageName = removeSpaces(joinTokens(toks, k + 1, e));
            break;
        }
    }

    // class 名前 … { … }
    size_t classPos = toks.size();
    for (size_t k = 0; k + 1 < toks.size(); ++k) {
        if (toks[k].s == "class" && toks[k + 1].kind == TokKind::Ident) { classPos = k; break; }
    }
    if (classPos == toks.size()) {
        result.error = "Java の中に class が見つかりませんでした";
        return result;
    }
    result.robotName = toks[classPos + 1].s;
    size_t open = classPos;
    while (open < toks.size() && toks[open].s != "{") ++open;
    if (open == toks.size()) {
        result.error = "class の { が見つかりませんでした";
        return result;
    }
    size_t close = matchBracket(toks, open);
    if (close == toks.size()) close = toks.size();  // } が足りなくても、最後まで読む

    Importer importer;
    json tops = json::array();
    int y = 20;
    std::vector<Toks> members = splitMembers(toks, open + 1, close);
    // 先に変数（double v_名前 = 0;）を集める（メソッドの中で使われているので）
    std::vector<bool> isVariableMember(members.size(), false);
    for (size_t k = 0; k < members.size(); ++k) isVariableMember[k] = importer.registerVariable(members[k]);
    // 関数（void f_名前(…) { … }）も先に集める（イベントの中から呼ばれているので）
    std::vector<bool> isFunctionMember(members.size(), false);
    for (size_t k = 0; k < members.size(); ++k) {
        if (!isVariableMember[k]) isFunctionMember[k] = importer.registerFunction(members[k]);
    }
    for (size_t k = 0; k < members.size(); ++k) {
        const Toks& member = members[k];
        if (isVariableMember[k]) continue;
        if (isFunctionMember[k]) {
            json block = importer.convertFunction(member);
            block["x"] = 20;
            block["y"] = y;
            y += estimateHeight(block) + 30;
            tops.push_back(block);
            continue;
        }
        std::string text = removeSpaces(joinTokens(member, 0, member.size()));
        // このアプリが自動で作る変数（記憶ブロック用）は、ブロックにしない
        if (text == "doublememA=0,memB=0,memC=0;") continue;
        json block = importer.convertMember(member);
        block["x"] = 20;
        block["y"] = y;
        y += estimateHeight(block) + 30;
        tops.push_back(block);
    }

    // 関数を作るブロックを先に並べる（画面の場所は x, y で決まるので変わらない）
    //   Blockly は上から順に読みこむ。関数を呼ぶブロックを先に読むと、Blockly が同じ名前の空の関数を
    //   自動で作ってしまい、あとから読む本物の関数の名前が「shoot2」のように変わってしまうため
    {
        json ordered = json::array();
        for (const auto& b : tops) {
            std::string type = b.value("type", "");
            if (type == "procedures_defnoreturn" || type == "procedures_defreturn") ordered.push_back(b);
        }
        for (const auto& b : tops) {
            std::string type = b.value("type", "");
            if (type != "procedures_defnoreturn" && type != "procedures_defreturn") ordered.push_back(b);
        }
        tops = ordered;
    }

    importer.removeUnusedLocals(tops);
    json workspace = {{"blocks", {{"languageVersion", 0}, {"blocks", tops}}}};
    if (!importer.variableList.empty()) workspace["variables"] = importer.variableList;
    result.workspaceJson = workspace.dump(-1, ' ', false, json::error_handler_t::replace);
    result.convertedBlocks = importer.converted;
    result.rawBlocks = importer.raw;
    result.notes = importer.notes;
    result.ok = true;
    return result;
}

}  // namespace rb
