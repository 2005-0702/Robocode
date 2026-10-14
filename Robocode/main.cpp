// =====================================================================
//  main.cpp  ―― 動作確認用
//  ブロック定義・ツールボックス・Javaコード・チェック結果を表示します。
//  アプリ本体ができたら、この中身は本体側へ移してください。
// =====================================================================
#include <iostream>

#include "Block.h"
#include "RobotMove.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // コンソールで日本語が文字化けしないように
#endif

    // ---- ブロックを登録する ----
    BlockRegistry registry;
    // 新しいブロックのファイルを作ったら、ここに1行足す
    //   例: registerRobotGunBlocks(registry);

    // ---- 1. Blockly に渡すブロック定義とツールボックス ----
    std::cout << "=== ブロック定義 ===\n" << registry.definitions().dump(2) << "\n\n";
    std::cout << "=== ツールボックス ===\n" << registry.toolbox().dump(2) << "\n\n";

    // ---- 2. Blockly から届いたブロック（例）を Java にする ----
    // Blockly.serialization.workspaces.save() で書き出したJSONと同じ形
    const json workspace = json::parse(R"({
      "blocks": {
        "languageVersion": 0,
        "blocks": [
          {
            "type": "robot_move_forward", "id": "blk1", "x": 20, "y": 20,
            "fields": { "DIST": 100 },
            "next": { "block": {
              "type": "robot_move_backward", "id": "blk2",
              "fields": { "DIST": 50 },
              "next": { "block": {
                "type": "robot_move_forward", "id": "blk3",
                "fields": { "DIST": 0 }
              }}
            }}
          }
        ]
      }
    })");

    Generator gen(registry);
    const json& firstBlock = workspace["blocks"]["blocks"][0];
    const std::string body = gen.statements(firstBlock, 3);

    // クラスの骨組みは仮のもの（本番はコード生成担当が作る）
    std::cout << "=== Java ===\n"
              << "package mybots;\n"
              << "import robocode.*;\n\n"
              << "public class MyRobot extends Robot {\n"
              << "    public void run() {\n"
              << "        while (true) {\n"
              << body
              << "        }\n"
              << "    }\n"
              << "}\n\n";

    // ---- 3. チェック結果（Blockly 側へ返す） ----
    json result = json::array();
    for (const auto& m : gen.messages()) {
        result.push_back(m.toJson());
    }
    std::cout << "=== チェック結果 ===\n" << result.dump(2) << "\n";
    std::cout << (gen.hasError() ? "エラーがあるので実行しません\n" : "実行できます\n");

    return 0;
}
