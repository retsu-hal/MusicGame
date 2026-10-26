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

using namespace LaneDraw;

// ===============================================================
// Lane と JudgeLine で共通して使うもの
// ===============================================================

// 3D で描く準備(各 Draw の頭で呼ぶ)
static void Begin3D()
{
	Renderer::ClearDepth();            // 背景(2D)が書いた深度を消す
	Renderer::SetDepthEnable(false);   // 奥から順に描くので、深度は書かない
	if (CAMERA* camera = Manager::GetGameObject<CAMERA>())
		camera->Draw();
}

// 3D で描いたあとの後片付け
static void End3D()
{
	Renderer::SetDepthEnable(true);
}

// 押しているレーンを光らせる(押したら 1、離したらだんだん 0 へ)
static void UpdateGlow(float glow[], int count, float dt)
{
	for (int i = 0; i < count; i++)
	{
		if (Input::GetKeyPress(Lane::KEYS[i])) glow[i] = 1.0f;
		else glow[i] = std::max(0.0f, glow[i] - dt * 6.0f);
	}
}

// 奥行き方向の線(区切り線・両端)。床に寝た細長い四角形。奥で消える
static void DepthLine(const Layout& L, float laneX, float width, const XMFLOAT4& nearColor)
{
	float    w = width / L.laneWidth * 0.5f;   // ピクセルの太さ → レーン単位の半分
	XMFLOAT4 farColor = nearColor;
	farColor.w = 0.0f;
	QuadBatch::Quad(L.World(laneX - w, Z_FAR), L.World(laneX + w, Z_FAR),
					L.World(laneX + w, Z_NEAR), L.World(laneX - w, Z_NEAR),
					farColor, nearColor);
}

// 横方向の線(拍の線・判定ライン)。奥行き方向に width の太さを持つ
static void WidthLine(const Layout& L, float z, float width, const XMFLOAT4& color)
{
	float h = width * 0.5f;
	float N = (float)L.laneCount;
	QuadBatch::Quad(L.World(0, z + h), L.World(N, z + h),
					L.World(N, z - h), L.World(0, z - h),
					color, color);
}


// ===============================================================
// Lane(床)
// ===============================================================
void Lane::Init()
{
	m_Layer = 1;   // ノーツより奥
	m_White = TextureManager::Load(L"asset\\texture\\ui\\assets\\white_square.png");
}

void Lane::Update()
{
	UpdateGlow(m_Glow, m_Layout.laneCount, Manager::GetDeltaTime());
}

void Lane::Draw()
{
	NoteManager* noteManager = Manager::GetGameObject<NoteManager>();
	if (!noteManager) return;

	const Chart& chart = noteManager->GetChart();
	const float  now = (float)Conductor::GetSongTime();
	const float  speed = Settings::noteSpeed;

	const float anim = noteManager->GetPlayLayerAnim();      // 0 = 下段、1 = 上段

	Layout play = m_Layout; play.height = UPPER_HEIGHT * anim;   // 今いる面(上下する)
	Layout upper = m_Layout; upper.height = UPPER_HEIGHT;          // 上段の枠(動かない)

	Begin3D();

	// 上段を先、今いる面を後に描く。後のほうが手前に重なるので、今いる面が隠れない
	DrawFloor(upper, chart, now, speed, 0.0f);   // 薄い床だけ
	DrawFloor(play, chart, now, speed, 1.0f);   // 濃い床・光・区切り線
	QuadBatch::Flush(m_White);

	DrawNeon(upper);
	DrawNeon(play);
	QuadBatch::Flush(m_White, true);

	End3D();
}

