#include "main.h"
#include "Notelane.h"
#include "Audio.h"
#include "Conductor.h"
#include "Input.h"
#include "Manager.h"
#include "Settings.h"
#include "GameSession.h"
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------
// 判定とスコアの設定
// ---------------------------------------------------------------
static constexpr double WINDOW_PERFECT = 0.033;   // ±33ms
static constexpr double WINDOW_GREAT   = 0.066;   // ±66ms
static constexpr double WINDOW_GOOD    = 0.100;   // ±100ms

static constexpr int    MAX_SCORE      = 1000000; // 全部 PERFECT で 100万点
static constexpr double RATE_PERFECT   = 1.0;     // 1ノーツの点数に掛ける割合
static constexpr double RATE_GREAT     = 0.7;
static constexpr double RATE_GOOD      = 0.3;

static constexpr double LEAD_IN        = 2.0;     // 曲が始まるまでの待ち時間（秒）
static constexpr double END_WAIT       = 2.0;     // 最後のノーツから終了までの時間（秒）

static constexpr float EFFECT_TIME = 0.35f;     // ヒット演出の長さ（秒）

using namespace LaneDraw;


// ===============================================================
// 読み込み・再生
// ===============================================================
bool Notelane::LoadChart(const std::string& path)
{
	if (!m_Chart.Load(path)) return false;

	m_NoteTimes.clear();
	for (const Note& n : m_Chart.notes)
		m_NoteTimes.push_back(m_Chart.TickToSec(n.tick));

	m_Music = AddComponent<Audio>();
	m_Music->Load(m_Chart.audioPath.c_str());

	// 終了時刻：最後のノーツ + 少し。ノーツが無ければ曲の長さ
	if (!m_NoteTimes.empty()) m_EndTime = m_NoteTimes.back() + END_WAIT;
	else                      m_EndTime = m_Music->GetLength();
	return true;
}


void Notelane::PlayFromStart()
{
	Play(-LEAD_IN);   // マイナスから始めると、Conductor が 0 秒になるまで曲を待ってくれる
}


void Notelane::Play(double startSec)
{
	if (!m_Music) return;

	m_Judges.assign(m_Chart.notes.size(), Judge::None);
	for (int& c : m_Count) c = 0;
	m_DiffSum  = 0.0;
	m_HitCount = 0;
	m_ScoreRaw = 0.0;
	m_Combo    = 0;
	m_MaxCombo = 0;
	m_Finished = false;
	m_Effects.clear();

	for (size_t i = 0; i < m_NoteTimes.size(); i++)
		if (m_NoteTimes[i] < startSec) m_Judges[i] = Judge::Skip;

	Conductor::SetOffset(Settings::offsetMs / 1000.0);
	Conductor::Start(m_Music, startSec);
}


void Notelane::Uninit()
{
	Conductor::Stop();
	GameObject::Uninit();
}


int Notelane::GetScore() const
{
	return (int)std::lround(m_ScoreRaw);
}


// ===============================================================
// 判定
// ===============================================================
void Notelane::JudgeLane(int lane, double now)
{
	for (size_t i = 0; i < m_Chart.notes.size(); i++)
	{
		if (m_Judges[i] != Judge::None) continue;
		if (m_Chart.notes[i].lane != lane) continue;

		double diff = now - m_NoteTimes[i];
		if (diff < -WINDOW_GOOD) return;

		double a = std::fabs(diff);
		if      (a <= WINDOW_PERFECT) SetJudge(i, Judge::Perfect, diff);
		else if (a <= WINDOW_GREAT)   SetJudge(i, Judge::Great,   diff);
		else                          SetJudge(i, Judge::Good,    diff);
		return;
	}
}


void Notelane::SetJudge(size_t index, Judge judge, double diff)
{
	m_Judges[index] = judge;
	m_Count[(int)judge]++;

	// --- スコア：1ノーツあたりの点数 × 判定の割合 ---
	double perNote = (double)MAX_SCORE / m_Chart.notes.size();
	switch (judge)
	{
	case Judge::Perfect: m_ScoreRaw += perNote * RATE_PERFECT; break;
	case Judge::Great:   m_ScoreRaw += perNote * RATE_GREAT;   break;
	case Judge::Good:    m_ScoreRaw += perNote * RATE_GOOD;    break;
	default: break;
	}

	// --- コンボ：MISS で途切れる ---
	if (judge == Judge::Miss)
	{
		m_Combo = 0;
	}
	else
	{
		m_Combo++;
		m_MaxCombo = std::max(m_MaxCombo, m_Combo);
		m_ComboPop = 1.0f;

		m_DiffSum += diff;
		m_HitCount++;
	}

	HitEffect fx;
	fx.lane  = m_Chart.notes[index].lane;
	fx.judge = judge;
	m_Effects.push_back(fx);
}


