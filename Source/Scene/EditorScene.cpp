#include "main.h"
#include "Manager.h"
#include "EditorScene.h"
#include "TitleScene.h"
#include "ChartEditor.h"

void EditorScene::Init()
{
	m_Editor = Manager::AddGameObject<ChartEditor>();
}

void EditorScene::Update()
{
	// Esc の判定と「保存していません」の確認はエディタ側で行い、
	// シーンは「戻りたい」と言われたら切り替えるだけ
	if (m_Editor && m_Editor->IsExitRequested())
		Manager::ChangeScene<TitleScene>();
}
