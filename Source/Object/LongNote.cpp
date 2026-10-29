#include "main.h"
#include "Manager.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "LongNote.h"
#include <algorithm>
#include "Input.h"
#include "NoteManager.h"
#include "Renderer.h"
#include "Camera.h"
#include "PlaneRenderer.h"
#include <cmath>

static constexpr int TICK_STEP = TICKS_PER_BEAT / 2; // 中継点の間隔(8分音符)
static constexpr float BODY_WIDTH = 0.7f;			 // レーン幅に対する帯の幅(前の 0.15〜0.85)
static constexpr float CAP_WIDTH = 0.92f;			 // レーン幅に対する始点・終点の幅(TapNote と同じ)
static constexpr float CAP_DEPTH = 28.0f;			 // 始点・終点の厚み(TapNote と同じ)

// ===============================================================
// 初期化・読み込み
// ===============================================================
void LongNote::Init()
{
	m_Layer = 2;

	m_BodyTex = AddComponent<PlaneRenderer>();
	m_BodyTex->Load(L"asset\\texture\\longnote.png");
	m_CapTex = AddComponent<PlaneRenderer>();
	m_CapTex->Load(L"asset\\texture\\tapnote.png");
}

void LongNote::Update()
{
	if (!Conductor::IsPlaying())
		return; // ポーズ中・再開のカウント中は何もしない
	const double now = Conductor::GetSongTime();
	const int layer = m_Manager->GetPlayLayer();

	// 1. 押した瞬間:始点(タップと同じ。今いる段のノーツだけ)
	for (const KeyEvent &e : Input::GetKeyEvents())
	{
		for (int lane = 0; lane < m_Layout.laneCount; lane++)
		{
			if (e.key == Lane::KEYS[lane])
				JudgeStart(lane, layer, Conductor::CounterToSongTime(e.time));
		}
	}

	// 2. レーンごとに「押している」「離した瞬間」を出す
	bool held[LANE_COUNT];
	bool released[LANE_COUNT];
	for (int lane = 0; lane < m_Layout.laneCount; lane++)
	{
		held[lane] = Input::GetKeyPress(Lane::KEYS[lane]);
		released[lane] = m_WasHeld[lane] && !held[lane]; // 前は押していて、今は押していない
		m_WasHeld[lane] = held[lane];					 // 次のフレームのために覚えておく
	}

	// 3. ロングごとに、始点の MISS・中継点・終点を見る
	for (LongData &n : m_Notes)
	{
		if (n.endJudge != Judge::None)
			continue; // もう全部終わった

		// 始点:叩かれないまま BAD の幅を過ぎたら MISS(段に関係なく)
		if (n.startJudge == Judge::None && now - n.startTime > WINDOW_BAD)
			SetJudge(n, Judge::Miss, 0.0);

		JudgeTicks(n, now, held[n.lane]);
		JudgeEnd(n, now, held[n.lane], released[n.lane]);
	}
}

void LongNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;

	// 全部床に寝かせる
	m_Rotation = Vector3(XM_PIDIV2, 0.0f, 0.0f);

	// 1. 帯(加算)。黒いところは加算で消えて、光る帯になる
	Renderer::SetAddEnable(true);
	for (const LongData &n : m_Notes)
	{
		if (n.endJudge != Judge::None)
			continue;
		float zStart = ((float)n.startTime - now) * speed;
		float zEnd = ((float)n.endTime - now) * speed;

		bool holding = (n.startJudge != Judge::None) && Input::GetKeyPress(Lane::KEYS[n.lane]);
		if (holding)
			zStart = 0.0f;

		if (zStart > Z_FAR || zEnd < -60.0f)
			continue;				  // まだ遠い・通り過ぎ
		zEnd = std::min(zEnd, Z_FAR); // 奥ははみ出さない

		float x = (n.lane + 0.5f - m_Layout.laneCount * 0.5f) * m_Layout.laneWidth;
		float y = (n.layer == 1) ? UPPER_HEIGHT : 0.0f;

		m_Scale = Vector3(m_Layout.laneWidth * BODY_WIDTH, zEnd - zStart, 1.0f);
		m_Position = Vector3(x, y, (zStart + zEnd) * 0.5f);
		m_BodyTex->SetAlpha(holding ? 1.0f : 0.6f);
		m_BodyTex->Draw();
	}
	Renderer::SetAddEnable(false);

	// 2. 始点・終点(ふつうの合成)。帯の上にかぶせる
	m_Scale = Vector3(m_Layout.laneWidth * CAP_WIDTH, CAP_DEPTH, 1.0f);
	for (const LongData &n : m_Notes)
	{
		if (n.endJudge != Judge::None)
			continue;

		float zStart = ((float)n.startTime - now) * speed;
		float zEnd = ((float)n.endTime - now) * speed;

		bool holding = (n.startJudge != Judge::None) && Input::GetKeyPress(Lane::KEYS[n.lane]);
		if (holding)
			zStart = 0.0f;

		float x = (n.lane + 0.5f - m_Layout.laneCount * 0.5f) * m_Layout.laneWidth;
		float y = (n.layer == 1) ? UPPER_HEIGHT : 0.0f;

		// 段ごとの色
		m_CapTex->SetColor((n.layer == 1) ? XMFLOAT4(0.78f, 0.59f, 1.0f, 1.0f) : XMFLOAT4(0.43f, 0.88f, 1.0f, 1.0f));

		for (float z : {zStart, zEnd})
		{
			if (z > Z_FAR || z < -60.0f)
				continue;
			m_Position = Vector3(x, y, z);
			m_CapTex->Draw();
		}
	}

	// 元に戻す(Manager の並べ替えに影響しないように)
	m_Position = Vector3(0.0f, 0.0f, 0.0f);
	m_Rotation = Vector3(0.0f, 0.0f, 0.0f);
	m_Scale = Vector3(1.0f, 1.0f, 1.0f);
}

