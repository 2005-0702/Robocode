// RobocodeBlocks.cpp
// アプリのウィンドウを作り、その中に WebView2 で Blockly のページ（web/index.html）を表示する

#define NOMINMAX
#include <windows.h>
#include <memory>
#include <string>
#include <wrl.h>
#include "WebView2.h"
#include "RobocodeBlocks.h"   // ブロックの仕組み（ROBOCODE_BLOCKS で書いたブロックは自動で登録される）
#include "RobotBuilder.h"     // Java の保存と javac でのコンパイル
#include "WorkFile.h"         // 作品（組んだブロック）の保存と読み込み
#include <nlohmann/json.hpp>  // Blockly とやり取りする JSON を読み書きする

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
using json = nlohmann::json;

static ComPtr<ICoreWebView2Controller> g_controller;  // WebView2 の「入れ物」（大きさや表示を管理）
static ComPtr<ICoreWebView2> g_webview;               // WebView2 本体（ページの表示や読み込み）
static HWND g_hWnd = nullptr;                          // アプリのウィンドウ（ダイアログを出すときに使う）

// Blockly から届いたメッセージを、あとでウィンドウ側で処理するための合図
//   WebView2 のイベントの中でダイアログ（保存・開く画面など）を出すと、例外になることがあるので、
//   いったんウィンドウに預けて、イベントが終わってから処理する
static const UINT WM_APP_BLOCKLY_MESSAGE = WM_APP + 1;

// ------------------------------------------------------------
// web フォルダを探す
//   配布時：exe と同じ場所の web フォルダ
//   開発中：exe（x64\Debug など）から上のフォルダへさかのぼって、.sln の横にある web フォルダ
// ------------------------------------------------------------
static bool FileExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring FindWebFolder() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring dir(buf);
    dir = dir.substr(0, dir.find_last_of(L"\\/"));  // exe のあるフォルダ

    for (int i = 0; i < 5; ++i) {
        std::wstring candidate = dir + L"\\web";
        if (FileExists(candidate + L"\\index.html")) {
            return candidate;
        }
        size_t pos = dir.find_last_of(L"\\/");
        if (pos == std::wstring::npos) break;
        dir = dir.substr(0, pos);  // 1つ上のフォルダへ
    }
    return L"";
}

// UTF-8の文字列を、WebView2で使うワイド文字列に変換する
static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}

// WebView2 のワイド文字列を、UTF-8 の文字列に変換する
static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], len, nullptr, nullptr);
    return s;
}

// ブロック定義とツールボックスをBlocklyへ送る
//   送る形：{ "kind": "blocks", "setup": { "blocks": [...], "toolbox": {...}, "problems": [...] } }
static void SendBlocksToBlockly(ICoreWebView2* webview) {
    std::string msg = "{\"kind\":\"blocks\",\"setup\":" + rb::GetBlocklySetupJson() + "}";
    webview->PostWebMessageAsJson(Utf8ToWide(msg).c_str());
}

// エラー・注意の一覧を JSON にする
static json MessagesToJson(const std::vector<rb::Message>& list) {
    json array = json::array();
    for (const auto& m : list) {
        array.push_back({{"blockId", m.blockId}, {"label", m.blockLabel}, {"text", m.text}});
    }
    return array;
}

