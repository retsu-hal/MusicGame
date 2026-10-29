#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "WheelNote.h"
#include "Input.h"
#include "NoteManager.h"
#include "PlaneRenderer.h"
#include <algorithm>
#include <cmath>

static constexpr float ARROW_SIZE = 120.0f; // 矢印の大きさ
static constexpr float ARROW_GAP = 10.0f;	// バーと矢印のすき間

// ===============================================================
// 初期化・読み込み
// ===============================================================
void WheelNote::Init()
{
	m_Layer = 2; // 床(1)と判定ライン(3)の間

	m_UpBar = AddComponent<PlaneRenderer>();
	m_UpBar->Load(L"asset\\texture\\flick_wheel_up.png");
	m_DownBar = AddComponent<PlaneRenderer>();
	m_DownBar->Load(L"asset\\texture\\flick_wheel_down.png");
	m_UpArrow = AddComponent<PlaneRenderer>();
	m_UpArrow->Load(L"asset\\texture\\wheel_arrow_up.png");
	m_DownArrow = AddComponent<PlaneRenderer>();
	m_DownArrow->Load(L"asset\\texture\\wheel_arrow_down.png");
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

	DrawDir(1, now, speed);	 // 上(マゼンタ)
	DrawDir(-1, now, speed); // 下(ブルー)

	// 元に戻す(Manager の並べ替えに影響しないように)
	m_Position = Vector3(0.0f, 0.0f, 0.0f);
	m_Rotation = Vector3(0.0f, 0.0f, 0.0f);
	m_Scale = Vector3(1.0f, 1.0f, 1.0f);
}

void WheelNote::DrawDir(int dir, float now, float speed)
{
	// 上フリックは下段にいるときに来る → 下段の高さ。下フリックはその逆
	const float height = (dir > 0) ? 0.0f : UPPER_HEIGHT;

	// バーの大きさ。幅 = レーン全体、厚み = 画像の縦横比(1440 × 128)を保つ
	const float barW = m_Layout.laneWidth * m_Layout.laneCount;
	const float barD = barW * (128.0f / 1440.0f);

	// 矢印の真ん中の高さ。上向きはバーの上に立てる、下向きはバーの下にぶら下げる
	const float arrowY = (dir > 0) ? height + ARROW_GAP + ARROW_SIZE * 0.5f : height - ARROW_GAP - ARROW_SIZE * 0.5f;

	// この向きで使う板
	PlaneRenderer *bar = (dir > 0) ? m_UpBar : m_DownBar;
	PlaneRenderer *arrow = (dir > 0) ? m_UpArrow : m_DownArrow;

	for (const WheelData &n : m_Notes)
	{
		if (n.dir != dir)
			continue;
		if (n.judge != Judge::None)
			continue;

		float z = ((float)n.time - now) * speed;
		if (z > Z_FAR || z < -60.0f)
			continue; // 遠すぎ・通り過ぎ

		// 奥から出てくるときにふわっと(TapNote と同じ式)。色は画像に入っているので白
		float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);

		// バー:床に寝かせる
		m_Scale = Vector3(barW, barD, 1.0f);
		m_Rotation = Vector3(XM_PIDIV2, 0.0f, 0.0f);
		m_Position = Vector3(0.0f, height, z);
		bar->SetAlpha(fade);
		bar->Draw();

		// 矢印:立ったまま(カメラに正面を向く)
		m_Scale = Vector3(ARROW_SIZE, ARROW_SIZE, 1.0f);
		m_Rotation = Vector3(0.0f, 0.0f, 0.0f);
		m_Position = Vector3(0.0f, arrowY, z);
		arrow->SetAlpha(fade);
		arrow->Draw();
	}
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