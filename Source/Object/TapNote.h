#pragma once
#include "LaneDraw.h"
#include <vector>

class Chart;
enum class Judge;

// タップノーツ（画像に種類ごとの色を掛けて描く）
class TapNote
{
public:
	// まだ判定されていないノーツを描く
	void Draw(ImDrawList* dl, const LaneDraw::Layout& layout, const Chart& chart,
		const std::vector<double>& noteTimes, const std::vector<Judge>& judges, float now, float speed) const;
};
