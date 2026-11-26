#pragma once
#include "Scene.h"
#include "Pause.h"
#include "TutorialWindow.h"

class NoteManager;
class Score;
class Combo;
class Lane;
class JudgeLine;
class Polygon2D;

class GameScene : public Scene
{
private:
	NoteManager* m_NoteManager = nullptr;
	Score*    m_Score = nullptr;
	Combo* m_Combo = nullptr;
	Lane* m_Lane = nullptr;
	JudgeLine* m_JudgeLine = nullptr;
	Polygon2D* m_Progress = nullptr;
	PauseMenu m_Pause;
	TutorialWindow m_TutorialWindow;

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};
