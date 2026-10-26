#pragma once

#include "GameObject.h"

// 達成率(右上)。判定の割合を足していって、% で表示する
class Score : public GameObject
{
private:
	static const int DIGIT_NUM = 7;   // 表示する最大の文字数("101.00%")

	int    m_TotalJudges = 0;     // 判定の回数の合計(ロングの中継点も1回)
	double m_RateSum = 0.0;   // 判定の割合の合計

	int   m_Score = 0;      // 表示中の達成率(0.01% 単位。10042 = 100.42%)。だんだん増える
	int   m_TargetScore = 0;      // 本当の達成率(0.01% 単位)

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void   Reset();                                          // 最初からやり直すとき
	void   SetTotalJudges(int total) { m_TotalJudges = total; }
	void   AddScore(double rate);                            // 判定の割合を足す(PERFECT 1.0 など)
	double GetAchievement() const;                           // 本当の達成率(%)。リザルト用
};