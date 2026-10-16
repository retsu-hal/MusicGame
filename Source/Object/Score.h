#pragma once

#include "GameObject.h"

class Score : public GameObject
{
private:
	static const int DIGIT_NUM = 7;//桁数（最大 1000000 点）
public:
	int m_Score = 0;        // 実際に表示されるスコア（徐々に増える）
	int m_TargetScore = 0;  // 最終的に到達すべきスコア

	float m_PopTimer = 0.0f;   // ポップ演出用タイマー
	float m_CountTimer = 0.0f; // カウントアップの間隔調整用

	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void AddScore(int value) { m_TargetScore += value; }
	void SetTargetScore(int value) { m_TargetScore = value; }   // Notelane のスコアに合わせる
	int GetScore() const { return m_Score; }
};