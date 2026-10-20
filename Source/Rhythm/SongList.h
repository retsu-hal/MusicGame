#pragma once
#include <string>
#include <vector>

// 難易度
enum DIFFICULTY
{
	DIFFICULTY_EASY,
	DIFFICULTY_HARD,

	DIFFICULTY_MAX
};

// 1曲ぶんの情報(songs.json の "songs" の1つ)
struct SongInfo
{
	std::string  chartPath[DIFFICULTY_MAX];	// 難易度ごとの譜面

	std::wstring titleImage;				// 曲名の画像
	float        titleWidth = 0.0f;			// 曲名の画像の元のピクセル数
	float        titleHeight = 0.0f;

	std::wstring artistImage;				// アーティスト名の画像
	float        artistWidth = 0.0f;
	float        artistHeight = 0.0f;

	std::wstring jacket;					// ジャケットの画像
	double       previewStart = 0.0;		// プレビュー再生を始める位置(秒)
};

// 収録曲の一覧(songs.json から読む)
class SongList
{
public:
	static inline std::vector<SongInfo> songs;

	static bool Load(const std::string& path);	// 読めて1曲以上あれば true
};