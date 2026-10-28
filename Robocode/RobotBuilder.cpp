// =====================================================================
//  RobotBuilder.cpp  ―  できた Java を Robocode で使えるようにする
// 
//  2026/10/26 片山 変数も攻撃の上限のエラーが出るようにする。
// =====================================================================
#include <cstring>
#include "RobotBuilder.h"

#include <shlobj.h>     // SHGetKnownFolderPath（設定の保存場所）
#include <shobjidl.h>   // IFileOpenDialog（フォルダを選ぶ画面）
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iomanip>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// ---------------------------------------------------------------------
//  文字コードの変換
// ---------------------------------------------------------------------
static std::wstring ToWide(const std::string& s, UINT codePage) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(codePage, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(codePage, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}

static std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], len, nullptr, nullptr);
    return s;
}

// ---------------------------------------------------------------------
//  ファイル・フォルダの道具
// ---------------------------------------------------------------------
static bool FileExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

// Robocode のフォルダか？（libs\robocode.jar があるか）
static bool IsRobocodeFolder(const std::wstring& folder) {
    return !folder.empty() && FileExists(folder + L"\\libs\\robocode.jar");
}

// 設定ファイルの場所：%LOCALAPPDATA%\RobocodeBlocks\robocode_folder.txt
static std::wstring SettingsFile() {
    std::wstring dir;
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        dir = path;
    }
    CoTaskMemFree(path);
    dir += L"\\RobocodeBlocks";
    CreateDirectoryW(dir.c_str(), nullptr);  // すでにあれば何もしない
    return dir + L"\\robocode_folder.txt";
}

