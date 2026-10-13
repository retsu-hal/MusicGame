#pragma once
#include "Scene.h"

class ChartEditor;

// 譜面エディタのシーン（タイトルで F2、Esc でタイトルに戻る）
class EditorScene : public Scene
{
private:
	ChartEditor* m_Editor = nullptr;

public:
	void Init() override;
	void Update() override;
};
