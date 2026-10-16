#include "main.h"
#include "TapNote.h"
#include "Notelane.h"
#include "TextureManager.h"

using namespace LaneDraw;


void TapNote::Draw(ImDrawList* dl, const Layout& layout, const Chart& chart,
	const std::vector<double>& noteTimes, const std::vector<Judge>& judges, float now, float speed) const
{
	// 白黒のノーツ画像に色を掛けて描く（種類ごとに色を変えられる）
	static ID3D11ShaderResourceView* noteTex = TextureManager::Load(L"asset\\texture\\tapnote.png");

	for (size_t i = 0; i < chart.notes.size(); i++)
	{
		if (!judges.empty() && judges[i] != Judge::None) continue;

		float z = ((float)noteTimes[i] - now) * speed;
		if (z > Z_FAR || z < -60.0f) continue;

		float lane = (float)chart.notes[i].lane;
		float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);
		float h    = 14.0f;

		ImVec2 p0 = layout.Project(lane + 0.04f, z + h), p1 = layout.Project(lane + 0.96f, z + h);
		ImVec2 p2 = layout.Project(lane + 0.96f, z - h), p3 = layout.Project(lane + 0.04f, z - h);

		ImU32 tint = (chart.notes[i].type == 1)
			? Col(255, 210, 90, fade)    // ロング：黄色
			: Col(110, 225, 255, fade);  // タップ：水色
		dl->AddImageQuad((ImTextureID)(intptr_t)noteTex, p0, p1, p2, p3,
			ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), tint);
	}
}
