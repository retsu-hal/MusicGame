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
#include "Effect.h"
#include "JudgeLine.h"
#include "SongPanel.h"

#include "GameScene.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include "SelectScene.h"		

//==============================================================================
//マクロ宣言
//==============================================================================
// 
//==============================================================================
//初期化処理
//==============================================================================
void GameScene::Init()
{
	// 背景
	Polygon2D* bg = Manager::AddGameObject<Polygon2D>();
	bg->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\02_Play\\PlayBG.png");
	bg->SetUV(0.0556f, 0.0f, 0.8889f, 1.0f);
	bg->SetLayer(0);

	// カメラ:今のレーンの見た目(Project)と同じに合わせる
	CAMERA* camera = Manager::AddGameObject<CAMERA>();
	Layout layout;                                            // 判定ラインの高さなどの初期値
	camera->SetPosition(Vector3(0.0f, 805.0f, -CAMERA_DIST));   // 判定ラインの手前
	camera->SetTarget(Vector3(0.0f, 805.0f, 0.0f));                     // 床と平行に、真っすぐ奥
	camera->SetFocalLength(CAMERA_DIST);
	camera->SetLensCenter(SCREEN_WIDTH * 0.5f, HORIZON_Y);    // 地平線が y = 90 に来る

	m_Lane = Manager::AddGameObject<Lane>();
	m_JudgeLine = Manager::AddGameObject<JudgeLine>();
	m_Score = Manager::AddGameObject<Score>();

	// 進行バー:Score と同じレイヤー 3(ポーズの幕の下)。Score のあとに足すので枠の上に描かれる
	m_Progress = Manager::AddGameObject<Polygon2D>();
	m_Progress->Init(1362.0f, 164.0f, 504.0f, 8.0f, L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_Progress->SetPixelShader("shader\\progressPS.cso");
	m_Progress->SetLayer(3);

	m_Combo = Manager::AddGameObject<Combo>();
	Manager::AddGameObject<SongPanel>();
	Manager::AddGameObject<Effect>();

	// 選曲画面などで決めた譜面を読み込んで、カウントイン付きで開始
	m_NoteManager = Manager::AddGameObject<NoteManager>();
	bool loaded = m_NoteManager->LoadChart(GameSession::chartPath);

	m_Pause.Init();
	m_TutorialWindow.Init();		// ポーズの後に足す(どっちもレイヤー3。同時には開かないので順番は気にしなくていい)

	if (loaded)
	{
		if (GameSession::tutorial)
		{
			// チュートリアル:曲はまだ始めない。ヘルプを閉じたら Play する
			Input::SetMouseMode(Input::MOUSE_MODE_ABSOLUTE);	// ボタンを押すのでカーソルを出す
			m_TutorialWindow.Open();
		}
		else
		{
			m_NoteManager->Play();
		}
	}
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
		// チュートリアルのヘルプ中は、ヘルプだけ動かす(曲はまだ始まっていない)
		if (m_TutorialWindow.IsOpen())
		{
			if (m_TutorialWindow.Update() == TutorialWindow::RESULT_CLOSE)
			{
				Input::SetMouseMode(Input::MOUSE_MODE_RELATIVE);	// プレイ用のマウスに戻す
				m_NoteManager->Play();								// カウントイン付きで開始
			}
			return;
		}

	// ポーズ中はメニューだけ動かして、ほかは止める
	if (m_Pause.IsOpen())
	{
		switch (m_Pause.Update())
		{
			case PauseMenu::RESULT_RESTART:     // カウント終わり → 続きから
				Input::SetMouseMode(Input::MOUSE_MODE_RELATIVE);
				Conductor::ReStart();
				break;
			case PauseMenu::RESULT_RETRY:       // 最初から
				Input::SetMouseMode(Input::MOUSE_MODE_RELATIVE);
				m_NoteManager->Play();
				break;
			case PauseMenu::RESULT_SELECT:      // 選曲へ戻る
				Manager::ChangeScene<SelectScene>();
				break;
			default:
				break;
		}
		return;
	}

	// 進行バー:曲の時間 / 曲の長さ。カウントイン中はマイナスなので 0〜1 に収める
	double length = m_NoteManager ? m_NoteManager->GetLength() : 0.0;
	if (length > 0.0)
	{
		float progress = (float)std::clamp(Conductor::GetSongTime() / length, 0.0, 1.0);
		m_Progress->SetParam(XMFLOAT4(progress, 0.0f, 0.0f, 0.0f));
	}

	// 曲が終わったらリザルトへ（結果は NoteManager が GameSession に書いてある）
	if (m_NoteManager && m_NoteManager->IsFinished())
		Manager::ChangeScene<ResultScene>();

	// Esc：ポーズを開く
	if (Input::GetKeyTrigger(VK_ESCAPE))
	{
		Conductor::Pause();                                 // 曲と時間を止める
		Input::SetMouseMode(Input::MOUSE_MODE_ABSOLUTE);    // メニューをクリックするのでカーソルを出す
		m_Pause.Open();
	}
}

//==============================================================================
//描画処理
//==============================================================================
void GameScene::Draw()
{
}
