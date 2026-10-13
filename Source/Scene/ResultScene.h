#pragma once
#include "Scene.h"

class ResultScene : public Scene
{
private:
	float m_Time = 0.0f;   // シーンが始まってからの時間（演出用）

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};
