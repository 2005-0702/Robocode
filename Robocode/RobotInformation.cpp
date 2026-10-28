// =====================================================================
//  RobotInformation.cpp ― 「ゲーム情報」のブロック
//
// 2026/10/28 片山　ゲーム情報ブロックの説明・取得内容を整理
//
// ・自分のロボットの情報
// ・スキャンした敵の情報
// ・弾やバトル場の情報
// ・ラウンドの情報
//
// ※敵の情報を取得するブロックは、敵をスキャンしたときのイベント
//   （onScannedRobot）で使うことを想定しています。
// =====================================================================

#include "RobocodeBlocks.h"

ROBOCODE_BLOCKS(RobotInformation)
{
    //-----------------------------
    // 自分のロボットの情報
    //-----------------------------

    // 自分のロボットの名前
    Block("robot_name")
        .category("ゲーム情報")
        .text("自分の名前")
        .output("String")
        .tooltip("自分のロボットに設定されている名前を取得します")
        .java("getName()");

    // コンソールに文字や数値を表示する
    Block("robot_print")
        .category("ゲーム情報")
        .text("%1 を表示する")
        .valueInput("TEXT", "")
        .tooltip("指定した文字や数値をRobocodeのコンソールに表示します")
        .java("out.println({TEXT});");

    // 自分のX座標・Y座標
    Block("robot_get_position")
        .category("ゲーム情報")
        .text("自分の %1 座標")
        .dropdown("AXIS", {
            {"X（左右）", "X"},
            {"Y（上下）", "Y"}
        })
        .output("Number")
        .tooltip("バトル場での自分の位置を取得します")
        .java("get{AXIS}()");

    // 自分の車体・砲塔・レーダーの向き
    Block("robot_heading")
        .category("ゲーム情報")
        .text("自分の %1 の向き")
        .dropdown("PART", {
            {"車体", "getHeading"},
            {"砲塔", "getGunHeading"},
            {"レーダー", "getRadarHeading"}
        })
        .output("Number")
        .tooltip("自分の車体・砲塔・レーダーが向いている角度を取得します\n"
                 "画面の上が0度で、時計回りに角度が大きくなります")
        .java("{PART}()");

    // 自分のエネルギー残量
    Block("robot_energy")
        .category("ゲーム情報")
        .text("自分のエネルギー残量")
        .output("Number")
        .tooltip("自分のロボットに残っているエネルギーを取得します\n"
                 "攻撃などでエネルギーが減ると、この値も小さくなります")
        .java("getEnergy()");

    // 砲塔の冷却速度
    Block("robot_gun_cooltime")
        .category("ゲーム情報")
        .text("砲塔の冷却速度")
        .output("Number")
        .tooltip("砲塔の温度が下がる速さを取得します")
        .java("getGunCoolingRate()");

    // 砲塔の現在の温度
    Block("robot_gun_heat")
        .category("ゲーム情報")
        .text("砲塔の温度")
        .output("Number")
        .tooltip("砲塔の現在の温度を取得します\n"
                 "温度が0になると、通常は再び弾を撃てる状態になります")
        .java("getGunHeat()");

    // 自分の移動速度
    Block("robot_velocity")
        .category("ゲーム情報")
        .text("自分の速度")
        .output("Number")
        .tooltip("自分のロボットの現在の移動速度を取得します\n"
                 "止まっているときは0になります")
        .java("getVelocity()");

    //-----------------------------
    // 敵の情報
    //-----------------------------

    // 残っている敵の数
    Block("others")
        .category("ゲーム情報")
        .text("残りの敵の数")
        .output("Number")
        .tooltip("自分以外に残っているロボットの数を取得します")
        .java("getOthers()");

    // スキャンした敵との距離
    Block("enemy_distance")
        .category("ゲーム情報")
        .text("敵との距離")
        .output("Number")
        .tooltip("敵をスキャンしたときに、その敵までの距離を取得します\n"
                 "敵の情報を受け取れるイベント内で使用してください")
        .java("e.getDistance()");

    // スキャンした敵がいる方向
    Block("enemy_Bearing")
        .category("ゲーム情報")
        .text("敵がいる方向")
        .output("Number")
        .tooltip("自分の車体の正面から見て、敵がどちらにいるかを角度で取得します\n"
                 "右側はプラス、左側はマイナスの角度になります\n"
                 "敵をスキャンしたときのイベント内で使用してください")
        .java("e.getBearing()");

    // スキャンした敵の車体の向き
    Block("enemy_heading")
        .category("ゲーム情報")
        .text("敵の向き")
        .output("Number")
        .tooltip("スキャンした敵の車体が向いている角度を取得します。\n"
                 "画面の上が0度で、時計回りに角度が大きくなります。\n"
                 "敵をスキャンしたときのイベント内で使用してください。")
        .java("e.getHeading()");

    // スキャンした敵の速度
    Block("enemy_velocity")
        .category("ゲーム情報")
        .text("敵の速度")
        .output("Number")
        .tooltip("スキャンした敵の移動速度を取得します\n"
                 "止まっている敵の速度は0です\n"
                 "敵をスキャンしたときのイベント内で使用してください")
        .java("e.getVelocity()");

    //-----------------------------
    // 弾の情報
    //-----------------------------

    // 指定したパワーの弾の速度
    Block("rule_bullet_speed")
        .category("ゲーム情報")
        .text("パワー %1 の弾の速さ")
        .valueInput("POWER", "Number")
        .output("Number")
        .tooltip("指定した発射パワーの弾の速さを取得します\n"
                 "発射パワーは0.1～3.0の範囲で指定します\n"
                 "Robocodeでは、弾の速さは「20 - 3 × 発射パワー」で計算されます\n"
                 "発射パワーが大きいほど、弾は遅くなります")
        .java("Rules.getBulletSpeed({POWER})");

    // 砲塔がすぐに撃てるか
    Block("gun_ready")
        .category("ゲーム情報")
        .text("砲塔がすぐ撃てる")
        .output("Boolean")
        .tooltip("砲塔の温度が0のとき「はい」になります\n"
                 "温度が0でないときは「いいえ」になります")
        .java("(getGunHeat() == 0)");

    //-----------------------------
    // バトル場・ロボットの大きさ
    //-----------------------------

    // バトル場の幅・高さ
    Block("battlefield_size")
        .category("ゲーム情報")
        .text("バトル場の %1")
        .dropdown("SIZE", {
            {"横の長さ（幅）", "Width"},
            {"縦の長さ（高さ）", "Height"}
        })
        .output("Number")
        .tooltip("バトル場の幅または高さを取得します\n"
                 "（普通は 幅 800、高さ 600）\n")
        .java("getBattleField{SIZE}()");

    // 自分のロボットの幅・高さ
    Block("robot_size")
        .category("ゲーム情報")
        .text("ロボットの %1")
        .dropdown("SIZE", {
            {"横の長さ（幅）", "Width"},
            {"縦の長さ（高さ）", "Height"}
        })
        .output("Number")
        .tooltip("自分のロボットの幅または高さを取得します")
        .java("get{SIZE}()");

    //-----------------------------
    // ラウンド・経過時間の情報
    //-----------------------------

    // バトルで設定されている総ラウンド数
    Block("num_rounds")
        .category("ゲーム情報")
        .text("総ラウンド数")
        .output("Number")
        .tooltip("バトルで設定されているラウンドの合計数を取得します")
        .java("getNumRounds()");

    // 現在のラウンド番号（1から表示）
    Block("round_num")
        .category("ゲーム情報")
        .text("現在のラウンド数")
        .output("Number")
        .tooltip("現在のラウンドが何回目かを取得します")
        .java("getRoundNum() + 1");

    // ラウンド開始からの経過時間
    Block("time")
        .category("ゲーム情報")
        .text("ラウンド開始からの経過時間")
        .output("Number")
        .tooltip("現在のラウンドが始まってからの経過時間を取得します")
        .java("getTime()");
}
