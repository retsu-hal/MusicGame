#include "main.h"
#include "Manager.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "QuadBatch.h"
#include "TextureManager.h"
#include "ArcNote.h"
#include "Judge.h"
#include "NoteManager.h"
#include <algorithm>
#include <cmath>

using namespace LaneDraw;

static constexpr int   TICK_STEP = TICKS_PER_BEAT / 2;   // 中継点の間隔(8分音符)
static constexpr float ON_WIDTH = 0.5f;                 // 線の上とみなす幅(中心から。レーン単位)★


// t(0〜1、区間のどこまで進んだか)→ 横にどこまで動いたか(0〜1)
static float Ease(float t, int ease)
{
	switch (ease)
	{
		case 1:  return 1.0f - std::cos(t * XM_PIDIV2);   // だんだん速く(最初ゆっくり)
		case 2:  return std::sin(t * XM_PIDIV2);          // だんだん遅く(最初に一気に)
		case 3:  return 0.5f - 0.5f * std::cos(t * XM_PI); // S字(両端ゆっくり)
		default: return t;                                // まっすぐ
	}
}

// ===============================================================
// 初期化・読み込み
// ===============================================================
void ArcNote::Init()
{
	m_Layer = 2;   // 床(1)と判定ライン(3)の間
	m_BandTex = TextureManager::Load(L"asset\\texture\\ui\\assets\\arc_band.png");
	m_HeadTex = TextureManager::Load(L"asset\\texture\\ui\\assets\\arc_head.png");
}

void ArcNote::Update()
{
	const double now     = Conductor::GetSongTime();
	const float  cursorX = m_Manager->GetCursorX();

	for (ArcData& a : m_Arcs)
	{
		// 今乗っているか(見た目用。アークの途中のときだけ)
		bool inside = (now >= a.checks.front() && now <= a.checks.back());
		a.on = inside && std::fabs(cursorX - XAt(a, now)) <= ON_WIDTH;

		// 時刻を過ぎた判定を、その瞬間に乗っているかで決める
		while (a.nextCheck < (int)a.checks.size() && now >= a.checks[a.nextCheck])
		{
			double t  = a.checks[a.nextCheck];
			bool   ok = std::fabs(cursorX - XAt(a, t)) <= ON_WIDTH;

			int lane = std::clamp((int)XAt(a, t), 0, LANE_COUNT - 1);   // 光らせるレーン(x の下のレーン)
			m_Manager->OnJudge(ok ? Judge::Perfect : Judge::Miss, lane, 0.0);
			a.nextCheck++;
		}
	}
}

void ArcNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;
	const float W = 0.3f;   // 帯の幅の半分(レーン単位。0.6 レーンぶん)

	Begin3D();

	// 1. 帯
	for (const ArcData& arc : m_Arcs)
	{
		Layout L = m_Layout;
		L.height = (arc.layer == 1) ? UPPER_HEIGHT : 0.0f;

		for (size_t i = 0; i + 1 < arc.samples.size(); i++)
		{
			const ArcSample& s0 = arc.samples[i];       // 手前
			const ArcSample& s1 = arc.samples[i + 1];   // 奥

			float z0 = ((float)s0.time - now) * speed;
			float z1 = ((float)s1.time - now) * speed;
			if (z1 < -60.0f) continue;   // 通り過ぎた
			if (z0 > Z_FAR)  break;      // ここから先は全部まだ遠い
			if (z1 <= z0)    continue;   // 同じ時刻の点(長さ 0)は描けない

			// 奥ほど薄く(TapNote と同じ式)
			float    f0 = std::clamp((Z_FAR - z0) / (Z_FAR * 0.3f), 0.0f, 1.0f);
			float    f1 = std::clamp((Z_FAR - z1) / (Z_FAR * 0.3f), 0.0f, 1.0f);
			XMFLOAT4 nearColor(1.0f, 1.0f, 1.0f, f0);
			XMFLOAT4 farColor(1.0f, 1.0f, 1.0f, f1);

			// 画像の真ん中の1行だけ貼る(帯の「断面」を引き伸ばす)
			QuadBatch::Quad(L.World(s1.x - W, z1), L.World(s1.x + W, z1),
							L.World(s0.x + W, z0), L.World(s0.x - W, z0),
							farColor, nearColor,
							XMFLOAT2(0.0f, 0.5f), XMFLOAT2(1.0f, 0.5f));
		}
	}
	QuadBatch::Flush(m_BandTex);

	// 2. 判定ラインのところの印。乗っていると明るい
	for (const ArcData& arc : m_Arcs)
	{
		if (now < arc.samples.front().time || now > arc.samples.back().time) continue;

		float x = XAt(arc, now);   // ← その1の for で探していたところが、この1行に

		Layout L = m_Layout;
		L.height = (arc.layer == 1) ? UPPER_HEIGHT : 0.0f;

		const float s = 0.4f;
		const float h = s * L.laneWidth;
		XMFLOAT4 color(1.0f, 1.0f, 1.0f, arc.on ? 1.0f : 0.4f);   // 乗ってなければ薄く
		QuadBatch::Quad(L.World(x - s, h), L.World(x + s, h),
						L.World(x + s, -h), L.World(x - s, -h),
						color, color);
	}
	QuadBatch::Flush(m_HeadTex);

	End3D();
}

