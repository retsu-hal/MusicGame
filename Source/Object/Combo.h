#pragma once

#include "GameObject.h"

// コンボ数(レーンの右)。2 コンボ以上で表示する
class Combo : public GameObject
{
private:
	static const int DIGIT_NUM = 4;   // 表示する最大の桁数(9999 まで)

	int   m_Combo = 0;
	int   m_MaxCombo = 0;
public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void Reset();                    // 最初からやり直すとき
	void AddCombo();                 // つながった
	void ResetCombo();               // 切れた(BAD・MISS)
	int  GetMaxCombo() const { return m_MaxCombo; }
};