// ------------------------------------------------------------
// Blockly から届いたメッセージを処理して、返事の JSON を返す（返事がないときは ""）
//
//   「Java を作る」ボタン
//     届く形：{ "kind": "convert", "name": "MyRobot", "package": "student", "workspace": {...} }
//     返す形：{ "kind": "java", "ok": true, "java": "...", "errors": [...], "warnings": [...],
//               "build": { "ok": true, "message": "...", "detail": "...", "javaPath": "..." } }
//
//   「保存」「別名で保存」ボタン
//     届く形：{ "kind": "save", "path": "（上書きするときだけ）", "name": "MyRobot",
//               "package": "student", "workspace": {...} }
//     返す形：{ "kind": "saved", "ok": true, "canceled": false, "path": "...", "error": "" }
//
//   「開く」ボタン
//     届く形：{ "kind": "open" }
//     返す形：{ "kind": "opened", "ok": true, "canceled": false, "path": "...", "error": "",
//               "work": { "robotName": "...", "package": "...", "workspace": {...} } }
//
//   「Robocode の場所」ボタン
//     届く形：{ "kind": "chooseFolder" }
//     返す形：{ "kind": "folder", "path": "C:\\robocode" }（キャンセルなら ""）
// ------------------------------------------------------------
static std::string HandleMessageFromBlockly(const std::string& text) {
    json msg = json::parse(text, nullptr, false);
    if (msg.is_discarded() || !msg.is_object()) return "";

    const std::string kind = msg.value("kind", "");
    json reply;

    // 「Java を作る」ボタン
    if (kind == "convert") {
        const std::string name = msg.value("name", "");
        const std::string packageName = msg.value("package", "student");
        const std::string workspace = msg.contains("workspace") ? msg["workspace"].dump() : "{}";

        // 0. 名前のチェック（だめなら Java を作らずに理由を返す）
        std::string nameError = CheckPackageName(packageName);
        if (nameError.empty()) nameError = CheckRobotName(name);

        // 1. ブロックを Java にする
        rb::JavaResult result = rb::ConvertToJava(workspace, name, packageName);
        if (!nameError.empty()) {
            rb::Message m;
            m.text = nameError;
            result.errors.insert(result.errors.begin(), m);
            result.ok = false;
        }

        reply["kind"] = "java";
        reply["ok"] = result.ok;
        reply["java"] = result.java;
        reply["errors"] = MessagesToJson(result.errors);
        reply["warnings"] = MessagesToJson(result.warnings);

        // 2. エラーがなければ、保存して javac でコンパイルする
        if (result.ok) {
            json build;
            std::wstring folder = GetRobocodeFolder(g_hWnd);
            if (folder.empty()) {
                build["ok"] = false;
                build["message"] = "Robocode のフォルダが選ばれなかったので、保存していません";
                build["detail"] = "";
                build["javaPath"] = "";
            } else {
                BuildResult b = BuildRobot(folder, packageName, name, result.java);
                build["ok"] = b.ok;
                build["message"] = b.message;
                build["detail"] = b.detail;
                build["javaPath"] = b.javaPath;
            }
            reply["build"] = build;
        }
    }

    // 「保存」「別名で保存」ボタン
    else if (kind == "save") {
        json work;
        work["format"] = "robocode-blocks";
        work["version"] = 1;
        work["robotName"] = msg.value("name", "");
        work["package"] = msg.value("package", "student");
        work["workspace"] = msg.contains("workspace") ? msg["workspace"] : json::object();

        const std::string suggested = msg.value("name", "");
        WorkFileResult r = SaveWorkFile(g_hWnd, msg.value("path", ""),
                                        suggested.empty() ? "MyRobot" : suggested,
                                        work.dump(2, ' ', false, json::error_handler_t::replace));
        reply["kind"] = "saved";
        reply["ok"] = r.ok;
        reply["canceled"] = r.canceled;
        reply["path"] = r.path;
        reply["error"] = r.error;
    }

    // 「開く」ボタン
    else if (kind == "open") {
        WorkFileResult r = OpenWorkFile(g_hWnd);
        reply["kind"] = "opened";
        reply["ok"] = r.ok;
        reply["canceled"] = r.canceled;
        reply["path"] = r.path;
        reply["error"] = r.error;

        if (r.ok) {
            json work = json::parse(r.content, nullptr, false);
            if (work.is_discarded() || !work.is_object() || work.value("format", "") != "robocode-blocks") {
                reply["ok"] = false;
                reply["error"] = "このファイルは作品のファイルではないか、こわれています";
            } else {
                reply["work"] = work;
            }
        }
    }

    // 「Robocode の場所」ボタン
    else if (kind == "chooseFolder") {
        reply["kind"] = "folder";
        reply["path"] = WideToUtf8(GetRobocodeFolder(g_hWnd, true));
    }

    else {
        return "";
    }

    return reply.dump(-1, ' ', false, json::error_handler_t::replace);
}

