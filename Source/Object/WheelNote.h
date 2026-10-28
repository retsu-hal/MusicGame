#pragma once
#include "NoteBase.h"
#include "Lane.h"
#include "Judge.h"
#include <vector>

// ホイールフリック1個ぶんのデータ
struct WheelData
{
	double time = 0.0; // 回す時刻(秒)
	int dir = 0;	   // 1:上 -1:下
	Judge judge = Judge::None;
};

// ホイールフリック全部。レイヤー2(床と判定ラインの間)
class WheelNote : public NoteBase
{
private:
	std::vector<WheelData> m_Notes; // 時刻の順
	Layout m_Layout;
	ID3D11ShaderResourceView *m_UpTex = nullptr;   // flick_wheel_up.png
	ID3D11ShaderResourceView *m_DownTex = nullptr; // flick_wheel_down.png

	void DrawDir(int dir, float now, float speed); // 片方の向きだけ描く
	void SetJudge(WheelData &note, Judge judge, double diff);

public:
	void Init() override;
	void Update() override;
	void Draw() override;

	void Load(const Chart &chart) override;
	void Reset(double startSec) override;
	int GetJudgeCount() const override { return (int)m_Notes.size(); }
	double GetLastTime() const override { return m_Notes.empty() ? 0.0 : m_Notes.back().time; }
};