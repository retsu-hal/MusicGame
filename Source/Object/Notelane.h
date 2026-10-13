#pragma once
#include "GameObject.h"
#include "Chart.h"
#include <vector>
#include <string>

class Audio;
struct ImVec2;

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

	// 見た目の設定
	int   m_LaneCount  = 4;
	float m_LaneWidth  = 120.0f;       // 判定ライン上でのレーン幅（ピクセル）
	float m_JudgeLineY = 600.0f;       // 判定ラインの高さ

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
	float m_LaneGlow[4] = {};
	std::vector<HitEffect> m_Effects;

	void JudgeLane(int lane, double now);
	void SetJudge(size_t index, Judge judge, double diff);
	void Finish();

	ImVec2 Project(float laneX, float z) const;
	void DrawLane();
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
