#include "main.h"
#include "NoteManager.h"
#include "Score.h"
#include "Combo.h"
#include "Audio.h"
#include "Conductor.h"
#include "Input.h"
#include "Manager.h"
#include "Settings.h"
#include "GameSession.h"
#include "NoteBase.h"
#include "TapNote.h"
#include "LongNote.h"
#include "ArcNote.h"
#include "Lane.h"       
#include <algorithm>

// 達成率の割合(OnJudge の switch で使う)
static constexpr double RATE_PERFECT = 1.0;
static constexpr double RATE_GREAT = 0.7;
static constexpr double RATE_GOOD = 0.3;

static constexpr double LEAD_IN = 2.0;   // 曲が始まるまでの待ち時間(秒)
static constexpr double END_WAIT = 2.0;   // 最後のノーツから終了までの時間(秒)

void NoteManager::Init()
{
	// GameScene が先に足しておいたものを探す
	m_Score = Manager::GetGameObject<Score>();
	m_Combo = Manager::GetGameObject<Combo>();
}

void NoteManager::Uninit()
{
	Conductor::Stop();      // シーンを抜けるとき、曲の時計を止める
	GameObject::Uninit();   // 親の後片付け(m_Music などのコンポーネント)
}

void NoteManager::Update()
{
	GameObject::Update();

	const double now = Conductor::GetSongTime();
	const float  dt = Manager::GetDeltaTime();

	// 段の切り替え(ノーツの判定より先。NoteManager は TapNote より先に足してあるので、先に Update される)
	// 奥に回す(+)で上段、手前に回す(-)で下段
	int wheel = Input::GetMouseWheel();
	if (wheel > 0) m_PlayLayer = 1;
	if (wheel < 0) m_PlayLayer = 0;

	// 見た目だけ追いかける(判定は m_PlayLayer を見るから遅れない)
	m_PlayLayerAnim += ((float)m_PlayLayer - m_PlayLayerAnim) * std::min(1.0f, dt * 20.0f);
	
    // マウスの横の移動量で、カーソルを動かす(レーンの幅の中で止める)
	m_CursorX += Input::GetMouseMoveX() * Settings::mouseSensitivity;
	m_CursorX = std::clamp(m_CursorX, 0.0f, (float)LANE_COUNT);



	// 終了時刻を過ぎたら終わり
	if (m_Music && !m_Finished && now > m_EndTime)
		Finish();
}

bool NoteManager::LoadChart(const std::string& path)
{
	if (!m_Chart.Load(path)) return false;

	m_Music = AddComponent<Audio>();
	m_Music->Load(m_Chart.audioPath.c_str(), Audio::TYPE_MUSIC);

	// ノーツの種類を足す(種類が増えたら、ここに1行足す)
    m_Notes.push_back(Manager::AddGameObject<ArcNote>());
	m_Notes.push_back(Manager::AddGameObject<TapNote>());
	m_Notes.push_back(Manager::AddGameObject<LongNote>());


	// 各種類に、自分の分を拾ってもらう
	double lastTime = 0.0;
	for (NoteBase* notes : m_Notes)
	{
		notes->SetManager(this);
		notes->Load(m_Chart);
		lastTime = std::max(lastTime, notes->GetLastTime());
	}

	// 終了時刻:最後のノーツ + 少し。ノーツがなければ曲の長さ
	m_EndTime = (lastTime > 0.0) ? lastTime + END_WAIT : m_Music->GetLength();
	return true;
}

void NoteManager::Play()
{
	if (!m_Music) return;

	for (int& c : m_Count) c = 0;
	m_Finished = false;
	m_PlayLayer = 0;      // 下段から始める
	m_PlayLayerAnim = 0.0f;
	m_CursorX = LANE_COUNT * 0.5f;

	// 各種類をやり直して、判定の回数を数える
	m_TotalJudges = 0;
	for (NoteBase* notes : m_Notes)
	{
		notes->Reset(-LEAD_IN);
		m_TotalJudges += notes->GetJudgeCount();
	}

	m_Score->Reset();
	m_Score->SetTotalJudges(m_TotalJudges);
	m_Combo->Reset();

	Conductor::SetOffset(Settings::offsetMs / 1000.0);
	Conductor::Start(m_Music,-LEAD_IN);
}

void NoteManager::OnJudge(Judge judge, int lane, double diff)
{
	m_Count[(int)judge]++;

	// 判定ごとに、達成率とコンボへ知らせる
	switch (judge)
	{
		case Judge::Perfect: m_Score->AddScore(RATE_PERFECT); m_Combo->AddCombo(); break;
		case Judge::Great:   m_Score->AddScore(RATE_GREAT);   m_Combo->AddCombo(); break;
		case Judge::Good:    m_Score->AddScore(RATE_GOOD);    m_Combo->AddCombo(); break;
		case Judge::Bad:
		case Judge::Miss:    m_Combo->ResetCombo(); break;   // 達成率は 0 なので足さない
		default: break;
	}
}

// 曲が終わったら、結果を GameSession に書いて「終わった」印を立てる
void NoteManager::Finish()
{
	PlayResult& r = GameSession::result;
	r.title = m_Chart.title;
	r.achievement = m_Score->GetAchievement();
	r.perfect = m_Count[(int)Judge::Perfect];
	r.great = m_Count[(int)Judge::Great];
	r.good = m_Count[(int)Judge::Good];
	r.bad = m_Count[(int)Judge::Bad];
	r.miss = m_Count[(int)Judge::Miss];
	r.maxCombo = m_Combo->GetMaxCombo();
	r.totalNotes = m_TotalJudges;   // ノーツの数ではなく判定の回数

	m_Finished = true;   // シーン切り替えは GameScene が行う
}


