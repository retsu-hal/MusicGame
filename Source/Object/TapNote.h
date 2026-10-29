#pragma once
#include "NoteBase.h"
#include "Judge.h"
#include "Lane.h"
#include <vector>

// タップノーツ1個ぶんのデータ
struct TapData
{
	double time = 0.0; // 叩く時刻(秒)
	int lane = 0;
	int layer = 0;			   // 0:下段 1:上段
	Judge judge = Judge::None; // 判定の結果(None = まだ)
};

// タップノーツ全部。レイヤー2(床と判定ラインの間)
class TapNote : public NoteBase
{
private:
	Layout m_Layout;
	std::vector<TapData> m_Notes; // 時刻の順に並んでいる

	void JudgeLane(int lane, int layer, double hitTime); // キーが押されたレーンを判定
	void SetJudge(TapData &note, Judge judge, double diff);

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void Load(const Chart &chart) override;
	void Reset(double startSec) override;
	int GetJudgeCount() const override { return (int)m_Notes.size(); }
	double GetLastTime() const override { return m_Notes.empty() ? 0.0 : m_Notes.back().time; }
};