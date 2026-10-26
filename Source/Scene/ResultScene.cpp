#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "GameSession.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace
{
	ImU32 Col(int r, int g, int b, float a)
	{
		a = std::clamp(a, 0.0f, 1.0f);
		return IM_COL32(r, g, b, (int)(a * 255));
	}

	void TextAt(ImDrawList* dl, float size, ImVec2 pos, ImU32 col, const char* text, float alignX = 0.0f)
	{
		// alignX：0 = 左揃え、0.5 = 中央、1 = 右揃え
		ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
		dl->AddText(ImGui::GetFont(), size, ImVec2(pos.x - ts.x * alignX, pos.y), col, text);
	}

	// ランク(ノーツ仕様で決定済みの境目)
	const char* RankOf(double a, ImU32& color)
	{
		if (a >= 100.5) { color = IM_COL32(255, 240, 150, 255); return "SSS+"; }
		if (a >= 100.0) { color = IM_COL32(255, 240, 150, 255); return "SSS"; }
		if (a >= 99.0) { color = IM_COL32(255, 220, 110, 255); return "SS"; }
		if (a >= 97.0) { color = IM_COL32(255, 220, 110, 255); return "S"; }
		if (a >= 94.0) { color = IM_COL32(255, 140, 220, 255); return "AA"; }
		if (a >= 90.0) { color = IM_COL32(255, 140, 220, 255); return "A"; }
		if (a >= 80.0) { color = IM_COL32(150, 200, 255, 255); return "B"; }
		color = IM_COL32(190, 190, 200, 255); return "C";
	}
}


void ResultScene::Init()
{
	m_Time = 0.0f;
}

void ResultScene::Uninit()
{
}

void ResultScene::Update()
{
	m_Time += Manager::GetDeltaTime();

	// 少し待ってから受け付ける（ゲームの最後の打鍵で飛ばしてしまわないように）
	if (m_Time > 0.5f && (Input::GetKeyTrigger(VK_RETURN) || Input::GetKeyTrigger(VK_ESCAPE)))
		Manager::ChangeScene<TitleScene>();

	// ---------- 描画（ImGui の描画リストを使う） ----------
	const PlayResult& r = GameSession::result;
	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	const float W = SCREEN_WIDTH, H = SCREEN_HEIGHT;
	const float fade = std::min(1.0f, m_Time * 3.0f);   // ふわっと表示
	char buf[64];

	// 背景
	dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(W, H),
		Col(30, 14, 60, 1), Col(60, 14, 70, 1), Col(12, 8, 24, 1), Col(10, 10, 30, 1));

	// 見出しと曲名
	TextAt(dl, 22.0f, ImVec2(80, 50), Col(220, 200, 255, fade), "RESULT");
	TextAt(dl, 40.0f, ImVec2(80, 80), Col(255, 255, 255, fade), r.title.empty() ? "(no title)" : r.title.c_str());
	dl->AddLine(ImVec2(80, 136), ImVec2(W - 80, 136), Col(255, 120, 230, 0.6f * fade), 2.0f);

	// ランク（左）
	ImU32 rankColor;
	const char* rank = RankOf(r.achievement, rankColor);
	ImU32 rc = (rankColor & 0x00FFFFFF) | ((ImU32)(255 * fade) << 24);
	TextAt(dl, 200.0f, ImVec2(300, 190), rc, rank, 0.5f);

	// クリア状況
	const char* badge = nullptr;
	if (r.totalNotes > 0 && r.perfect == r.totalNotes) badge = "ALL PERFECT";
	else if (r.totalNotes > 0 && r.miss == 0 && r.bad == 0) badge = "FULL COMBO";
	if (badge) TextAt(dl, 34.0f, ImVec2(300, 430), Col(255, 230, 140, fade), badge, 0.5f);

	// スコア（右上）
	const float rx = 620.0f;
	TextAt(dl, 22.0f, ImVec2(rx, 180), Col(200, 190, 255, fade), "ACHIEVEMENT");
	snprintf(buf, sizeof(buf), "%.2f%%", r.achievement);
	TextAt(dl, 80.0f, ImVec2(rx, 205), Col(255, 255, 255, fade), buf);

	// 判定の内訳（右下）
	struct Row { const char* name; int value; ImU32 color; };
	Row rows[] =
	{
		{ "PERFECT",   r.perfect,  Col(255, 230, 120, fade) },
		{ "GREAT",     r.great,    Col(120, 255, 170, fade) },
		{ "GOOD",      r.good,     Col(120, 190, 255, fade) },
		{ "BAD",       r.bad,      Col(120, 160, 255, fade) },   
		{ "MISS",      r.miss,     Col(180, 180, 190, fade) },
		{ "MAX COMBO", r.maxCombo, Col(255, 255, 255, fade) },
	};
	float y = 320.0f;
	for (const Row& row : rows)
	{
		TextAt(dl, 28.0f, ImVec2(rx, y), row.color, row.name);
		snprintf(buf, sizeof(buf), "%d", row.value);
		TextAt(dl, 28.0f, ImVec2(rx + 420, y), Col(255, 255, 255, fade), buf, 1.0f);
		dl->AddLine(ImVec2(rx, y + 38), ImVec2(rx + 420, y + 38), Col(255, 255, 255, 0.08f * fade));
		y += 48.0f;
	}

	// 操作の案内（点滅）
	float blink = 0.5f + 0.5f * std::sin(m_Time * 4.0f);
	TextAt(dl, 24.0f, ImVec2(W * 0.5f, H - 70), Col(255, 255, 255, (0.4f + 0.6f * blink) * fade),
		"Enter でタイトルへ", 0.5f);
}

void ResultScene::Draw()
{
}
