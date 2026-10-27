/*==============================================================================

[GameScene.cpp]
														Author :Watanabe Retsu
														Date   :
--------------------------------------------------------------------------------

==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "NoteManager.h"
#include "GameSession.h"
#include "score.h"
#include "Combo.h"
#include "Camera.h"
#include "Polygon2D.h"
#include "Lane.h"
#include "Conductor.h"

#include "GameScene.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include "SelectScene.h"		
//==============================================================================
//初期化処理
//==============================================================================
void GameScene::Init()
{
	Input::SetMouseMode(Input::MOUSE_MODE_RELATIVE);   // カーソルを消して、移動量だけ取る

	// 背景(ほかの 2D より先に足す)
	Polygon2D* bg = Manager::AddGameObject<Polygon2D>();
	bg->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\backGround.png");
	bg->SetUV(0.0556f, 0.0f, 0.8889f, 1.0f);
	bg->SetLayer(0);

	// カメラ:今のレーンの見た目(Project)と同じに合わせる
	CAMERA* camera = Manager::AddGameObject<CAMERA>();
	LaneDraw::Layout layout;                                            // 判定ラインの高さなどの初期値
	float eyeY = layout.judgeLineY - LaneDraw::HORIZON_Y;               // カメラの高さ
	camera->SetPosition(Vector3(0.0f, eyeY, -LaneDraw::CAMERA_DIST));   // 判定ラインの手前
	camera->SetTarget(Vector3(0.0f, eyeY, 0.0f));                     // 床と平行に、真っすぐ奥
	camera->SetFocalLength(LaneDraw::CAMERA_DIST);
	camera->SetLensCenter(SCREEN_WIDTH * 0.5f, LaneDraw::HORIZON_Y);    // 地平線が y = 90 に来る

	m_Lane = Manager::AddGameObject<Lane>();
	m_JudgeLine = Manager::AddGameObject<JudgeLine>();
	m_Score = Manager::AddGameObject<Score>();
	m_Combo = Manager::AddGameObject<Combo>();

	// 選曲画面などで決めた譜面を読み込んで、カウントイン付きで開始
	m_NoteManager = Manager::AddGameObject<NoteManager>();
	if (m_NoteManager->LoadChart(GameSession::chartPath))
		m_NoteManager->PlayFromStart();
}

//==============================================================================
//終了処理
//==============================================================================
void GameScene::Uninit()
{
	Conductor::Stop();
	Input::SetMouseMode(Input::MOUSE_MODE_ABSOLUTE);   // ← 追加。ほかの画面ではふつうのマウスに戻す
}

//==============================================================================
//更新処理
//==============================================================================
void GameScene::Update()
{

	// 曲が終わったらリザルトへ（結果は NoteManager が GameSession に書いてある）
	if (m_NoteManager && m_NoteManager->IsFinished())
		Manager::ChangeScene<ResultScene>();

	// Esc：途中でやめて選曲へ(ポーズを作ったらポーズを開くように変える)
	if (Input::GetKeyTrigger(VK_ESCAPE))
		Manager::ChangeScene<SelectScene>();
}

//==============================================================================
//描画処理
//==============================================================================
void GameScene::Draw()
{
}
