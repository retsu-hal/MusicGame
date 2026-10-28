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

// キーの文字の画像と幅(高さはどれも 41)
static const WCHAR *KEY_FILE[LANE_COUNT] = {
	L"asset\\texture\\ui\\text\\lane_key\\a.png",
	L"asset\\texture\\ui\\text\\lane_key\\s.png",
	L"asset\\texture\\ui\\text\\lane_key\\d.png",
	L"asset\\texture\\ui\\text\\lane_key\\f.png",
	L"asset\\texture\\ui\\text\\lane_key\\space.png"};
static const float KEY_W[LANE_COUNT] = {26.0f, 26.0f, 27.0f, 26.0f, 81.0f};
static constexpr float KEY_Z = -50.0f; // キー枠の真ん中の奥行き

void JudgeLine::Init()
{
	m_Layer = 3; // ノーツより手前
	m_White = TextureManager::Load(L"asset\\texture\\ui\\assets\\white_square.png");
	m_Cursor = TextureManager::Load(L"asset\\texture\\ui\\assets\\cursor.png");
	for (int i = 0; i < LANE_COUNT; i++)
		m_KeyTex[i] = TextureManager::Load(KEY_FILE[i]);
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

	// 4. キーの文字(2D)。キー枠の真ん中に、押している間だけ白く
	Renderer::SetWorldViewProjection2D();

	const float k = CAMERA_DIST / (CAMERA_DIST + KEY_Z);	// 奥行き -50 の大きさの倍率
	const float eyeY = m_play.judgeLineY - HORIZON_Y;		// カメラの高さ(GameScene と同じ式)
	const float y = HORIZON_Y + (eyeY - m_play.height) * k; // 文字の中心の高さ

	for (int i = 0; i < LANE_COUNT; i++)
	{
		float x = SCREEN_WIDTH * 0.5f + (i + 0.5f - N * 0.5f) * m_play.laneWidth * k;
		float hw = KEY_W[i] * 0.5f;
		float hh = 41.0f * 0.5f;

		XMFLOAT4 c = Input::GetKeyPress(Lane::KEYS[i]) ? XMFLOAT4(0.949f, 0.941f, 0.980f, 1.0f)	 // 押している #F2F0FA
													   : XMFLOAT4(0.541f, 0.525f, 0.659f, 1.0f); // 押していない #8A86A8
		QuadBatch::Quad(XMFLOAT3(x - hw, y - hh, 0.0f), XMFLOAT3(x + hw, y - hh, 0.0f),
						XMFLOAT3(x + hw, y + hh, 0.0f), XMFLOAT3(x - hw, y + hh, 0.0f),
						c, c);
		QuadBatch::Flush(m_KeyTex[i]); // キーごとに画像が違うので、1個ずつ
	}

	Renderer::SetDepthEnable(true);
}