#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Input.h"
#include "Camera.h"
#include "NoteManager.h"
#include "QuadBatch.h"
#include "TextureManager.h"
#include "JudgeLine.h"
#include <algorithm>

void JudgeLine::Init()
{
	m_Layer = 3; // ノーツより手前
	m_White = TextureManager::Load(L"asset\\texture\\ui\\assets\\white_square.png");
	m_Cursor = TextureManager::Load(L"asset\\texture\\ui\\assets\\cursor.png");
	m_upper.height = UPPER_HEIGHT; // 上段は動かないので、最初に1回だけ
}

void JudgeLine::Update()
{
	float dt = Manager::GetDeltaTime();

	// 押したキーの枠を濃くする(徐々に戻す)
	for (int i = 0; i < LANE_COUNT; i++)
	{
		if (Input::GetKeyPress(Lane::KEYS[i]))
			m_Glow[i] = 1.0f;
		else
			m_Glow[i] = std::max(0.0f, m_Glow[i] - dt * 6.0f);
	}
}

void JudgeLine::Draw()
{
	NoteManager *noteManager = Manager::GetGameObject<NoteManager>();
	if (!noteManager)
		return;

	m_play.height = UPPER_HEIGHT * noteManager->GetPlayLayerAnim(); // 今いる面(上下する)

	// 3D 描画
	Renderer::ClearDepth();
	Renderer::SetDepthEnable(false);
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw();

	// 1. 判定ライン(上段の固定と、今いる面)。太さ 6 → 半分の 3
	const XMFLOAT4 lineColor(1.0f, 0.78f, 0.96f, 1.0f); // (255, 200, 245)
	const float N = (float)LANE_COUNT;
	for (const Layout &L : {m_upper, m_play})
	{
		QuadBatch::Quad(L.World(0, 3.0f), L.World(N, 3.0f),
						L.World(N, -3.0f), L.World(0, -3.0f),
						lineColor, lineColor);
	}

	// 2. キーの枠(今いる面)。押すと濃くなる
	for (int i = 0; i < LANE_COUNT; i++)
	{
		XMFLOAT4 c(0.59f, 0.47f, 1.0f, 0.10f + 0.35f * m_Glow[i]); // (150, 120, 255)
		QuadBatch::Quad(m_play.World(i + 0.12f, -20.0f), m_play.World(i + 0.88f, -20.0f),
						m_play.World(i + 0.88f, -80.0f), m_play.World(i + 0.12f, -80.0f),
						c, c);
	}
	QuadBatch::Flush(m_White);

	// 3. アークのカーソル(今いる面の判定ラインの上)
	const float x = noteManager->GetCursorX();
	const float s = 0.3f;				  // 大きさの半分(レーン単位)
	const float h = s * m_play.laneWidth; // 奥行きも同じピクセル数で正方形に
	const XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);
	QuadBatch::Quad(m_play.World(x - s, h), m_play.World(x + s, h),
					m_play.World(x + s, -h), m_play.World(x - s, -h),
					white, white);
	QuadBatch::Flush(m_Cursor);

	Renderer::SetDepthEnable(true);
}