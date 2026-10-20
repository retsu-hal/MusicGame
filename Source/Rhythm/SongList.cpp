#include "SongList.h"
#include "nlohmann/json.hpp"
#include <fstream>
using json = nlohmann::json;

namespace
{
	// パスの string を、Polygon2D に渡せる wstring にする(パスは英数字と記号だけの前提)
	std::wstring ToWide(const std::string& s)
	{
		return std::wstring(s.begin(), s.end());
	}
}

bool SongList::Load(const std::string& path)
{
	songs.clear();					// 2回目に呼ばれたときに曲が倍にならないように

	std::ifstream ifs(path);
	if (!ifs) return false;

	json j = json::parse(ifs, nullptr, false);
	if (j.is_discarded()) return false;

	// "songs" の配列を1つずつ見る(無ければ空の配列として扱う)
	for (const json& s : j.value("songs", json::array()))
	{
		SongInfo info;
		info.chartPath[DIFFICULTY_EASY] = s.value("easy", "");
		info.chartPath[DIFFICULTY_HARD] = s.value("hard", "");

		info.titleImage = ToWide(s.value("titleImage", ""));
		info.titleWidth = s.value("titleWidth", 0.0f);
		info.titleHeight = s.value("titleHeight", 0.0f);

		info.artistImage = ToWide(s.value("artistImage", ""));
		info.artistWidth = s.value("artistWidth", 0.0f);
		info.artistHeight = s.value("artistHeight", 0.0f);

		info.jacket = ToWide(s.value("jacket", ""));
		info.previewStart = s.value("previewStart", 0.0);

		songs.push_back(info);
	}

	return !songs.empty();
}