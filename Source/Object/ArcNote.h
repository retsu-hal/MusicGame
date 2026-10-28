#pragma once
#include "NoteBase.h"
#include "Lane.h"
#include <vector>

// 細かく区切ったアークの1点(読み込みのときに計算しておく)
struct ArcSample
{
	double time = 0.0;   // 秒
	float  x = 0.0f;  // レーン単位
};

// アーク1本ぶんのデータ
struct ArcData
{
	int                    layer = 0;
	std::vector<ArcSample> samples;   // 時刻の順

	std::vector<double> checks;      // 判定する時刻(始点・中継点・終点)。時刻の順
	int                 nextCheck = 0;
	bool                on = false;  // 今カーソルが乗っているか(見た目用)
};

// アーク全部。レイヤー2(床と判定ラインの間)
class ArcNote : public NoteBase
{
private:
	std::vector<ArcData>      m_Arcs;
	Layout m_Layout;
	ID3D11ShaderResourceView* m_BandTex = nullptr;
	ID3D11ShaderResourceView* m_HeadTex = nullptr;

	static constexpr int DIVISION = 16;

	static float XAt(const ArcData& arc, double time);  

public:
	void Init() override;
	void Update() override;   
	void Draw() override;

	void   Load(const Chart& chart) override;
	void   Reset(double startSec) override;  
	int    GetJudgeCount() const override;    
	double GetLastTime() const override;
};