// 曲が終わったら、結果を GameSession に書いて「終わった」印を立てる
void Notelane::Finish()
{
	PlayResult& r = GameSession::result;
	r.title      = m_Chart.title;
	r.score      = GetScore();
	r.perfect    = m_Count[(int)Judge::Perfect];
	r.great      = m_Count[(int)Judge::Great];
	r.good       = m_Count[(int)Judge::Good];
	r.miss       = m_Count[(int)Judge::Miss];
	r.maxCombo   = m_MaxCombo;
	r.totalNotes = (int)m_Chart.notes.size();

	m_Finished = true;   // シーン切り替えは GameScene が行う
}


// ===============================================================
// 更新
// ===============================================================
void Notelane::Update()
{
	GameObject::Update();

	double now = Conductor::GetSongTime();
	float  dt  = Manager::GetDeltaTime();

	if (!m_Judges.empty() && !m_Finished)
	{
		// 1. キーが押されたレーンを判定（押された瞬間の時刻を使う）
		for (const KeyEvent& e : Input::GetKeyEvents())
		{
			for (int lane = 0; lane < m_Layout.laneCount; lane++)
			{
				if (e.key == Lane::KEYS[lane])
				{
					double hitTime = Conductor::CounterToSongTime(e.time);
					m_LastInputLag = now - hitTime;
					JudgeLane(lane, hitTime);
				}
			}
		}

		// 2. 叩かれないまま判定ラインを過ぎたノーツは Miss
		for (size_t i = 0; i < m_Judges.size(); i++)
		{
			if (m_Judges[i] == Judge::None && now - m_NoteTimes[i] > WINDOW_GOOD)
				SetJudge(i, Judge::Miss, 0.0);
		}
	}

	// 3. 終了時刻を過ぎたら終わり
	if (m_Music && !m_Finished && now > m_EndTime)
		Finish();

	// --- 演出の更新 ---
	m_Lane.Update(m_Layout, dt);
	m_ComboPop = std::max(0.0f, m_ComboPop - dt * 8.0f);

	for (HitEffect& fx : m_Effects) fx.time += dt;
	m_Effects.erase(
		std::remove_if(m_Effects.begin(), m_Effects.end(),
			[](const HitEffect& fx) { return fx.time > EFFECT_TIME; }),
		m_Effects.end());

	// --- 設定・デバッグ用ウィンドウ ---
	ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);   // 最初は畳んでおく
	ImGui::Begin("設定 / デバッグ");

	ImGui::SliderFloat("ハイスピード", &Settings::noteSpeed, 200.0f, 2000.0f, "%.0f");
	if (ImGui::IsItemDeactivatedAfterEdit()) Settings::Save();      // 手を離したときに保存

	if (ImGui::SliderFloat("オフセット(ms)", &Settings::offsetMs, -200.0f, 200.0f, "%.0f"))
		Conductor::SetOffset(Settings::offsetMs / 1000.0);
	if (ImGui::IsItemDeactivatedAfterEdit()) Settings::Save();

	if (ImGui::Button("最初から")) PlayFromStart();
	ImGui::SameLine();
	if (ImGui::Button("10秒から")) Play(10.0);

	ImGui::Separator();
	ImGui::Text("song : %.3f / end %.1f", now, m_EndTime);
	if (m_HitCount > 0)
		ImGui::Text("平均ずれ : %+.1f ms（＋は遅い）", m_DiffSum / m_HitCount * 1000.0);
	ImGui::Text("入力→判定の遅れ : %.1f ms", m_LastInputLag * 1000.0);
	ImGui::End();
}


// ===============================================================
// 描画
// ===============================================================
void Notelane::Draw()
{
	ImDrawList* dl    = ImGui::GetBackgroundDrawList();
	const float now   = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;

	m_Lane.DrawBack(dl, m_Layout, m_Chart, now, speed);                     // 床・拍の線・区切り線
	m_TapNote.Draw(dl, m_Layout, m_Chart, m_NoteTimes, m_Judges, now, speed); // タップノーツ
	m_Lane.DrawFront(dl, m_Layout);                                          // 判定ライン・キーの枠
	DrawEffects();
	DrawHUD();
}


