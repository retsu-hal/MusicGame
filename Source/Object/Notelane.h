#pragma once
#include "GameObject.h"
#include "Chart.h"
#include "Lane.h"
#include "TapNote.h"
#include <vector>
#include <string>

class Audio;

// 判定の種類
enum class Judge
{
	None,      // まだ判定していない
	Perfect,
	Great,
	Good,
	Miss,
	Skip,      // 途中から再生したときに飛ばしたノーツ（数えない）
};

// 叩いたときの演出1つ分
struct HitEffect
{
	int   lane;
	Judge judge;
	float time = 0.0f;   // 発生してからの経過時間（秒）
};

class Notelane : public GameObject
{
private:
	Audio* m_Music{ nullptr };
	Chart  m_Chart;
	std::vector<double> m_NoteTimes;   // 各ノーツの時刻（秒）
	std::vector<Judge>  m_Judges;      // 各ノーツの判定結果

	// 見た目
	LaneDraw::Layout m_Layout;         // レーンの本数・幅・判定ラインの高さ
	Lane             m_Lane;           // レーンの描画（Lane.cpp）
	TapNote          m_TapNote;        // タップノーツの描画（TapNote.cpp）

	// 判定の結果
	int    m_Count[6] = {};            // Judge ごとの個数
	double m_DiffSum   = 0.0;          // ずれの合計（平均を出すため）
	int    m_HitCount  = 0;            // 叩いたノーツの数
	double m_LastInputLag = 0.0;       // 入力→判定の遅れ（秒、確認用）

	// スコアとコンボ
	double m_ScoreRaw  = 0.0;          // 小数のまま足していくスコア
	int    m_Combo     = 0;
	int    m_MaxCombo  = 0;
	float  m_ComboPop  = 0.0f;         // コンボが増えた瞬間に文字を大きくする演出（1→0）

	// 曲の終わり
	double m_EndTime   = 0.0;          // この時刻を過ぎたら終了
	bool   m_Finished  = false;

	// 演出
	std::vector<HitEffect> m_Effects;

	void JudgeLane(int lane, double now);
	void SetJudge(size_t index, Judge judge, double diff);
	void Finish();

	void DrawEffects();
	void DrawHUD();

public:
	void Uninit() override;
	void Update() override;
	void Draw() override;

	bool LoadChart(const std::string& path);
	void Play(double startSec);        // 途中から（デバッグ用）
	void PlayFromStart();              // カウントインを付けて最初から

	bool IsFinished() const { return m_Finished; }
	int  GetScore() const;
};
