// =====================================================================
//  RobotAdvancedMove.cpp ― 「まとめて動く」のブロック
//
// 2026/10/28 片山　攻撃パワーの範囲チェックと最高速度の変数対応を追加
//
// 【このカテゴリの使い方】
// 1. 移動や回転、攻撃などの動作を指示する
// 2. run() の繰り返し処理で execute() を呼び、指示した動作を進める
// 3. 必要に応じて、残りの距離や角度を調べたり、動作の完了を待ったりする
//
// 注意：数値を直接入力した場合は、ブロックのチェックで範囲を確認する。
// 変数を接続した場合、その変数に実際に入っている値はブロック配置時には
// 分からないため、ブロックのチェックだけでは範囲を判定できない。
// =====================================================================

#include "RobocodeBlocks.h"

#include <cstdlib>

namespace
{
    constexpr double MIN_FIRE_POWER = 0.1;
    constexpr double MAX_FIRE_POWER = 3.0;

    constexpr double MIN_MAX_VELOCITY = 0.0;
    constexpr double MAX_MAX_VELOCITY = 8.0;

    // 入力が数値そのものの場合だけ範囲をチェックする。
    // 変数名などの式はここでは数値に変換できないため、実行時の値は判定しない。
    bool TryParseNumber(const std::string& text, double& value)
    {
        char* end = nullptr;
        value = std::strtod(text.c_str(), &end);
        return end != text.c_str() && *end == '\0';
    }
}

ROBOCODE_BLOCKS(RobotAdvancedMove)
{
    //-----------------------------
    // 移動・回転・攻撃の指示
    //-----------------------------

    // 進む／下がる動きを指示する
    Block("adv_set_ahead")
        .category("まとめて動く")
        .text("動きを決める：%1 %2")
        .valueInput("DIST", "Number")
        .dropdown("DIR", {
            {"進む", "setAhead"},
            {"下がる", "setBack"}
        })
        .tooltip("進む（下がる）動きを先に決めます\n"
            "最後に「まとめて動かす」を置くと、動作を進めます")
        .java("{DIR}({DIST});");

    // 車体・砲塔・レーダーの回転を指示する
    Block("adv_set_turn")
        .category("まとめて動く")
        .text("回転を決める：%1 を %2 に %3 度")
        .dropdown("PART", {
            {"車体", "setTurn"},
            {"砲塔", "setTurnGun"},
            {"レーダー", "setTurnRadar"}
        })
        .dropdown("DIR", {
            {"右", "Right"},
            {"左", "Left"}
        })
        .valueInput("ANGLE", "Number")
        .tooltip("車体・砲塔・レーダーの回転を先に決めます\n"
            "最後に「まとめて動かす」を置くと動作を進めます")
        .java("{PART}{DIR}({ANGLE});");

    // 撃つパワーを指示する（直接入力した数値は 0.1～3.0 をチェック）
    Block("adv_set_fire")
        .category("まとめて動く")
        .text("撃つパワーを決める：%1")
        .valueInput("POWER", "Number")
        .tooltip("弾を撃つパワーを先に決めます（0.1〜3)\n"
            "最後に「まとめて動かす」を置くと動作を進めます")
        .check([](BlockData& b) {
            const std::string text = b.text("POWER");
            double value = 0.0;
            if (TryParseNumber(text, value) &&
                (value < MIN_FIRE_POWER || value > MAX_FIRE_POWER)) {
                b.error("発射パワーは0.1～3.0にしてください");
            }
        })
        .java("setFire({POWER});");

    // 指示した動作を進める
    Block("adv_execute")
        .category("まとめて動く")
        .text("まとめて動かす")
        .tooltip("先に決めた移動・回転・攻撃を実行します\n"
            "動きを決めるブロックのあとに置きます\n"
            "基本的に 「スタートしたら、ずっと」 の繰り返し処理で使用します。")
        .java("execute();");

    //-----------------------------
    // 最高速度の設定
    //-----------------------------

    // 数字ブロックだけでなく、変数などの数値ブロックも接続できる
    Block("adv_set_max_velocity")
        .category("まとめて動く")
        .text("最高速度を %1 にする")
        .valueInput("SPEED", "Number")
        .tooltip("ロボットが進む最高速度を決めます（0〜8。普通は 8）\n"
                 "変数を使う場合は、変数に入れる値も 0～8 にしてください。")
        .check([](BlockData& b) {
            const std::string text = b.text("SPEED");
            double value = 0.0;
            if (TryParseNumber(text, value) &&
                (value < MIN_MAX_VELOCITY || value > MAX_MAX_VELOCITY)) {
                b.error("最高速度は0～8にしてください");
            }
        })
        .java("setMaxVelocity({SPEED});");

    //-----------------------------
    // 動作の残りを調べる
    //-----------------------------

    Block("adv_remaining")
        .category("まとめて動く")
        .text("残りの量を調べる：%1")
        .dropdown("WHAT", {
            {"進む距離", "getDistanceRemaining"},
            {"車体の回転", "getTurnRemaining"},
            {"砲塔の回転", "getGunTurnRemaining"},
            {"レーダーの回転", "getRadarTurnRemaining"}
        })
        .output("Number")
        .tooltip("決めておいた移動や回転が、あとどれだけ残っているかを調べます\n"
            "動作が進むにつれて残りの値は減り、完了すると通常は0になります。")
        .java("{WHAT}()");

    //-----------------------------
    // 動作が終わるまで待つ
    //-----------------------------

    Block("adv_wait_for")
        .category("まとめて動く")
        .text("%1 まで待つ")
        .dropdown("COND", {
            {"移動が終わる", "MoveCompleteCondition"},
            {"車体が回り終わる", "TurnCompleteCondition"},
            {"砲塔が回り終わる", "GunTurnCompleteCondition"},
            {"レーダーが回り終わる", "RadarTurnCompleteCondition"}
        })
        .tooltip("選んだ移動や回転が終わるまで、次の処理に進むの待ちます\n"
            "待っている間も、すでに指示した動作は進みます\n"
            "複数の動作がある場合、選んだ条件の動作が終わると待機を終了します")
        .java("waitFor(new {COND}(this));");
}