static std::wstring LoadSavedFolder() {
    std::ifstream in(SettingsFile().c_str(), std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    return ToWide(text, CP_UTF8);
}

static void SaveFolder(const std::wstring& folder) {
    std::ofstream out(SettingsFile().c_str(), std::ios::binary);
    out << ToUtf8(folder);
}

// フォルダを選ぶ画面を出す（キャンセルされたら空の文字列）
static std::wstring ChooseFolder(HWND owner) {
    std::wstring result;
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) {
        return result;
    }
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dialog->SetTitle(L"Robocode のフォルダ（中に libs フォルダがある場所）を選んでください");

    if (SUCCEEDED(dialog->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                result = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dialog->Release();
    return result;
}

std::wstring GetRobocodeFolder(HWND owner, bool forceChoose) {
    if (!forceChoose) {
        // 前に選んだフォルダ
        std::wstring saved = LoadSavedFolder();
        if (IsRobocodeFolder(saved)) return saved;

        // よくあるインストール先
        if (IsRobocodeFolder(L"C:\\robocode")) {
            SaveFolder(L"C:\\robocode");
            return L"C:\\robocode";
        }
    }

    // 選んでもらう
    for (;;) {
        std::wstring chosen = ChooseFolder(owner);
        if (chosen.empty()) return L"";  // キャンセル
        if (IsRobocodeFolder(chosen)) {
            SaveFolder(chosen);
            return chosen;
        }
        int answer = MessageBoxW(owner,
            L"このフォルダには libs\\robocode.jar がありません。\n"
            L"Robocode をインストールしたフォルダ（例：C:\\robocode）を選んでください。",
            L"Robocode のフォルダ", MB_RETRYCANCEL | MB_ICONWARNING);
        if (answer != IDRETRY) return L"";
    }
}

// ---------------------------------------------------------------------
//  javac を探して実行する
// ---------------------------------------------------------------------
// アプリといっしょに入れた JDK（exe の横の jdk フォルダ）
//   インストーラーで JDK もいっしょに入れると、JAVA_HOME や PATH を設定しなくても動く
static std::wstring BundledJdkFolder() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return L"";
    std::wstring dir(buf);
    dir = dir.substr(0, dir.find_last_of(L"\\/"));   // exe のあるフォルダ
    std::wstring jdk = dir + L"\\jdk";
    return FileExists(jdk + L"\\bin\\javac.exe") ? jdk : L"";
}

static std::wstring FindJavac() {
    wchar_t buf[MAX_PATH];

    // 0. アプリといっしょに入れた JDK（いちばん先に使う）
    std::wstring bundled = BundledJdkFolder();
    if (!bundled.empty()) return bundled + L"\\bin\\javac.exe";

    // 1. JAVA_HOME\bin\javac.exe
    DWORD n = GetEnvironmentVariableW(L"JAVA_HOME", buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        std::wstring path = std::wstring(buf) + L"\\bin\\javac.exe";
        if (FileExists(path)) return path;
    }

    // 2. PATH の中
    n = SearchPathW(nullptr, L"javac.exe", nullptr, MAX_PATH, buf, nullptr);
    if (n > 0 && n < MAX_PATH) return buf;

    return L"";
}

// コマンドを実行して、画面に出した文字（標準出力・標準エラー）と終了コードを受け取る
static bool RunCommand(std::wstring commandLine, std::string& output, DWORD& exitCode) {
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;

    PROCESS_INFORMATION pi = {};
    BOOL started = CreateProcessW(nullptr, &commandLine[0], nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(writePipe);  // こちらでは書かないので閉じる（閉じないと読み終わらない）
    if (!started) {
        CloseHandle(readPipe);
        return false;
    }

    std::string raw;
    char buf[4096];
    DWORD read = 0;
    while (ReadFile(readPipe, buf, sizeof(buf), &read, nullptr) && read > 0) {
        raw.append(buf, read);
    }
    CloseHandle(readPipe);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // javac は Windows の文字コード（日本語の Windows なら Shift-JIS）で出力するので UTF-8 にする
    output = ToUtf8(ToWide(raw, CP_ACP));
    return true;
}

// ---------------------------------------------------------------------
//  名前のチェック
//    Robocode では、パッケージ名は16文字まで、ロボットの名前は32文字まで（どちらも大文字を使える）
// ---------------------------------------------------------------------
static bool IsLower(char c) { return c >= 'a' && c <= 'z'; }
static bool IsUpper(char c) { return c >= 'A' && c <= 'Z'; }
static bool IsDigit(char c) { return c >= '0' && c <= '9'; }

std::string CheckRobotName(const std::string& name) {
    if (name.empty()) return "ロボットの名前が入っていません";
    if (name.size() > 32) return "ロボットの名前は32文字までにしてください";
    if (!IsLower(name[0]) && !IsUpper(name[0])) return "ロボットの名前は英字で始めてください";
    for (char c : name) {
        if (!IsLower(c) && !IsUpper(c) && !IsDigit(c) && c != '_') {
            return "ロボットの名前には、英字・数字・_ だけを使ってください";
        }
    }
    return "";
}

std::string CheckPackageName(const std::string& name) {
    if (name.empty()) return "パッケージ名が入っていません";
    if (name.size() > 16) return "パッケージ名は16文字までにしてください";
    bool startOfPart = true;  // . の直後（区切りの最初の文字）かどうか
    for (char c : name) {
        if (c == '.') {
            if (startOfPart) return "パッケージ名の . の前後には文字を入れてください";
            startOfPart = true;
            continue;
        }
        // 大文字も使える（例：Team.Red）
        if (startOfPart && !IsLower(c) && !IsUpper(c)) return "パッケージ名は英字で始めてください（. のあとも同じ）";
        if (!IsLower(c) && !IsUpper(c) && !IsDigit(c) && c != '_') {
            return "パッケージ名には、英字・数字・_ と、区切りの . だけを使ってください";
        }
        startOfPart = false;
    }
    if (startOfPart) return "パッケージ名の最後に . は付けられません";
    return "";
}

// ---------------------------------------------------------------------
// fire のパワーを検査する
// ---------------------------------------------------------------------
namespace
{
    constexpr double MIN_FIRE_POWER = 0.1;
    constexpr double MAX_FIRE_POWER = 3.0;

    class ExpressionParser
    {
    public:
        ExpressionParser(
            const std::string& expression,
            const std::map<std::string, double>& variables)
            : m_expression(expression),
            m_variables(variables),
            m_position(0)
        {
        }

        double Calculate()
        {
            double result = ParseExpression();
            SkipSpaces();

            // 式の最後まで読み取れなければ計算できない
            if (m_position != m_expression.size())
                throw std::runtime_error("計算できない式です");

            return result;
        }

    private:
        const std::string& m_expression;
        const std::map<std::string, double>& m_variables;
        size_t m_position;

        void SkipSpaces()
        {
            while (m_position < m_expression.size() &&
                std::isspace(
                    static_cast<unsigned char>(
                        m_expression[m_position])))
            {
                ++m_position;
            }
        }

        // 足し算・引き算
        double ParseExpression()
        {
            double value = ParseTerm();

            while (true)
            {
                SkipSpaces();

                if (m_position >= m_expression.size())
                    break;

                char op = m_expression[m_position];

                if (op != '+' && op != '-')
                    break;

                ++m_position;

                double right = ParseTerm();

                if (op == '+')
                    value += right;
                else
                    value -= right;
            }

            return value;
        }

        // 掛け算・割り算
        double ParseTerm()
        {
            double value = ParseFactor();

            while (true)
            {
                SkipSpaces();

                if (m_position >= m_expression.size())
                    break;

                char op = m_expression[m_position];

                if (op != '*' && op != '/')
                    break;

                ++m_position;

                double right = ParseFactor();

                if (op == '*')
                {
                    value *= right;
                }
                else
                {
                    if (right == 0.0)
                        throw std::runtime_error("0で割る式です");

                    value /= right;
                }
            }

            return value;
        }

        // 数字・変数・括弧
        double ParseFactor()
        {
            SkipSpaces();

            if (m_position >= m_expression.size())
                throw std::runtime_error("値がありません");

            char c = m_expression[m_position];

            // マイナスの値
            if (c == '-')
            {
                ++m_position;
                return -ParseFactor();
            }

            // プラスの値
            if (c == '+')
            {
                ++m_position;
                return ParseFactor();
            }

            // 括弧内の計算
            if (c == '(')
            {
                ++m_position;

                double value = ParseExpression();

                SkipSpaces();

                if (m_position >= m_expression.size() ||
                    m_expression[m_position] != ')')
                {
                    throw std::runtime_error("括弧が正しくありません");
                }

                ++m_position;
                return value;
            }

            // 数字
            if (std::isdigit(static_cast<unsigned char>(c)) ||
                c == '.')
            {
                const char* start = m_expression.c_str()
                    + m_position;

                char* end = nullptr;
                double value = std::strtod(start, &end);

                if (end == start)
                    throw std::runtime_error("数字を読み取れません");

                m_position += static_cast<size_t>(end - start);
                return value;
            }

            // 変数名
            if (std::isalpha(static_cast<unsigned char>(c)) ||
                c == '_' || c == '$')
            {
                size_t start = m_position++;

                while (m_position < m_expression.size())
                {
                    char ch = m_expression[m_position];

                    if (!std::isalnum(
                        static_cast<unsigned char>(ch)) &&
                        ch != '_' && ch != '$')
                    {
                        break;
                    }

                    ++m_position;
                }

                std::string name =
                    m_expression.substr(start, m_position - start);

                auto found = m_variables.find(name);

                if (found == m_variables.end())
                    throw std::runtime_error("変数の値が不明です");

                return found->second;
            }

            throw std::runtime_error("対応していない計算式です");
        }
    };


    std::string CheckAdvancedValues(const std::string& javaCode)
    {
        std::map<std::string, double> variables;

        // 数値型変数の初期値を取得する（前に宣言された変数を使う計算にも対応）
        const std::regex declaration(
            R"(\b(?:byte|short|int|long|float|double)\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*([^;]+);)"
        );

        for (std::sregex_iterator it(
            javaCode.begin(), javaCode.end(), declaration), end;
            it != end; ++it)
        {
            const std::string name = (*it)[1].str();
            const std::string expression = (*it)[2].str();

            try
            {
                ExpressionParser parser(expression, variables);
                variables[name] = parser.Calculate();
            }
            catch (...)
            {
                // 初期値を計算できない変数は登録しない
            }
        }

        struct CallRule
        {
            const char* name;
            double minimum;
            double maximum;
            const char* label;
            const char* rangeText;
        };

        const CallRule rules[] = {
            { "fire",           0.1, 3.0, "攻撃エラー", "パワーは0.1～3.0にしてください。" },
            { "setFire",        0.1, 3.0, "攻撃エラー", "パワーは0.1～3.0にしてください。" },
            { "getBulletSpeed", 0.1, 3.0, "ゲーム情報エラー", "弾の速さを調べる発射パワーは0.1～3.0にしてください。" },
            { "setMaxVelocity", 0.0, 8.0, "速度エラー", "最高速度は0～8にしてください。" }
        };

        for (const auto& rule : rules)
        {
            size_t searchPosition = 0;

            while (true)
            {
                size_t callPosition = javaCode.find(rule.name, searchPosition);
                if (callPosition == std::string::npos)
                    break;

                searchPosition = callPosition + std::strlen(rule.name);

                // メソッド名の一部に一致した場合は除外する
                if (callPosition > 0)
                {
                    char before = javaCode[callPosition - 1];
                    if (std::isalnum(static_cast<unsigned char>(before)) ||
                        before == '_' || before == '$')
                    {
                        continue;
                    }
                }

                size_t openPosition = searchPosition;
                while (openPosition < javaCode.size() &&
                    std::isspace(static_cast<unsigned char>(javaCode[openPosition])))
                {
                    ++openPosition;
                }

                if (openPosition >= javaCode.size() || javaCode[openPosition] != '(')
                    continue;

                size_t argumentStart = openPosition + 1;
                size_t position = argumentStart;
                int depth = 1;

                while (position < javaCode.size() && depth > 0)
                {
                    if (javaCode[position] == '(')
                        ++depth;
                    else if (javaCode[position] == ')')
                        --depth;
                    ++position;
                }

                if (depth != 0)
                    continue;

                const size_t argumentEnd = position - 1;
                const std::string argument = javaCode.substr(
                    argumentStart, argumentEnd - argumentStart);

                searchPosition = position;

                try
                {
                    ExpressionParser parser(argument, variables);
                    const double value = parser.Calculate();

                    if (value < rule.minimum || value > rule.maximum)
                    {
                        std::ostringstream valueText;
                        valueText << std::setprecision(10) << value;
                        std::string valueString = valueText.str();

                        if (valueString.find('.') != std::string::npos)
                        {
                            valueString.erase(valueString.find_last_not_of('0') + 1);
                            if (!valueString.empty() && valueString.back() == '.')
                                valueString.pop_back();
                        }

                        return std::string(rule.label) + "：" + rule.name +
                            " に指定した値が範囲外です。\n" +
                            rule.rangeText +
                            "計算結果：" + valueString;
                    }
                }
                catch (...)
                {
                    // 値を計算できない式は、この簡易検査では判定しない
                }
            }
        }

        return "";
    }

    
}


// ---------------------------------------------------------------------
//  保存してコンパイルする
// ---------------------------------------------------------------------
BuildResult BuildRobot(const std::wstring& robocodeFolder,
                       const std::string& packageName,
                       const std::string& robotName,
                       const std::string& javaCode) {
    BuildResult r;

    r.message = CheckPackageName(packageName);
    if (r.message.empty()) r.message = CheckRobotName(robotName);
    if (!r.message.empty()) return r;

    // 攻撃パワーの検査
    r.message = CheckAdvancedValues(javaCode);
    if (!r.message.empty()) return r;

    // 1. robots\パッケージ名\ロボット名.java に保存する
    //    パッケージ名の . はフォルダの区切りになる（例：team.red → robots\team\red）
    std::wstring packagePath = ToWide(packageName, CP_UTF8);
    for (auto& c : packagePath) {
        if (c == L'.') c = L'\\';
    }
    const std::wstring robotsDir = robocodeFolder + L"\\robots";
    const std::wstring packageDir = robotsDir + L"\\" + packagePath;
    SHCreateDirectoryExW(nullptr, packageDir.c_str(), nullptr);  // 途中のフォルダもまとめて作る

    const std::wstring javaFile = packageDir + L"\\" + ToWide(robotName, CP_UTF8) + L".java";
    {
        std::ofstream out(javaFile.c_str(), std::ios::binary);
        if (!out) {
            r.message = "Java のファイルを保存できませんでした（フォルダに書きこめない可能性があります）";
            r.detail = ToUtf8(javaFile);
            return r;
        }
        out << javaCode;
    }
    r.javaPath = ToUtf8(javaFile);

    // 2. javac を探す
    const std::wstring javac = FindJavac();
    if (javac.empty()) {
        r.message = "Java を保存しましたが、javac が見つからないのでコンパイルできませんでした。"
                    "JDK（Java の開発キット）をインストールしてください";
        r.detail = "JAVA_HOME か PATH に JDK の bin フォルダを設定すると見つかるようになります";
        return r;
    }

    // 3. javac でコンパイルする
    //   javac -encoding UTF-8 -source 1.8 -target 1.8 -Xlint:-options -cp "<Robocode>\libs\robocode.jar" -d "<Robocode>\robots" "<.java>"
    //   -source 1.8 -target 1.8：Java 8 でも動く .class を作る（JDK 8 でも JDK 21 でも使える書き方）
    //   -Xlint:-options      ：JDK 21 で出る「8 は廃止予定」という警告を出さない
    const std::wstring command =
        L"\"" + javac + L"\" -encoding UTF-8 -source 1.8 -target 1.8 -Xlint:-options"
        L" -cp \"" + robocodeFolder + L"\\libs\\robocode.jar\""
        L" -d \"" + robotsDir + L"\""
        L" \"" + javaFile + L"\"";

    std::string output;
    DWORD exitCode = 1;
    if (!RunCommand(command, output, exitCode)) {
        r.message = "javac を実行できませんでした";
        r.detail = ToUtf8(javac);
        return r;
    }
    if (exitCode != 0) {
        r.message = "コンパイルでエラーが出ました（下のくわしい内容を見てください）";
        r.detail = output;
        return r;
    }

    r.ok = true;
    r.message = "Robocode で使えるようになりました（" + packageName + "." + robotName + "）";
    r.detail = output;  // 警告が出ていればここに入る
    return r;
}

// ---------------------------------------------------------------------
//  Robocode を起動する（開き直す）
// ---------------------------------------------------------------------
//  このアプリから起動した Robocode は「ジョブ」という入れ物に入れておく。
//  開き直すときは、その入れ物ごと閉じる（robocode.bat から起動した Java もまとめて閉じられる）。
//  自分で別に起動した Robocode は閉じられないので、そのときは手で閉じてもらう。
static HANDLE g_robocodeJob = nullptr;

bool LaunchRobocode(const std::wstring& robocodeFolder, std::string& message) {
    const std::wstring bat = robocodeFolder + L"\\robocode.bat";
    if (!FileExists(bat)) {
        message = "robocode.bat が見つかりません（" + ToUtf8(robocodeFolder) + "）";
        return false;
    }

    // 前に起動した Robocode がまだ開いていたら閉じる
    bool restarted = false;
    if (g_robocodeJob != nullptr) {
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info = {};
        if (QueryInformationJobObject(g_robocodeJob, JobObjectBasicAccountingInformation,
                                      &info, sizeof(info), nullptr) &&
            info.ActiveProcesses > 0) {
            restarted = true;
        }
        TerminateJobObject(g_robocodeJob, 0);
        CloseHandle(g_robocodeJob);
        g_robocodeJob = nullptr;
        if (restarted) Sleep(500);  // 閉じ終わるのを少し待つ
    }

    // robocode.bat は、PATH の中の java で Robocode を起動する。
    // アプリといっしょに入れた JDK があれば、その bin を PATH の先頭に足しておく
    // （このアプリから起動するものにだけ効く。パソコンの設定は変えない）
    //   → Java を入れていないパソコンでも、Robocode が起動できる
    std::wstring bundled = BundledJdkFolder();
    if (!bundled.empty()) {
        std::wstring bin = bundled + L"\\bin";
        DWORD size = GetEnvironmentVariableW(L"PATH", nullptr, 0);
        std::wstring path(size, L'\0');
        if (size > 0) path.resize(GetEnvironmentVariableW(L"PATH", &path[0], size));
        else path.clear();
        if (path.compare(0, bin.size() + 1, bin + L";") != 0) {   // まだ足していなければ
            SetEnvironmentVariableW(L"PATH", (bin + L";" + path).c_str());
        }
        SetEnvironmentVariableW(L"JAVA_HOME", bundled.c_str());
    }

    // robocode.bat を、Robocode のフォルダで実行する
    //   黒い画面（コンソール）は最小化して出す。起動できないときはそこにエラーが出る。
    std::wstring command = L"cmd.exe /c \"\"" + bat + L"\"\"";
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWMINNOACTIVE;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE | CREATE_SUSPENDED, nullptr,
                        robocodeFolder.c_str(), &si, &pi)) {
        message = "Robocode を起動できませんでした";
        return false;
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job != nullptr && !AssignProcessToJobObject(job, pi.hProcess)) {
        CloseHandle(job);
        job = nullptr;  // 入れ物に入れられなくても、起動はそのまま続ける
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    g_robocodeJob = job;

    message = restarted ? "Robocode を開き直しました" : "Robocode を起動しました";
    message += "（画面が出るまで少し時間がかかります）";
    return true;
}
