#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "QuadBatch.h"
#include "TextureManager.h"
#include "WheelNote.h"
#include "Input.h"
#include "NoteManager.h"
#include <algorithm>
#include <cmath>

// ===============================================================
// 初期化・読み込み
// ===============================================================
void WheelNote::Init()
{
	m_Layer = 2; // 床(1)と判定ライン(3)の間
	m_UpTex = TextureManager::Load(L"asset\\texture\\flick_wheel_up.png");
	m_DownTex = TextureManager::Load(L"asset\\texture\\flick_wheel_down.png");
}

void WheelNote::Update()
{
	if (!Conductor::IsPlaying())
		return; // ポーズ中・再開のカウント中は何もしない
	const double now = Conductor::GetSongTime();
	const int wheel = Input::GetMouseWheel();

	// 1. 回したら、その向きのノーツを1個だけ取る
	if (wheel != 0)
	{
		const int dir = (wheel > 0) ? 1 : -1; // 回した向き

		for (WheelData &n : m_Notes) // 時刻の順
		{
			if (n.judge != Judge::None)
				continue; // もう判定した

			double diff = now - n.time;
			if (diff > WINDOW_BAD)
				continue; // もう過ぎてる(このあと MISS になる)
			if (diff < -WINDOW_BAD)
				break; // ここから先は全部まだ遠い

			if (n.dir != dir)
				continue; // 向きが違う → 次のノーツを見る

			// タイミングはタップと同じ(ずれで PERFECT〜BAD。10/28 変更)
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
			break; // 1フレームで1個だけ
		}
	}

	// 2. 回されないまま BAD の幅も過ぎたノーツは MISS
	for (WheelData &n : m_Notes)
	{
		if (n.judge == Judge::None && now - n.time > WINDOW_BAD)
			SetJudge(n, Judge::Miss, 0.0);
	}
}

void WheelNote::SetJudge(WheelData &note, Judge judge, double diff)
{
	note.judge = judge;
	m_Manager->OnJudge(judge, 0, diff, LANE_COUNT);
}

// ===============================================================
// 描画
// ===============================================================
void WheelNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;

	// 3D 描画
	Renderer::ClearDepth();
	Renderer::SetDepthEnable(false);
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw();

	DrawDir(1, now, speed);	 // 上(マゼンタ)
	DrawDir(-1, now, speed); // 下(ブルー)

	Renderer::SetDepthEnable(true);
}

void WheelNote::DrawDir(int dir, float now, float speed)
{
	// 上フリックは下段にいるときに来る → 下段の高さ。下フリックはその逆
	Layout L = m_Layout;
	L.height = (dir > 0) ? 0.0f : UPPER_HEIGHT;

	// 画像の縦横比(1440 × 128)を保つ厚み。幅 = レーン全体のピクセル数
	const float N = (float)L.laneCount;
	const float h = L.laneWidth * N * (128.0f / 1440.0f) * 0.5f;

	for (const WheelData &n : m_Notes)
	{
		if (n.dir != dir)
			continue;
		if (n.judge != Judge::None)
			continue;

		float z = ((float)n.time - now) * speed;
		if (z > Z_FAR || z < -60.0f)
			continue; // 遠すぎ・通り過ぎ

		// 奥から出てくるときにふわっと(TapNote と同じ式)
		float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);
		XMFLOAT4 tint(1.0f, 1.0f, 1.0f, fade); // 色は画像に入っているので白

		QuadBatch::Quad(L.World(0, z + h), L.World(N, z + h),
						L.World(N, z - h), L.World(0, z - h),
						tint, tint);
	}
	QuadBatch::Flush((dir > 0) ? m_UpTex : m_DownTex);
}

void WheelNote::Load(const Chart &chart)
{
	m_Notes.clear();
	for (const Note &n : chart.notes)
	{
		if (n.type != 2)
			continue; // ホイールフリックだけ拾う
		if (n.dir == 0)
			continue; // 向きがないものは回しようがないので飛ばす

		WheelData data;
		data.time = chart.TickToSec(n.tick);
		data.dir = (n.dir > 0) ? 1 : -1; // 2 や -3 と書かれていても 1 / -1 にそろえる
		m_Notes.push_back(data);
	}
}

void WheelNote::Reset(double startSec)
{
	for (WheelData &n : m_Notes)
		n.judge = (n.time < startSec) ? Judge::Skip : Judge::None; // 途中から再生なら、前のノーツは飛ばす
}