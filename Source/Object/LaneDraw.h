#pragma once
#include "main.h"
#include <algorithm>

// ---------------------------------------------------------------
// ノーツレーンまわり（Lane / TapNote / Notelane）で共通して使う
// 見た目の設定と描画の補助関数
// ---------------------------------------------------------------
namespace LaneDraw
{
	// 見た目の設定（奥行き）
	constexpr float CAMERA_DIST = 600.0f;
	constexpr float HORIZON_Y   = 90.0f;
	constexpr float Z_FAR       = 2400.0f;
	constexpr float Z_NEAR      = -110.0f;

	// レーンの並び方。レーン上の位置と奥行き z を、画面の座標に変換する
	struct Layout
	{
		int   laneCount  = 4;
		float laneWidth  = 120.0f;     // 判定ライン上でのレーン幅（ピクセル）
		float judgeLineY = 600.0f;     // 判定ラインの高さ

		ImVec2 Project(float laneX, float z) const
		{
			float s  = CAMERA_DIST / (CAMERA_DIST + z);
			float cx = SCREEN_WIDTH * 0.5f;
			float x  = cx + (laneX - laneCount * 0.5f) * laneWidth * s;
			float y  = HORIZON_Y + (judgeLineY - HORIZON_Y) * s;
			return ImVec2(x, y);
		}
	};

	inline ImU32 Col(int r, int g, int b, float a)
	{
		a = std::clamp(a, 0.0f, 1.0f);
		return IM_COL32(r, g, b, (int)(a * 255));
	}

	inline void QuadGradient(ImDrawList* dl, ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3,
		ImU32 c0, ImU32 c1, ImU32 c2, ImU32 c3)
	{
		ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
		dl->PrimReserve(6, 4);
		ImDrawIdx i = (ImDrawIdx)dl->_VtxCurrentIdx;
		dl->PrimWriteIdx(i);     dl->PrimWriteIdx(i + 1); dl->PrimWriteIdx(i + 2);
		dl->PrimWriteIdx(i);     dl->PrimWriteIdx(i + 2); dl->PrimWriteIdx(i + 3);
		dl->PrimWriteVtx(p0, uv, c0);
		dl->PrimWriteVtx(p1, uv, c1);
		dl->PrimWriteVtx(p2, uv, c2);
		dl->PrimWriteVtx(p3, uv, c3);
	}

	inline void GlowLine(ImDrawList* dl, ImVec2 a, ImVec2 b, int r, int g, int bl, float alpha, float thick)
	{
		dl->AddLine(a, b, Col(r, g, bl, 0.08f * alpha), thick * 8.0f);
		dl->AddLine(a, b, Col(r, g, bl, 0.15f * alpha), thick * 4.0f);
		dl->AddLine(a, b, Col(r, g, bl, 0.50f * alpha), thick * 2.0f);
		dl->AddLine(a, b, Col(255, 255, 255, 0.90f * alpha), thick);
	}

	// 指定した位置を「中心」として文字を描く
	inline void TextCenter(ImDrawList* dl, float size, ImVec2 center, ImU32 col, const char* text)
	{
		ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
		dl->AddText(ImGui::GetFont(), size, ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f), col, text);
	}

	// 指定した位置を「右上」として文字を描く
	inline void TextRight(ImDrawList* dl, float size, ImVec2 rightTop, ImU32 col, const char* text)
	{
		ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
		dl->AddText(ImGui::GetFont(), size, ImVec2(rightTop.x - ts.x, rightTop.y), col, text);
	}
}