void LongNote::Load(const Chart &chart)
{
	m_Notes.clear();
	for (const Note &n : chart.notes)
	{
		if (n.type != 1)
			continue; // ロングだけ拾う
		if (n.length <= 0)
			continue; // 長さ 0 はロングにならないので飛ばす

		LongData data;
		data.startTime = chart.TickToSec(n.tick);
		data.endTime = chart.TickToSec(n.tick + n.length);
		data.lane = n.lane;
		data.layer = n.layer;
		for (int t = n.tick + TICK_STEP; t < n.tick + n.length; t += TICK_STEP)
			data.ticks.push_back(chart.TickToSec(t));
		m_Notes.push_back(data);
	}
}

void LongNote::Reset(double startSec)
{
	for (LongData &n : m_Notes)
	{
		n.nextTick = 0;
		if (n.startTime < startSec)
		{
			// 途中から再生なら、始まっているロングは丸ごと飛ばす
			n.startJudge = Judge::Skip;
			n.endJudge = Judge::Skip;
			n.nextTick = (int)n.ticks.size();
		}
		else
		{
			n.startJudge = Judge::None;
			n.endJudge = Judge::None;
		}
	}
	for (bool &h : m_WasHeld)
		h = false;
}

int LongNote::GetJudgeCount() const
{
	int count = 0;
	for (const LongData &n : m_Notes)
		count += 1 + (int)n.ticks.size() + 1; // 始点 + 中継点 + 終点
	return count;
}

double LongNote::GetLastTime() const
{
	double last = 0.0;
	for (const LongData &n : m_Notes)
		last = std::max(last, n.endTime);
	return last;
}

void LongNote::JudgeStart(int lane, int layer, double hitTime)
{
	for (LongData &n : m_Notes) // 始点の時刻の順
	{
		if (n.startJudge != Judge::None)
			continue; // もう始点を判定した
		if (n.lane != lane || n.layer != layer)
			continue; // 違うレーン・違う段

		double diff = hitTime - n.startTime;
		if (diff > WINDOW_BAD)
			continue; // もう過ぎてる(このフレームで MISS になる)
		if (diff < -WINDOW_BAD)
			return; // まだ遠い

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

void LongNote::JudgeTicks(LongData &note, double now, bool held)
{
	// 時刻を過ぎた中継点を、その瞬間に押しているかで判定
	while (note.nextTick < (int)note.ticks.size() && now >= note.ticks[note.nextTick])
	{
		m_Manager->OnJudge(held ? Judge::Perfect : Judge::Miss, note.lane, 0.0, 0); // 輪は出さない
		note.nextTick++;
	}
}

void LongNote::JudgeEnd(LongData &note, double now, bool held, bool released)
{
	if (note.startJudge == Judge::None)
		return; // 始点がまだなら終点もまだ

	double diff = now - note.endTime; // マイナス = 終点より前

	// 終点の BAD の幅の中で離した → そのずれで判定
	if (released && diff >= -WINDOW_BAD && diff < 0.0)
	{
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
		SetJudge(note, judge, diff);
		return;
	}

	// 終点の時刻になった → 押していれば PERFECT、離していれば MISS
	if (diff >= 0.0)
		SetJudge(note, held ? Judge::Perfect : Judge::Miss, 0.0);
}

void LongNote::SetJudge(LongData &note, Judge judge, double diff)
{
	if (note.startJudge == Judge::None)
	{
		note.startJudge = judge; // 始点がまだなら始点
	}
	else
	{
		note.endJudge = judge; // 始点が済んでいれば終点
	}
	m_Manager->OnJudge(judge, note.lane, diff);
}