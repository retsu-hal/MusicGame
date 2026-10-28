#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Input.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "QuadBatch.h"
#include "TextureManager.h"
#include "NoteManager.h"
#include "TapNote.h"
#include <cmath>
#include <algorithm>

// ===============================================================
// 初期化・読み込み・やり直し
// ===============================================================
void TapNote::Init()
{
	m_Layer = 2; // 床(1)と判定ライン(3)の間
	m_Texture = TextureManager::Load(L"asset\\texture\\tapnote.png");
}

void TapNote::Load(const Chart &chart)
{
	m_Notes.clear();
	for (const Note &n : chart.notes)
	{
		if (n.type != 0)
			continue; // タップだけ拾う

		TapData data;
		data.time = chart.TickToSec(n.tick);
		data.lane = n.lane;
		data.layer = n.layer;
		m_Notes.push_back(data);
	}
}

void TapNote::Reset(double startSec)
{
	for (TapData &n : m_Notes)
		n.judge = (n.time < startSec) ? Judge::Skip : Judge::None; // 途中から再生なら、前のノーツは飛ばす
}

// ===============================================================
// 判定
// ===============================================================
void TapNote::Update()
{
	if (m_Manager->IsFinished())
		return;

	const double now = Conductor::GetSongTime();
	const int layer = m_Manager->GetPlayLayer(); // 今いる段のノーツだけ叩ける

	// 1. キーが押されたレーンを判定(押された瞬間の時刻を使う)
	for (const KeyEvent &e : Input::GetKeyEvents())
	{
		for (int lane = 0; lane < m_Layout.laneCount; lane++)
		{
			if (e.key == Lane::KEYS[lane])
				JudgeLane(lane, layer, Conductor::CounterToSongTime(e.time));
		}
	}

	// 2. 叩かれないまま BAD の幅も過ぎたノーツは MISS(段に関係なく)
	for (TapData &n : m_Notes)
	{
		if (n.judge == Judge::None && now - n.time > WINDOW_BAD)
			SetJudge(n, Judge::Miss, 0.0);
	}
}

void TapNote::JudgeLane(int lane, int layer, double hitTime)
{
	for (TapData &n : m_Notes) // 時刻の順
	{
		if (n.judge != Judge::None)
			continue; // もう判定した
		if (n.lane != lane || n.layer != layer)
			continue; // 違うレーン・違う段

		double diff = hitTime - n.time; // +なら遅い、-なら早い
		if (diff > WINDOW_BAD)
			continue; // もう過ぎてる(このフレームで MISS になる)
		if (diff < -WINDOW_BAD)
			return; // まだ遠い。後ろのノーツはもっと遠いので終わり

		double a = std::fabs(diff);
		Judge judge;
		if (a <= WINDOW_PERFECT)
			judge = Judge::Perfect;
		else if (a <= WINDOW_GREAT)
			judge = Judge::Great;
		else if (a <= WINDOW_GOOD)
			judge = Judge::Good;
		else
			judge = Judge::Bad;

		SetJudge(n, judge, diff);
		return; // 1回押したら1個だけ
	}
}

void TapNote::SetJudge(TapData &note, Judge judge, double diff)
{
	note.judge = judge;
	m_Manager->OnJudge(judge, note.lane, diff);
}

// ===============================================================
// 描画
// ===============================================================
void TapNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;
	const float h = 14.0f; // ノーツの奥行き方向の厚みの半分

	Renderer::ClearDepth();			 // 背景(2D)が書いた深度を消す
	Renderer::SetDepthEnable(false); // 奥から順に描くので、深度は書かない
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw(); // 背景の 2D で上書きされた行列を送り直す

	for (const TapData &n : m_Notes)
	{
		if (n.judge != Judge::None)
			continue; // 判定が済んだものは消える

		float z = ((float)n.time - now) * speed;
		if (z > Z_FAR || z < -60.0f)
			continue; // 遠すぎ・通り過ぎ

		// 段の高さに置く
		Layout L = m_Layout;
		L.height = (n.layer == 1) ? UPPER_HEIGHT : 0.0f;

		// 奥から出てくるときにふわっと出す
		float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);

		// 段ごとの色
		XMFLOAT4 tint = (n.layer == 1) ? XMFLOAT4(0.78f, 0.59f, 1.0f, fade)	 // 上段:紫 (200, 150, 255)
									   : XMFLOAT4(0.43f, 0.88f, 1.0f, fade); // 下段:水色 (110, 225, 255)

		float lane = (float)n.lane;
		QuadBatch::Quad(L.World(lane + 0.04f, z + h), L.World(lane + 0.96f, z + h),
						L.World(lane + 0.96f, z - h), L.World(lane + 0.04f, z - h),
						tint, tint);
	}
	QuadBatch::Flush(m_Texture);

	Renderer::SetDepthEnable(true);
}