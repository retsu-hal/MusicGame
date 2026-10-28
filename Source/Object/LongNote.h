#pragma once
#include "NoteBase.h"
#include "Judge.h"
#include "Lane.h"
#include <vector>

// ロングノーツ1個ぶんのデータ
struct LongData
{
	double startTime = 0.0; // 始点の時刻(秒)
	double endTime = 0.0;	// 終点の時刻(秒)
	int lane = 0;
	int layer = 0; // 0:下段 1:上段

	std::vector<double> ticks;		// 中継点の時刻(秒)。時刻の順
	int nextTick = 0;				// 次に判定する中継点の番号
	Judge startJudge = Judge::None; // 始点の判定(None = まだ)
	Judge endJudge = Judge::None;	// 終点の判定(None = まだ)
};

// ロングノーツ全部。レイヤー2(床と判定ラインの間)
class LongNote : public NoteBase
{
private:
	Layout m_Layout;
	std::vector<LongData> m_Notes;				   // 始点の時刻の順
	ID3D11ShaderResourceView *m_BodyTex = nullptr; // 帯(longnote.png)
	ID3D11ShaderResourceView *m_CapTex = nullptr;  // 始点・終点(tapnote.png)
	bool m_WasHeld[LANE_COUNT] = {};

	void JudgeStart(int lane, int layer, double hitTime);				 // 押した瞬間:始点
	void JudgeTicks(LongData &note, double now, bool held);				 // 中継点
	void JudgeEnd(LongData &note, double now, bool held, bool released); // 終点
	void SetJudge(LongData &note, Judge judge, double diff);

public:
	void Init() override;
	void Update() override;
	void Draw() override;

	void Load(const Chart &chart) override;
	void Reset(double startSec) override;
	int GetJudgeCount() const override;
	double GetLastTime() const override;
};