void ArcNote::Load(const Chart& chart)
{
	m_Arcs.clear();
	for (const ArcLine& arc : chart.arcs)
	{
		ArcData data;
		data.layer = arc.layer;

		// 点と点のあいだ(区間)ごとに、DIVISION 個に区切る
		for (size_t i = 0; i + 1 < arc.points.size(); i++)
		{
			const ArcPoint& a = arc.points[i];
			const ArcPoint& b = arc.points[i + 1];

			for (int k = 0; k < DIVISION; k++)
			{
				float t = (float)k / DIVISION;
				int   tick = a.tick + (int)std::lround((b.tick - a.tick) * t);

				ArcSample s;
				s.time = chart.TickToSec(tick);
				s.x = a.x + (b.x - a.x) * Ease(t, a.ease);
				data.samples.push_back(s);
			}
		}

		// 最後の点(上のループは「次の点の手前まで」なので、終点だけ足りない)
		const ArcPoint& last = arc.points.back();
		data.samples.push_back({ chart.TickToSec(last.tick), last.x });

		// 判定する時刻:始点・8分ごとの中継点・終点
		int startTick = arc.points.front().tick;
		int endTick = arc.points.back().tick;
		data.checks.push_back(chart.TickToSec(startTick));
		for (int t = startTick + TICK_STEP; t < endTick; t += TICK_STEP)
			data.checks.push_back(chart.TickToSec(t));
		data.checks.push_back(chart.TickToSec(endTick));

		m_Arcs.push_back(data);
	}
}

double ArcNote::GetLastTime() const
{
	double last = 0.0;
	for (const ArcData& a : m_Arcs)
		last = std::max(last, a.samples.back().time);
	return last;
}

void ArcNote::Reset(double startSec)
{
	for (ArcData& a : m_Arcs)
	{
		a.on = false;
		// 途中から再生なら、始まっているアークは丸ごと飛ばす(ロングと同じ)
		a.nextCheck = (a.checks.front() < startSec) ? (int)a.checks.size() : 0;
	}
}

int ArcNote::GetJudgeCount() const
{
	int count = 0;
	for (const ArcData& a : m_Arcs)
		count += (int)a.checks.size();   // 始点 + 中継点 + 終点
	return count;
}

// その時刻のアークの x。始まる前は始点の x、終わったあとは終点の x
float ArcNote::XAt(const ArcData& arc, double time)
{
	const std::vector<ArcSample>& s = arc.samples;
	if (time <= s.front().time) return s.front().x;

	for (size_t i = 0; i + 1 < s.size(); i++)
	{
		if (time > s[i + 1].time) continue;   // まだ先

		// s[i] と s[i+1] のあいだ → 割合で混ぜる
		double span = s[i + 1].time - s[i].time;
		float  t = (span > 0.0) ? (float)((time - s[i].time) / span) : 1.0f;
		return s[i].x + (s[i + 1].x - s[i].x) * t;
	}
	return s.back().x;
}