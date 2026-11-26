#pragma once

#include "GameObject.h"

// 達成率(右上)。判定の割合を足していって、% で表示する
class Score : public GameObject
{
private:
	static const int DIGIT_NUM = 7;               // 表示する最大の文字数("101.00%")
	static const int QUAD_FRAME = DIGIT_NUM;      // 枠の四角の番号(8 枚目)
	static const int QUAD_LABEL = DIGIT_NUM + 1;  // ACHIEVEMENT の四角の番号(9 枚目)
	static const int QUAD_NUM = DIGIT_NUM + 2;  // 四角の数

	int    m_TotalJudges = 0;     // 判定の回数の合計(ロングの中継点も1回)
	double m_RateSum = 0.0;   // 判定の割合の合計

	int   m_Score = 0;      // 表示中の達成率(0.01% 単位。10042 = 100.42%)。だんだん増える
	int   m_TargetScore = 0;      // 本当の達成率(0.01% 単位)


	ID3D11ShaderResourceView* m_FrameTex = nullptr;  
	ID3D11ShaderResourceView* m_LabelTex = nullptr;
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