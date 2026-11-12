#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "ResultScene.h"
#include "SelectScene.h"

void ResultScene::Init()
{
	m_Time = 0.0f;
}

void ResultScene::Uninit()
{
}

void ResultScene::Update()
{
	m_Time += Manager::GetDeltaTime();

	// 少し待ってから受け付ける（ゲームの最後の打鍵で飛ばしてしまわないように）
	if (m_Time > 0.5f && (Input::GetKeyTrigger(VK_RETURN) || Input::GetKeyTrigger(VK_ESCAPE)))
		Manager::ChangeScene<SelectScene>();
}

void ResultScene::Draw()
{
}
