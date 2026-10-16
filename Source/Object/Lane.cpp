#include "main.h"
#include "Lane.h"
#include "Chart.h"
#include "Input.h"
#include <cmath>

using namespace LaneDraw;

static const char* KEY_NAMES[4] = { "D", "F", "J", "K" };


void Lane::Update(const Layout& layout, float dt)
{
	for (int lane = 0; lane < layout.laneCount; lane++)
	{
		if (Input::GetKeyPress(KEYS[lane])) m_Glow[lane] = 1.0f;
		else m_Glow[lane] = std::max(0.0f, m_Glow[lane] - dt * 6.0f);
	}
}


void Lane::DrawBack(ImDrawList* dl, const Layout& layout, const Chart& chart, float now, float speed) const
{
	const int N = layout.laneCount;

	// 1. 床
	QuadGradient(dl,
		layout.Project(0, Z_FAR), layout.Project((float)N, Z_FAR), layout.Project((float)N, Z_NEAR), layout.Project(0, Z_NEAR),
		Col(20, 10, 40, 0.0f), Col(20, 10, 40, 0.0f), Col(20, 15, 45, 0.95f), Col(20, 15, 45, 0.95f));

	// 2. 押しているレーンの光
	for (int i = 0; i < N; i++)
	{
		if (m_Glow[i] <= 0.0f) continue;
		float g = m_Glow[i];
		QuadGradient(dl,
			layout.Project((float)i, Z_FAR * 0.6f), layout.Project((float)i + 1, Z_FAR * 0.6f),
			layout.Project((float)i + 1, 0), layout.Project((float)i, 0),
			Col(120, 90, 255, 0.0f), Col(120, 90, 255, 0.0f),
			Col(150, 120, 255, 0.45f * g), Col(150, 120, 255, 0.45f * g));
	}

	// 3. 拍の線
	{
		int beat = (int)std::floor(chart.SecToTick(now) / TICKS_PER_BEAT);
		for (int k = 0; k < 64; k++, beat++)      // 念のため本数に上限を付ける
		{
			float z = ((float)chart.TickToSec(beat * TICKS_PER_BEAT) - now) * speed;
			if (z > Z_FAR) break;
			if (z < 0.0f) continue;
			bool  bar  = (beat % 4 == 0);
			float fade = 1.0f - z / Z_FAR;
			dl->AddLine(layout.Project(0, z), layout.Project((float)N, z),
				Col(170, 150, 255, (bar ? 0.35f : 0.12f) * fade), bar ? 2.0f : 1.0f);
		}
	}

	// 4・5. 区切り線と両端のネオン（奥ほど薄く）
	const int SEG = 24;
	for (int k = 0; k < SEG; k++)
	{
		float z0 = Z_NEAR + (Z_FAR - Z_NEAR) * k / SEG;
		float z1 = Z_NEAR + (Z_FAR - Z_NEAR) * (k + 1) / SEG;
		float fade = 1.0f - std::max(0.0f, z0) / Z_FAR;
		fade = fade * fade;

		for (int i = 1; i < N; i++)
			dl->AddLine(layout.Project((float)i, z0), layout.Project((float)i, z1), Col(160, 140, 230, 0.25f * fade), 1.0f);

		GlowLine(dl, layout.Project(0, z0), layout.Project(0, z1), 190, 90, 255, fade, 2.0f);
		GlowLine(dl, layout.Project((float)N, z0), layout.Project((float)N, z1), 255, 70, 190, fade, 2.0f);
	}
}


void Lane::DrawFront(ImDrawList* dl, const Layout& layout) const
{
	const int N = layout.laneCount;

	// 7. 判定ライン
	GlowLine(dl, layout.Project(0, 0), layout.Project((float)N, 0), 255, 120, 230, 1.0f, 3.0f);

	// 8. キーの枠
	for (int i = 0; i < N; i++)
	{
		ImVec2 a = layout.Project(i + 0.12f, -20.0f);
		ImVec2 b = layout.Project(i + 0.88f, -80.0f);
		float  g = m_Glow[i];
		dl->AddRectFilled(a, b, Col(150, 120, 255, 0.10f + 0.35f * g), 6.0f);
		dl->AddRect(a, b, Col(200, 190, 255, 0.6f + 0.4f * g), 6.0f, 0, 2.0f);
		TextCenter(dl, 26.0f, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f),
			Col(255, 255, 255, 0.8f + 0.2f * g), KEY_NAMES[i]);
	}
}
