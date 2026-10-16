#pragma once
#include "LaneDraw.h"

class Chart;

// レーン（床・押したときの光・拍の線・区切り線・判定ライン・キーの枠）
class Lane
{
public:
	static constexpr BYTE KEYS[4] = { 'D', 'F', 'J', 'K' };   // 各レーンのキー

	void Update(const LaneDraw::Layout& layout, float dt);    // 押しているレーンを光らせる

	// ノーツより奥に描くもの（床・光・拍の線・区切り線）
	void DrawBack(ImDrawList* dl, const LaneDraw::Layout& layout, const Chart& chart, float now, float speed) const;
	// ノーツより手前に描くもの（判定ライン・キーの枠）
	void DrawFront(ImDrawList* dl, const LaneDraw::Layout& layout) const;

private:
	float m_Glow[4] = {};
};
