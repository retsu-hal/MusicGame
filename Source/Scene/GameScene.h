#pragma once
#include "Scene.h"

class Notelane;
class Score;

class GameScene : public Scene
{
private:
	Notelane* m_Lane = nullptr;
	Score*    m_Score = nullptr;

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};
