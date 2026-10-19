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
    return cleaned;
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
enum class PieceKind { Literal, Hole, IdentHole };

struct Piece {
    PieceKind kind;
    std::string text;           // Literal のとき
    int arg = -1;               // Hole / IdentHole のとき：何番目の入力欄か
    std::string prefix, suffix; // IdentHole のとき：get{AXIS}() の get と空
};

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
            while (j < src.size()) {
                size_t e = 0;
                int a = -1;
                if (holeArg < 0 && readHole(info, src, j, e, a)) {
                    holeArg = a;
                    j = e;
                } else if (isIdentChar(src[j])) {
                    (holeArg < 0 ? prefix : suffix) += src[j];
                    ++j;
                } else {
                    break;
                }
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
    case Arg::Text:
        return true;
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
    int converted = 0;
    int raw = 0;
    std::vector<std::string> notes;

    Importer() {
        for (const Info* info : detail::RegisteredBlocks()) {
            if (info->javaTemplate.empty()) continue;  // javaFunction のブロックは型紙がない
            if (info->type == kRawStatement || info->type == kRawExpression || info->type == kRawMember ||
                info->type == kNumberBlock) continue;
            Pattern p;
            if (!buildPattern(*info, p)) continue;
            if (info->isEvent) events.push_back(p);
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
                }
                for (const auto& o : arg.options) {
                    if (removeSpaces(o.second) == text) block["fields"][arg.name] = o.second;
                }
                break;
            }
            case Arg::Text:
                block["fields"][arg.name] = joinTokens(toks, from, to);
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
        return rawBlock(kRawExpression, joinTokens(toks, 0, toks.size()), "式");
    }

    // 命令ブロックにする
    json convertStatement(const Toks& toks, const std::string& eventType, int& rawCount) {
        if (const Pattern* p = findBest(statements, toks, "", eventType, false)) {
            Captures caps;
            matchPattern(*p, toks, caps);
            json block = {{"type", p->info->type}};
            fillArgs(*p, toks, caps, eventType, block, rawCount);
            ++converted;
            return block;
        }
        ++rawCount;
        return rawBlock(kRawStatement, joinTokens(toks, 0, toks.size()), "命令");
    }

    // 文をいくつか並べて、上から下につながったブロックにする
    json convertStatementList(const Toks& toks, size_t from, size_t to, const std::string& eventType, int& rawCount) {
        std::vector<Toks> list = splitStatements(toks, from, to);
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
            json b = *it;
            if (!chain.is_null()) b["next"]["block"] = chain;
            chain = b;
        }
        return chain;
    }

    // class の中の1つ分（メソッドや変数）をブロックにする
    json convertMember(const Toks& toks) {
        int rawCount = 0;
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
        return rawBlock(kRawMember, joinTokens(toks, 0, toks.size()), "メソッド・変数");
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
    for (const Toks& member : splitMembers(toks, open + 1, close)) {
        std::string text = removeSpaces(joinTokens(member, 0, member.size()));
        // このアプリが自動で作る変数（記憶ブロック用）は、ブロックにしない
        if (text == "doublememA=0,memB=0,memC=0;") continue;
        json block = importer.convertMember(member);
        block["x"] = 20;
        block["y"] = y;
        y += estimateHeight(block) + 30;
        tops.push_back(block);
    }

    json workspace = {{"blocks", {{"languageVersion", 0}, {"blocks", tops}}}};
    result.workspaceJson = workspace.dump(-1, ' ', false, json::error_handler_t::replace);
    result.convertedBlocks = importer.converted;
    result.rawBlocks = importer.raw;
    result.notes = importer.notes;
    result.ok = true;
    return result;
}

}  // namespace rb
