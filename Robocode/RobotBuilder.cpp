// =====================================================================
//  RobotBuilder.cpp  ―  できた Java を Robocode で使えるようにする
// =====================================================================
#include "RobotBuilder.h"

#include <shlobj.h>     // SHGetKnownFolderPath（設定の保存場所）
#include <shobjidl.h>   // IFileOpenDialog（フォルダを選ぶ画面）
#include <fstream>
#include <sstream>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// Java の package 名（RobocodeBlocksCore.cpp の kJavaPackage と同じにする）
static const wchar_t* const kPackageFolder = L"student";

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
static std::wstring FindJavac() {
    wchar_t buf[MAX_PATH];

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

// Java のクラス名に使える名前か（英字で始まり、英数字と _ だけ）
static bool IsValidRobotName(const std::string& name) {
    if (name.empty() || name.size() > 60) return false;
    if (!((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= 'a' && name[0] <= 'z'))) return false;
    for (char c : name) {
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

// ---------------------------------------------------------------------
//  保存してコンパイルする
// ---------------------------------------------------------------------
BuildResult BuildRobot(const std::wstring& robocodeFolder,
                       const std::string& robotName,
                       const std::string& javaCode) {
    BuildResult r;

    if (!IsValidRobotName(robotName)) {
        r.message = "ロボットの名前は、英字で始めて、英字・数字・_ だけで書いてください";
        return r;
    }

    // 1. robots\student\ロボット名.java に保存する
    const std::wstring robotsDir = robocodeFolder + L"\\robots";
    const std::wstring packageDir = robotsDir + L"\\" + kPackageFolder;
    CreateDirectoryW(robotsDir.c_str(), nullptr);
    CreateDirectoryW(packageDir.c_str(), nullptr);

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
    //   javac -encoding UTF-8 -cp "<Robocode>\libs\robocode.jar" -d "<Robocode>\robots" "<.java>"
    const std::wstring command =
        L"\"" + javac + L"\" -encoding UTF-8 --release 8"     // ← --release 8 を足す
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
    r.message = "Robocode で使えるようになりました（" + robotName + "）";
    r.detail = output;  // 警告が出ていればここに入る
    return r;
}
