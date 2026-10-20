#include "Settings.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <algorithm>  

static const char* SETTINGS_PATH = "settings.json";

void Settings::Load()
{
	std::ifstream ifs(SETTINGS_PATH);
	if (!ifs) return;                           // まだファイルが無ければ初期値のまま

	nlohmann::json j = nlohmann::json::parse(ifs, nullptr, false);
	if (j.is_discarded()) return;               // 壊れていたら初期値のまま

	noteSpeed = j.value("noteSpeed", noteSpeed);
	offsetMs  = j.value("offsetMs",  offsetMs);
	fullscreen = j.value("fullscreen", fullscreen);

	masterVolume = std::clamp(j.value("masterVolume", masterVolume), 0, 100);
	musicVolume = std::clamp(j.value("musicVolume", musicVolume), 0, 100);
	seVolume = std::clamp(j.value("seVolume", seVolume), 0, 100);

	visualOffsetMs = j.value("visualOffsetMs", visualOffsetMs);
	layerGuide = j.value("layerGuide", layerGuide);
	fastSlow = j.value("fastSlow", fastSlow);
	showFps = j.value("showFps", showFps);
}

void Settings::Save()
{
	nlohmann::json j;
	j["noteSpeed"] = noteSpeed;
	j["offsetMs"]  = offsetMs;
	j["fullscreen"] = fullscreen;

	j["masterVolume"] = masterVolume;
	j["musicVolume"] = musicVolume;
	j["seVolume"] = seVolume;

	j["visualOffsetMs"] = visualOffsetMs;
	j["layerGuide"] = layerGuide;
	j["fastSlow"] = fastSlow;
	j["showFps"] = showFps;

	std::ofstream ofs(SETTINGS_PATH);
	if (ofs) ofs << j.dump(2);
}