void Lane::DrawFloor(const Layout& L, const Chart& chart, float now, float speed, float active)
{

	const float N = (float)L.laneCount;

	// 1. 床。いない段は薄く(0.2)、今いる段は濃く(0.95)
	float floorA = 0.2f + 0.75f * active;
	QuadBatch::Quad(L.World(0, Z_FAR), L.World(N, Z_FAR),
					L.World(N, Z_NEAR), L.World(0, Z_NEAR),
					Rgba(20, 10, 40, 0.0f), Rgba(20, 15, 45, floorA));

	// 2. 押しているレーンの光。今いる段だけ(active を掛ける)
	for (int i = 0; i < L.laneCount; i++)
	{
		float g = m_Glow[i] * active;
		if (g <= 0.0f) continue;
			QuadBatch::Quad(L.World((float)i, Z_FAR * 0.6f), L.World(i + 1.0f, Z_FAR * 0.6f),
							L.World(i + 1.0f, 0.0f), L.World((float)i, 0.0f),
							Rgba(120, 90, 255, 0.0f), Rgba(150, 120, 255, 0.45f * g));
		}

	// 3. 拍の線(計算は前と同じ。描き方だけ変える)
	int beat = (int)std::floor(chart.SecToTick(now) / TICKS_PER_BEAT);
	for (int k = 0; k < 64; k++, beat++)      // 念のため本数に上限を付ける
	{
		float z = ((float)chart.TickToSec(beat * TICKS_PER_BEAT) - now) * speed;
		if (z > Z_FAR) break;
		if (z < 0.0f) continue;
		bool  bar = (beat % 4 == 0);
		float fade = 1.0f - z / Z_FAR;
		WidthLine(L, z, bar ? 4.0f : 2.0f, Rgba(170, 150, 255, (bar ? 0.35f : 0.12f) * fade));
	}

	// 4. 区切り線(今いる面だけ)
	if (active > 0.0f)
	{
		for (int i = 1; i < L.laneCount; i++)
			DepthLine(L, (float)i, 1.5f, Rgba(160, 140, 230, 0.25f));
	}
}

void Lane::DrawFrame(const Layout& lower, const Layout& upper)
{
	// 上段と下段をつなぐ縦の線(両端)。縦に立った細い四角形
	// 画面で見て「上 = 上段、下 = 下段」なので、左上 → 右上 → 右下 → 左下 の順がそのまま守れる
	const float w = 1.0f / lower.laneWidth;   // 2 ピクセルぶん(レーン単位の半分)
	for (float x : { 0.0f, (float)lower.laneCount })
	{
		QuadBatch::Quad(upper.World(x - w, 0.0f), upper.World(x + w, 0.0f),
						lower.World(x + w, 0.0f), lower.World(x - w, 0.0f),
						Rgba(200, 180, 255, 0.5f), Rgba(200, 180, 255, 0.5f));
	}
}

void Lane::DrawNeon(const Layout& L)
{
	const float N = (float)L.laneCount;

	// 太い線から順に重ねて、にじんだ光にする(加算なので重なるほど明るい)
	for (float w : { 16.0f, 8.0f, 4.0f })
	{
		DepthLine(L, 0.0f, w, Rgba(190, 90, 255, 0.15f));
		DepthLine(L, N, w, Rgba(255, 70, 190, 0.15f));
	}
	// 芯の白い線
	DepthLine(L, 0.0f, 2.0f, Rgba(255, 255, 255, 0.8f));
	DepthLine(L, N, 2.0f, Rgba(255, 255, 255, 0.8f));
}


// ===============================================================
// JudgeLine(判定ライン・キー枠)
// ===============================================================
void JudgeLine::Init()
{
	m_Layer = 3;   // ノーツより手前
	m_White = TextureManager::Load(L"asset\\texture\\ui\\assets\\white_square.png");
}

void JudgeLine::Update()
{
	UpdateGlow(m_Glow, m_Layout.laneCount, Manager::GetDeltaTime());
}

void JudgeLine::Draw()
{
	Layout upper = m_Layout; upper.height = UPPER_HEIGHT;   // 上段の判定ライン(動かない)
	Layout play = m_Layout;                                // 今いる面(上下する)
	if (NoteManager* noteManager = Manager::GetGameObject<NoteManager>())
		play.height = UPPER_HEIGHT * noteManager->GetPlayLayerAnim();

	Begin3D();

	// 1. 判定ライン(上段の固定と、今いる面)
	WidthLine(upper, 0.0f, 6.0f, Rgba(255, 200, 245, 1.0f));
	WidthLine(play, 0.0f, 6.0f, Rgba(255, 200, 245, 1.0f));

	// 2. キーの枠(今いる面)
	for (int i = 0; i < play.laneCount; i++)
	{
		XMFLOAT4 c = Rgba(150, 120, 255, 0.10f + 0.35f * m_Glow[i]);
		QuadBatch::Quad(play.World(i + 0.12f, -20.0f), play.World(i + 0.88f, -20.0f),
						play.World(i + 0.88f, -80.0f), play.World(i + 0.12f, -80.0f),
						c, c);
	}
	QuadBatch::Flush(m_White);

	End3D();
}