// ウィンドウの大きさに WebView2 を合わせる
static void ResizeWebView(HWND hWnd) {
    if (g_controller) {
        RECT bounds;
        GetClientRect(hWnd, &bounds);
        g_controller->put_Bounds(bounds);
    }
}

// WebView2 を作って Blockly のページを開く
static void InitWebView(HWND hWnd) {
    std::wstring webFolder = FindWebFolder();
    if (webFolder.empty()) {
        MessageBoxW(hWnd,
            L"web フォルダ（index.html）が見つかりません。\n.sln ファイルと同じ場所に web フォルダがあるか確認してください。",
            L"エラー", MB_ICONERROR);
        return;
    }

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd, webFolder](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || env == nullptr) {
                    MessageBoxW(hWnd,
                        L"WebView2 を起動できませんでした。\nWebView2 ランタイムがインストールされているか確認してください。",
                        L"エラー", MB_ICONERROR);
                    return result;
                }

                env->CreateCoreWebView2Controller(hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hWnd, webFolder](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || controller == nullptr) {
                                return result;
                            }
                            g_controller = controller;
                            g_controller->get_CoreWebView2(&g_webview);
                            ResizeWebView(hWnd);

                            // web フォルダを「https://app.local/」というアドレスに割り当てる
                            // （file:// で開くより制限が少なく、あとで JSON ファイルの読み込みなどもできる）
                            ComPtr<ICoreWebView2_3> webview3;
                            if (SUCCEEDED(g_webview.As(&webview3))) {
                                webview3->SetVirtualHostNameToFolderMapping(
                                    L"app.local", webFolder.c_str(),
                                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                            }
                            EventRegistrationToken navToken;
                            g_webview->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                                        SendBlocksToBlockly(sender);
                                        return S_OK;
                                    }).Get(), &navToken);

                            // Blockly からメッセージが届いたら、ウィンドウに預ける（処理は WndProc で行う）
                            EventRegistrationToken msgToken;
                            g_webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR raw = nullptr;
                                        if (SUCCEEDED(args->get_WebMessageAsJson(&raw)) && raw != nullptr) {
                                            std::string* text = new std::string(WideToUtf8(raw));
                                            CoTaskMemFree(raw);
                                            if (!PostMessageW(g_hWnd, WM_APP_BLOCKLY_MESSAGE, 0,
                                                              reinterpret_cast<LPARAM>(text))) {
                                                delete text;
                                            }
                                        }
                                        return S_OK;
                                    }).Get(), &msgToken);

                            g_webview->Navigate(L"https://app.local/index.html");
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        MessageBoxW(hWnd,
            L"WebView2 を起動できませんでした。\nWebView2 ランタイムがインストールされているか確認してください。",
            L"エラー", MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        ResizeWebView(hWnd);
        return 0;

    // Blockly から届いたメッセージを処理して、返事を送り返す
    case WM_APP_BLOCKLY_MESSAGE: {
        std::unique_ptr<std::string> text(reinterpret_cast<std::string*>(lParam));
        std::string reply;
        try {
            reply = HandleMessageFromBlockly(*text);
        } catch (const std::exception& ex) {
            // 予想していない問題が起きても、アプリを止めずに画面に知らせる
            json error = {{"kind", "error"},
                          {"message", std::string("アプリの中で問題が起きました：") + ex.what()}};
            reply = error.dump(-1, ' ', false, json::error_handler_t::replace);
        }
        if (!reply.empty() && g_webview) {
            g_webview->PostWebMessageAsJson(Utf8ToWide(reply).c_str());
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) {
    // WebView2 を使うための準備
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RobocodeBlocksWindow";
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowW(wc.lpszClassName, L"Robocode Blocks",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) {
        return 1;
    }
    g_hWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    InitWebView(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_controller) {
        g_controller->Close();
    }
    g_webview.Reset();
    g_controller.Reset();
    CoUninitialize();
    return (int)msg.wParam;
}
