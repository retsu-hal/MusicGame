#include "Settings.h"
#include "nlohmann/json.hpp"
#include <fstream>

static const char* SETTINGS_PATH = "settings.json";

void Settings::Load()
{
	std::ifstream ifs(SETTINGS_PATH);
	if (!ifs) return;                           // まだファイルが無ければ初期値のまま

	nlohmann::json j = nlohmann::json::parse(ifs, nullptr, false);
	if (j.is_discarded()) return;               // 壊れていたら初期値のまま

	noteSpeed = j.value("noteSpeed", noteSpeed);
	offsetMs  = j.value("offsetMs",  offsetMs);
}

void Settings::Save()
{
	nlohmann::json j;
	j["noteSpeed"] = noteSpeed;
	j["offsetMs"]  = offsetMs;

	std::ofstream ofs(SETTINGS_PATH);
	if (ofs) ofs << j.dump(2);
}
