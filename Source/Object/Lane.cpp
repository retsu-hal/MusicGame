#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Input.h"
#include "Lane.h"
#include "Chart.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "NoteManager.h"
#include "QuadBatch.h"
#include "TextureManager.h"
#include <cmath>
#include <algorithm>

// 奥行き方向の線(区切り線・両端)。床に寝た細長い四角形。奥で消える
static void DepthLine(const Layout &L, float laneX, float width, const XMFLOAT4 &nearColor)
{
	float w = width / L.laneWidth * 0.5f; // ピクセルの太さ → レーン単位の半分
	XMFLOAT4 farColor = nearColor;
	farColor.w = 0.0f; // 奥は同じ色で透明
	QuadBatch::Quad(L.World(laneX - w, Z_FAR), L.World(laneX + w, Z_FAR),
					L.World(laneX + w, Z_NEAR), L.World(laneX - w, Z_NEAR),
					farColor, nearColor);
}

// ===============================================================
// Lane(床)
// ===============================================================
void Lane::Init()
{
	m_Layer = 1; // ノーツより奥
	m_White = TextureManager::Load(L"asset\\texture\\ui\\assets\\white_square.png");
	m_LaneTex = TextureManager::Load(L"asset\\texture\\lane.png");
	m_upper.height = UPPER_HEIGHT;
}

void Lane::Update()
{
	float dt = Manager::GetDeltaTime();

	// 押したキーのレーンを光らせる（徐々に暗くする）
	for (int i = 0; i < LANE_COUNT; i++)
	{
		if (Input::GetKeyPress(Lane::KEYS[i]))
			m_Glow[i] = 1.0f;
		else
			m_Glow[i] = std::max(0.0f, m_Glow[i] - dt * 6.0f);
	}
}

void Lane::Draw()
{
	// 拍の線を引くのに譜面がいる。NoteManager がいなければ何も描かない
	NoteManager *noteManager = Manager::GetGameObject<NoteManager>();
	if (!noteManager)
		return;

	const Chart &chart = noteManager->GetChart();
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;
	const float anim = noteManager->GetPlayLayerAnim(); // 0 = 下段、1 = 上段

	m_play.height = UPPER_HEIGHT * anim; // 今いる面(上下する)

	// 3d描画
	Renderer::ClearDepth();			 // 背景(2D)が書いた深度を消す
	Renderer::SetDepthEnable(false); // 奥から順に描くので、深度は書かない
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw(); // 背景の 2D で上書きされた行列を送り直す

	// 上段を先、今いる面を後に描く。後のほうが手前に重なるので、今いる面が隠れない
	// (DrawFloor の中で、床 → 線 の順に Flush まで済ませる)
	DrawFloor(m_upper, chart, now, speed, 0.0f);
	DrawFloor(m_play, chart, now, speed, 1.0f);

	// 両端のネオン(加算)
	DrawNeon(m_upper);
	DrawNeon(m_play);
	QuadBatch::Flush(m_White, true);

	Renderer::SetDepthEnable(true);
}

void Lane::DrawFloor(const Layout &L, const Chart &chart, float now, float speed, float active)
{
	const float N = (float)L.laneCount;

	// 1. 床(lane.png)。頂点の色は白にして、画像の色をそのまま出す。奥は透明
	//    今いる面は濃く(0.95)、上段の枠は薄く(0.2)
	float floorA = 0.2f + 0.75f * active;
	QuadBatch::Quad(L.World(0, Z_FAR), L.World(N, Z_FAR),
					L.World(N, Z_NEAR), L.World(0, Z_NEAR),
					XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, floorA));
	QuadBatch::Flush(m_LaneTex); // 床だけ画像が違うので、ここで区切る

	// 2. 押しているレーンの光(今いる面だけ)
	for (int i = 0; i < L.laneCount; i++)
	{
		float g = m_Glow[i] * active;
		if (g <= 0.0f)
			continue;
		QuadBatch::Quad(L.World((float)i, Z_FAR * 0.6f), L.World(i + 1.0f, Z_FAR * 0.6f),
						L.World(i + 1.0f, 0.0f), L.World((float)i, 0.0f),
						XMFLOAT4(0.47f, 0.35f, 1.0f, 0.0f),		  // (120, 90, 255) 奥は透明
						XMFLOAT4(0.59f, 0.47f, 1.0f, 0.45f * g)); // (150, 120, 255)
	}

	// 3. 拍の線
	int beat = (int)std::floor(chart.SecToTick(now) / TICKS_PER_BEAT);
	for (int k = 0; k < 64; k++, beat++) // 念のため本数に上限を付ける
	{
		float z = ((float)chart.TickToSec(beat * TICKS_PER_BEAT) - now) * speed;
		if (z > Z_FAR)
			break;
		if (z < 0.0f)
			continue;
		bool bar = (beat % 4 == 0);
		float fade = 1.0f - z / Z_FAR;

		// 拍の線:横いっぱいの細い四角形(奥行き方向に h の太さ)
		float h = bar ? 2.0f : 1.0f;									  // 太さの半分(小節の頭は太く)
		XMFLOAT4 color(0.67f, 0.59f, 1.0f, (bar ? 0.35f : 0.12f) * fade); // (170, 150, 255)
		QuadBatch::Quad(L.World(0, z + h), L.World(N, z + h),
						L.World(N, z - h), L.World(0, z - h),
						color, color);
	}

	// 4. 区切り線(今いる面だけ)
	if (active > 0.0f)
	{
		const XMFLOAT4 divider(0.63f, 0.55f, 0.90f, 0.25f); // (160, 140, 230)
		for (int i = 1; i < L.laneCount; i++)
			DepthLine(L, (float)i, 1.5f, divider);
	}

	QuadBatch::Flush(m_White);
}

void Lane::DrawNeon(const Layout &L)
{
	const float N = (float)L.laneCount;

	const XMFLOAT4 left(0.75f, 0.35f, 1.0f, 0.15f);	 // 左:紫 (190, 90, 255)
	const XMFLOAT4 right(1.0f, 0.27f, 0.75f, 0.15f); // 右:ピンク (255, 70, 190)
	const XMFLOAT4 core(1.0f, 1.0f, 1.0f, 0.8f);	 // 芯:白

	// 太い線から順に重ねて、にじんだ光にする(加算なので重なるほど明るい)
	for (float w : {16.0f, 8.0f, 4.0f})
	{
		DepthLine(L, 0.0f, w, left);
		DepthLine(L, N, w, right);
	}
	// 芯の白い線
	DepthLine(L, 0.0f, 2.0f, core);
	DepthLine(L, N, 2.0f, core);
}
