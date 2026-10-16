/*==============================================================================

[TitleScene.cpp]
														Author :Watanabe Retsu
														Date   :2026/07/01
--------------------------------------------------------------------------------

==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "Polygon2D.h"
#include "TitleScene.h"
#include "GameScene.h"
#include <algorithm>		// std::min
#include "audio.h"
//==============================================================================
//マクロ宣言
//==============================================================================
#define UI_SCALE 0.75f	//UIのスケール倍率

#define MENU_X     (208.0f)		// メニューの文字の左端
#define MENU_Y     (540.0f)		// 1つ目の項目の上端
#define MENU_PITCH (84.0f)		// 項目の間隔

#define MENU_X     (208.0f)		// メニューの文字の左端
#define MENU_Y     (540.0f)		// 1つ目の項目の上端
#define MENU_PITCH (84.0f)		// 項目の間隔

// 選択中の帯(=クリックの当たり範囲)。1つ目の項目の行の四角
#define CURSOR_X   (168.0f)
#define CURSOR_Y   (MENU_Y - 9.0f)
#define CURSOR_W   (660.0f)
#define CURSOR_H   (72.0f)

#define FADE_TIME  (0.5f)		// フェードにかける時間(秒)
//==============================================================================
//プロトタイプ宣言
//==============================================================================

//==============================================================================
//グローバル変数
//==============================================================================
static const XMFLOAT4 COLOR_SELECTED = { 0.949f, 0.941f, 0.980f, 1.0f };	// #F2F0FA 選択中
static const XMFLOAT4 COLOR_NORMAL = { 0.541f, 0.525f, 0.659f, 1.0f };	// #8A86A8 選択していない
static const XMFLOAT4 COLOR_ACCENT = { 0.435f, 0.890f, 1.0f,   1.0f };	// #6FE3FF 強調(水色)
static const XMFLOAT4 COLOR_CURSOR = { 0.486f, 0.424f, 1.0f,   0.25f };	// 帯のうす紫(半透明)
//==============================================================================
//初期化処理
//==============================================================================
void TitleScene::Init()
{
	//背景
	Manager::AddGameObject<Polygon2D>()->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\TitleBG.png");

	//エンブレム(ロゴの左の丸い印)
	Polygon2D* emblem = Manager::AddGameObject<Polygon2D>();
	emblem->Init(127.0f, 228.0f, 1107.0f * 0.19f, 896.0f * 0.19f, L"asset\\texture\\logo.png");

	//タイトルロゴ「Re:sonare」
	m_Logo = Manager::AddGameObject<Polygon2D>();
	m_Logo->Init(343.0f, 203.0f, 866.0f * UI_SCALE, 238.0f * UI_SCALE, L"asset\\texture\\ui\\text\\title\\resonare.png");
	m_Logo->SetColor(XMFLOAT4(0.949f, 0.941f, 0.980f, 1.0f));	
	m_Logo->SetPixelShader("shader\\logoPS.cso");	// 縁を光が走るシェーダー
	m_Time = 0.0f;

	//メニュー
	m_Select = MENU_START;

	//選択中の帯と線(文字より先に足して、文字の下に描かれるようにする)
	m_Cursor = Manager::AddGameObject<Polygon2D>();
	m_Cursor->Init(CURSOR_X, CURSOR_Y, CURSOR_W, CURSOR_H, L"asset\\texture\\ui\\assets\\white_square.png");
	m_Cursor->SetColor(COLOR_CURSOR);
	m_Cursor->SetGradientAlpha(1.0f, 0.0f);		// 左はそのまま、右端で消える

	m_CursorLine = Manager::AddGameObject<Polygon2D>();
	m_CursorLine->Init(180.0f, MENU_Y + 3.0f, 6.0f, 48.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	m_CursorLine->SetColor(COLOR_ACCENT);

	//項目ごとの画像と大きさ(画像の元のピクセル数)
	const WCHAR* textFile[MENU_MAX] = { L"asset\\texture\\ui\\text\\menu\\start.png", L"asset\\texture\\ui\\text\\menu\\exit.png" };
	const float textWidth[MENU_MAX] = { 190.0f, 138.0f };
	const float subWidth[MENU_MAX] = { 96.0f, 70.0f };
	const float subX[MENU_MAX] = { 367.0f, 329.0f };	// 英語の文字の長さが違うので、日本語の位置も項目ごと

	for (int i = 0; i < MENU_MAX; i++)
	{
		float y = MENU_Y + MENU_PITCH * i;

		m_MenuText[i] = Manager::AddGameObject<Polygon2D>();
		m_MenuText[i]->Init(MENU_X, y, textWidth[i] * UI_SCALE, 80.0f * UI_SCALE, textFile[i]);
	}

	UpdateMenuLook();

	//キーガイド(画面下)
//区切り線
	Polygon2D* guideLine = Manager::AddGameObject<Polygon2D>();
	guideLine->Init(0.0f, 1002.0f, SCREEN_WIDTH, 2.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	guideLine->SetColor(XMFLOAT4(0.541f, 0.525f, 0.659f, 0.3f));	// #8A86A8 をうすく

	//キーの画像を左から順に並べる
	const int GUIDE_NUM = 2;
	const WCHAR* guideFile[GUIDE_NUM] = { L"asset\\texture\\ui\\keyguide\\kg_click_decide.png", L"asset\\texture\\ui\\keyguide\\kg_esc_exit.png" };
	const float guideWidth[GUIDE_NUM] = { 200.0f, 182.0f };

	float guideX = 66.0f;
	for (int i = 0; i < GUIDE_NUM; i++)
	{
		Polygon2D* guide = Manager::AddGameObject<Polygon2D>();
		guide->Init(guideX, 1005.0f, guideWidth[i] * UI_SCALE, 104.0f * UI_SCALE, guideFile[i]);
		guide->SetColor(COLOR_NORMAL);

		guideX += guideWidth[i] * UI_SCALE + 30.0f;	// 次の画像は、今の画像の右端 + 30
	}

	//フェード用の黒い板(一番最後に足して、全部の上に描く)
	m_Fade = Manager::AddGameObject<Polygon2D>();
	m_Fade->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\ui\\assets\\white_square.png");
	m_Fade->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));	// 真っ黒・不透明から始める
	m_FadeTime = 0.0f;
	m_IsLeaving = false;


	//効果音(表示はしない GameObject に Audio を2つ付ける)
	GameObject* sound = Manager::AddGameObject<GameObject>();
	m_SelectSE = sound->AddComponent<Audio>();
	m_SelectSE->Load("asset\\audio\\se\\se_select.wav");

	m_DecideSE = sound->AddComponent<Audio>();
	m_DecideSE->Load("asset\\audio\\se\\se_click.wav");

	m_BGM = sound->AddComponent<Audio>();
	m_BGM->Load("asset\\audio\\bgm\\bgm_title.wav");
	m_BGM->SetVolume(0.3f);	
	m_BGM->Play(true);	// ループ再生


}

//==============================================================================
//終了処理
//==============================================================================
void TitleScene::Uninit()
{

}

//==============================================================================
//更新処理
//==============================================================================
void TitleScene::Update()
{
	//ロゴの光
	m_Time += Manager::GetDeltaTime();
	m_Logo->SetParam(XMFLOAT4(m_Time, 0.0f, 0.0f, 0.0f));

	//フェード
	m_FadeTime += Manager::GetDeltaTime();
	float t = std::min(m_FadeTime / FADE_TIME, 1.0f);	// 0 → 1 に進む割合
	m_Fade->SetAlpha(m_IsLeaving ? t : 1.0f - t);		// 出ていくときは濃く、入ってきたときは薄く

	if (m_IsLeaving)
	{
		return;		// 出ていく途中は操作を受け付けない(2回押し・ホバーの色変えを防ぐ)
	}

	//メニューの選択(マウスが乗っている行を選択中にする)
	int hover = HitMenu(Input::GetMouseX(), Input::GetMouseY());
	if (hover >= 0 && hover != m_Select)
	{
		m_Select = hover;
		UpdateMenuLook();
		m_SelectSE->Play();
	}

	//決定(項目の上で左クリック)
	if (Input::GetMouseTrigger(Input::MOUSE_LEFT))
	{
		int hit = HitMenu(Input::GetMouseX(), Input::GetMouseY());
		if (hit >= 0)
		{
			//クリックした項目を選択中にしてから決定する
			m_Select = hit;
			UpdateMenuLook();
			m_DecideSE->Play();

			switch (m_Select)
			{
				case MENU_START:
					Manager::ChangeScene<GameScene>(FADE_TIME);	// 黒くなりきってから切り替える
					m_IsLeaving = true;
					m_FadeTime = 0.0f;							// フェードアウトを 0 秒から始める
					break;

				case MENU_EXIT:
					PostQuitMessage(0);					// ゲームを終了する
					break;
			}
		}
	}

	//Esc でも終了(キーガイドの「Esc 終了」)
	if (Input::GetKeyTrigger(VK_ESCAPE))
	{
		PostQuitMessage(0);
	}

}

//==============================================================================
//描画処理
//==============================================================================
void TitleScene::Draw()
{

}

//==============================================================================
//メニューの見た目を選択に合わせる
//==============================================================================
void TitleScene::UpdateMenuLook()
{
	for (int i = 0; i < MENU_MAX; i++)
	{
		bool selected = (i == m_Select);
		m_MenuText[i]->SetColor(selected ? COLOR_SELECTED : COLOR_NORMAL);
	}

	//帯と線を選んでいる行へ動かす(Init で置いた START の行からのずらし量)
	Vector3 offset(0.0f, MENU_PITCH * m_Select, 0.0f);
	m_Cursor->SetPosition(offset);
	m_CursorLine->SetPosition(offset);
}

//==============================================================================
//画面の (x, y) がどの項目の上にあるか調べる
//==============================================================================
int TitleScene::HitMenu(int x, int y)
{
	for (int i = 0; i < MENU_MAX; i++)
	{
		float left = CURSOR_X;
		float top = CURSOR_Y + MENU_PITCH * i;	// i 行目の帯の上端
		float right = left + CURSOR_W;
		float bottom = top + CURSOR_H;

		if (x >= left && x < right && y >= top && y < bottom)
		{
			return i;	// i 番目の項目に当たった
		}
	}

	return -1;			// どれにも当たらなかった
}