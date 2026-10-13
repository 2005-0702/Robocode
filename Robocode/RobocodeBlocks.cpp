// RobocodeBlocks.cpp
// アプリのウィンドウを作り、その中に WebView2 で Blockly のページ（web/index.html）を表示する

#include <windows.h>
#include <string>
#include <wrl.h>
#include "WebView2.h"

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

static ComPtr<ICoreWebView2Controller> g_controller;  // WebView2 の「入れ物」（大きさや表示を管理）
static ComPtr<ICoreWebView2> g_webview;               // WebView2 本体（ページの表示や読み込み）

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
