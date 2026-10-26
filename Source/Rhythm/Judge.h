#pragma once

// 判定の種類(全ノーツ共通)
enum class Judge
{
	None,      // まだ判定していない
	Perfect,
	Great,
	Good,
	Bad,       // BAD を足して5段階(ノーツ仕様で決定済み)
	Miss,
	Skip,      // 途中から再生したときに飛ばしたノーツ(数えない)
	Max,
};

// 判定の幅(秒)。ずれがこれ以内ならその判定
constexpr double WINDOW_PERFECT = 0.033;
constexpr double WINDOW_GREAT = 0.066;
constexpr double WINDOW_GOOD = 0.100;
constexpr double WINDOW_BAD = 0.133;