// 叩いたときの演出（光の柱・判定の文字）
void Notelane::DrawEffects()
{
	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// 9. ヒット演出
	for (const HitEffect& fx : m_Effects)
	{
		float t = fx.time / EFFECT_TIME;
		float a = 1.0f - t;
		float L = (float)fx.lane;

		if (fx.judge != Judge::Miss)
		{
			float w = 0.5f * (1.0f - 0.6f * t);
			QuadGradient(dl,
				m_Layout.Project(L + 0.5f - w, Z_FAR * 0.5f), m_Layout.Project(L + 0.5f + w, Z_FAR * 0.5f),
				m_Layout.Project(L + 0.5f + w, 0), m_Layout.Project(L + 0.5f - w, 0),
				Col(120, 240, 255, 0.0f), Col(120, 240, 255, 0.0f),
				Col(200, 250, 255, 0.8f * a), Col(200, 250, 255, 0.8f * a));

			ImVec2 c = m_Layout.Project(L + 0.5f, 0);
			dl->AddCircleFilled(c, 20.0f + 50.0f * t, Col(160, 240, 255, 0.35f * a));
			dl->AddCircleFilled(c, 8.0f + 12.0f * t, Col(255, 255, 255, 0.9f * a));
		}

		const char* text = "";
		ImU32 color = 0;
		switch (fx.judge)
		{
		case Judge::Perfect: text = "PERFECT"; color = Col(255, 230, 120, a); break;
		case Judge::Great:   text = "GREAT";   color = Col(120, 255, 170, a); break;
		case Judge::Good:    text = "GOOD";    color = Col(120, 190, 255, a); break;
		case Judge::Miss:    text = "MISS";    color = Col(180, 180, 190, a); break;
		default: break;
		}
		TextCenter(dl, 22.0f, m_Layout.Project(L + 0.5f, 90.0f + 60.0f * t), color, text);
	}
}


// 画面の文字情報（スコア・コンボ・曲名・進行度）
void Notelane::DrawHUD()
{
	ImDrawList* dl  = ImGui::GetForegroundDrawList();   // デバッグウィンドウより手前に描く
	const double now = Conductor::GetSongTime();
	char buf[64];

	// --- 曲名（左上） ---
	dl->AddText(ImGui::GetFont(), 28.0f, ImVec2(30, 24), Col(255, 255, 255, 0.95f),
		m_Chart.title.empty() ? "(no title)" : m_Chart.title.c_str());

	// --- スコア（右上）：数字は Score（数字画像）が描くので、ここは見出しだけ ---
	TextRight(dl, 18.0f, ImVec2(SCREEN_WIDTH - 30.0f, 20), Col(200, 190, 255, 0.9f), "SCORE");

	// --- 進行度（画面上端のバー） ---
	if (m_EndTime > 0.0)
	{
		float rate = (float)std::clamp(now / m_EndTime, 0.0, 1.0);
		dl->AddRectFilled(ImVec2(0, 0), ImVec2(SCREEN_WIDTH, 4), Col(255, 255, 255, 0.15f));
		dl->AddRectFilled(ImVec2(0, 0), ImVec2(SCREEN_WIDTH * rate, 4), Col(255, 120, 230, 0.9f));
	}

	// --- コンボ（レーンの中央、奥のほう） ---
	if (m_Combo >= 2)
	{
		ImVec2 c    = m_Layout.Project(m_Layout.laneCount * 0.5f, 700.0f);
		float  size = 72.0f * (1.0f + 0.25f * m_ComboPop);   // 増えた瞬間だけ大きく
		snprintf(buf, sizeof(buf), "%d", m_Combo);
		TextCenter(dl, size, c, Col(255, 255, 255, 0.85f), buf);
		TextCenter(dl, 20.0f, ImVec2(c.x, c.y + 46.0f), Col(220, 200, 255, 0.8f), "COMBO");
	}

	// --- カウントイン ---
	if (now < 0.0)
		TextCenter(dl, 56.0f, m_Layout.Project(m_Layout.laneCount * 0.5f, 700.0f), Col(255, 255, 255, 0.9f), "READY");
}
