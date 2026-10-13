#pragma once
#include "Scene.h"

class Notelane;

class GameScene : public Scene
{
private:
	Notelane* m_Lane = nullptr;

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};
