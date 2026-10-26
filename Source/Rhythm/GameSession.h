#pragma once
#include <string>

// 1回のプレイの結果
struct PlayResult
{
	std::string title;
	double achievement = 0.0;
	int score      = 0;
	int perfect    = 0;
	int great      = 0;
	int good       = 0;
	int bad = 0;
	int miss       = 0;
	int maxCombo   = 0;
	int totalNotes = 0;
};

// シーンをまたいで受け渡す情報（選曲 → ゲーム → リザルト）
//   static inline（C++17）なので cpp に実体を書かなくてよい
class GameSession
{
public:
	static inline std::string chartPath = "asset\\chart\\test.json";   // 遊ぶ譜面
	static inline PlayResult  result;                                   // 直前のプレイ結果

	static inline int songIndex = 0;	// 選曲画面で選んでいた曲(SongList::songs の番号)
	static inline int difficulty = 0;	// 選んでいた難易度(DIFFICULTY の値)
};
