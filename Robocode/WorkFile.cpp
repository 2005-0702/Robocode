// =====================================================================
//  WorkFile.cpp  ―  作品（組んだブロック）の保存と読み込み
// =====================================================================
#include "WorkFile.h"

#include <shobjidl.h>   // IFileSaveDialog / IFileOpenDialog（ファイルを選ぶ画面）
#include <fstream>
#include <sstream>

#pragma comment(lib, "ole32.lib")

// 作品ファイルの拡張子
static const wchar_t* const kExtension = L"rcblocks";

// 「ファイルの種類」に出す説明
static const COMDLG_FILTERSPEC kFileTypes[] = {
    {L"ロボットの作品 (*.rcblocks)", L"*.rcblocks"},
};

// ---------------------------------------------------------------------
//  文字コードの変換
// ---------------------------------------------------------------------
static std::wstring ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}

static std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], len, nullptr, nullptr);
    return s;
}

// 拡張子が .rcblocks か（ほかのファイルを上書きしないための確認）
static bool HasWorkFileExtension(const std::wstring& path) {
    const std::wstring ext = std::wstring(L".") + kExtension;
    if (path.size() < ext.size()) return false;
    return _wcsicmp(path.c_str() + path.size() - ext.size(), ext.c_str()) == 0;
}

// ファイルを選ぶ画面で選ばれた場所を取り出す
static std::wstring ResultPath(IFileDialog* dialog) {
    std::wstring result;
    IShellItem* item = nullptr;
    if (SUCCEEDED(dialog->GetResult(&item))) {
        PWSTR path = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            result = path;
            CoTaskMemFree(path);
        }
        item->Release();
    }
    return result;
}

// ---------------------------------------------------------------------
//  保存
// ---------------------------------------------------------------------
WorkFileResult SaveWorkFile(HWND owner, const std::string& path,
                            const std::string& suggestedName, const std::string& content) {
    WorkFileResult r;
    std::wstring target = ToWide(path);

    // 保存先が決まっていなければ「名前を付けて保存」の画面を出す
    if (target.empty()) {
        IFileSaveDialog* dialog = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&dialog)))) {
            r.error = "保存の画面を開けませんでした";
            return r;
        }
        dialog->SetFileTypes(ARRAYSIZE(kFileTypes), kFileTypes);
        dialog->SetDefaultExtension(kExtension);
        dialog->SetTitle(L"作品を保存");
        if (!suggestedName.empty()) dialog->SetFileName(ToWide(suggestedName).c_str());

        if (SUCCEEDED(dialog->Show(owner))) {
            target = ResultPath(dialog);
        }
        dialog->Release();

        if (target.empty()) {
            r.canceled = true;
            return r;
        }
    }

    if (!HasWorkFileExtension(target)) {
        r.error = "作品は .rcblocks のファイルにしか保存できません";
        return r;
    }

    std::ofstream out(target.c_str(), std::ios::binary);
    if (!out) {
        r.error = "保存できませんでした（その場所に書きこめない可能性があります）";
        return r;
    }
    out << content;
    out.close();
    if (!out) {
        r.error = "保存の途中で問題が起きました";
        return r;
    }

    r.ok = true;
    r.path = ToUtf8(target);
    return r;
}

// ---------------------------------------------------------------------
//  開く
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
//  Java のファイルを開く
// ---------------------------------------------------------------------
static const COMDLG_FILTERSPEC kJavaFileTypes[] = {
    {L"Java のファイル (*.java)", L"*.java"},
};

// 中身が UTF-8 として正しいか
static bool IsValidUtf8(const std::string& s) {
    if (s.empty()) return true;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), nullptr, 0) > 0;
}

WorkFileResult OpenJavaFile(HWND owner) {
    WorkFileResult r;

    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) {
        r.error = "開く画面を開けませんでした";
        return r;
    }
    dialog->SetFileTypes(ARRAYSIZE(kJavaFileTypes), kJavaFileTypes);
    dialog->SetDefaultExtension(L"java");
    dialog->SetTitle(L"ブロックにする Java を選ぶ");

    std::wstring target;
    if (SUCCEEDED(dialog->Show(owner))) {
        target = ResultPath(dialog);
    }
    dialog->Release();

    if (target.empty()) {
        r.canceled = true;
        return r;
    }

    std::ifstream in(target.c_str(), std::ios::binary);
    if (!in) {
        r.error = "ファイルを開けませんでした";
        return r;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    std::string content = ss.str();

    // 先頭の BOM（UTF-8 の印）を取りのぞく
    if (content.size() >= 3 && (unsigned char)content[0] == 0xEF && (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        content.erase(0, 3);
    }
    // UTF-8 でなければ、Shift-JIS（Windows の日本語）として読んで UTF-8 にする
    if (!IsValidUtf8(content)) {
        int len = MultiByteToWideChar(CP_ACP, 0, content.c_str(), (int)content.size(), nullptr, 0);
        std::wstring w(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, content.c_str(), (int)content.size(), &w[0], len);
        content = ToUtf8(w);
    }

    r.ok = true;
    r.path = ToUtf8(target);
    r.content = content;
    return r;
}

WorkFileResult OpenWorkFile(HWND owner) {
    WorkFileResult r;

    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) {
        r.error = "開く画面を開けませんでした";
        return r;
    }
    dialog->SetFileTypes(ARRAYSIZE(kFileTypes), kFileTypes);
    dialog->SetDefaultExtension(kExtension);
    dialog->SetTitle(L"作品を開く");

    std::wstring target;
    if (SUCCEEDED(dialog->Show(owner))) {
        target = ResultPath(dialog);
    }
    dialog->Release();

    if (target.empty()) {
        r.canceled = true;
        return r;
    }

    std::ifstream in(target.c_str(), std::ios::binary);
    if (!in) {
        r.error = "ファイルを開けませんでした";
        return r;
    }
    std::stringstream ss;
    ss << in.rdbuf();

    r.ok = true;
    r.path = ToUtf8(target);
    r.content = ss.str();
    return r;
}
