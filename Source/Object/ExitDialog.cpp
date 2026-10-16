/*==============================================================================

[ExitDialog.cpp]
														Author :Watanabe Retsu
														Date   :2026/10/16
--------------------------------------------------------------------------------
 終了確認ダイアログ
==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "Polygon2D.h"
#include "Audio.h"
#include "ExitDialog.h"

//==============================================================================
//マクロ宣言
//==============================================================================
#define UI_SCALE   (0.75f)		// UI 画像は 1280 基準の 2 倍で作ってあるので 0.75 倍で描く

// ボタンの枠の四角(光の余白は含まない)
#define BTN_Y      (552.0f)
#define BTN_W      (294.0f)
#define BTN_H      (84.0f)
#define BTN_MARGIN (18.0f)		// ボタン画像の光の余白(24 × 0.75)

//==============================================================================
//グローバル変数
//==============================================================================
static const XMFLOAT4 COLOR_SELECTED = { 0.949f, 0.941f, 0.980f, 1.0f };	// #F2F0FA 選択中
static const XMFLOAT4 COLOR_NORMAL = { 0.541f, 0.525f, 0.659f, 1.0f };	// #8A86A8 選択していない
static const XMFLOAT4 COLOR_ACCENT = { 0.435f, 0.890f, 1.0f,   1.0f };	// #6FE3FF 強調(水色)

static const float BTN_X[] = { 630.0f, 960.0f };	// YES / NO の枠の画像の左上 x(光の余白込み)

//==============================================================================
//初期化処理
//==============================================================================
void ExitDialog::Init(Audio* selectSE, Audio* decideSE)
{
	m_SelectSE = selectSE;
	m_DecideSE = decideSE;

	//後ろを暗くする板
	m_Dim = Manager::AddGameObject<Polygon2D>();
	m_Dim->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\ui\\assets\\white_square.png");
	m_Dim->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

	//枠・見出し・質問
	Polygon2D* panel = Manager::AddGameObject<Polygon2D>();
	panel->Init(585.0f, 327.0f, 1000.0f * UI_SCALE, 504.0f * UI_SCALE, L"asset\\texture\\ui\\assets\\exit_panel.png");

	Polygon2D* label = Manager::AddGameObject<Polygon2D>();
	label->Init(621.0f, 355.0f, 380.0f * UI_SCALE, 48.0f * UI_SCALE, L"asset\\texture\\ui\\text\\label\\terminate_session.png");
	label->SetColor(COLOR_ACCENT);

	Polygon2D* question = Manager::AddGameObject<Polygon2D>();
	question->Init(743.0f, 427.5f, 574.0f * UI_SCALE, 86.0f * UI_SCALE, L"asset\\texture\\ui\\text\\jp\\confirm_exit.png");
	question->SetColor(COLOR_SELECTED);

	m_Parts = { panel, label, question };

	//ボタンの枠(選んでいない枠と選んでいる枠を同じ場所に重ねる)
	for (int i = 0; i < BUTTON_MAX; i++)
	{
		m_BtnNormal[i] = Manager::AddGameObject<Polygon2D>();
		m_BtnNormal[i]->Init(BTN_X[i], BTN_Y - BTN_MARGIN, 440.0f * UI_SCALE, 160.0f * UI_SCALE, L"asset\\texture\\ui\\assets\\btn_normal.png");

		m_BtnSelected[i] = Manager::AddGameObject<Polygon2D>();
		m_BtnSelected[i]->Init(BTN_X[i], BTN_Y - BTN_MARGIN, 440.0f * UI_SCALE, 160.0f * UI_SCALE, L"asset\\texture\\ui\\assets\\btn_selected.png");
	}

	//ボタンの文字(枠より後に足して、枠の上に描く)
	m_BtnText[BUTTON_YES] = Manager::AddGameObject<Polygon2D>();
	m_BtnText[BUTTON_YES]->Init(750.0f, 568.5f, 120.0f * UI_SCALE, 80.0f * UI_SCALE, L"asset\\texture\\ui\\text\\menu\\yes.png");

	m_BtnText[BUTTON_NO] = Manager::AddGameObject<Polygon2D>();
	m_BtnText[BUTTON_NO]->Init(1092.0f, 567.0f, 90.0f * UI_SCALE, 80.0f * UI_SCALE, L"asset\\texture\\ui\\text\\menu\\no.png");

	//最初は隠しておく
	Close();
}

//==============================================================================
//開く
//==============================================================================
void ExitDialog::Open()
{
	m_IsOpen = true;
	m_Select = BUTTON_YES;			// 開いたときは YES を選んでおく

	m_Dim->SetAlpha(0.5f);			// 後ろを半分暗く
	for (Polygon2D* part : m_Parts)
	{
		part->SetAlpha(1.0f);
	}

	UpdateLook();					// ボタンの見た目は選択に合わせて決める
}

//==============================================================================
//閉じる
//==============================================================================
void ExitDialog::Close()
{
	m_IsOpen = false;

	m_Dim->SetAlpha(0.0f);
	for (Polygon2D* part : m_Parts)
	{
		part->SetAlpha(0.0f);
	}
	for (int i = 0; i < BUTTON_MAX; i++)
	{
		m_BtnNormal[i]->SetAlpha(0.0f);
		m_BtnSelected[i]->SetAlpha(0.0f);
		m_BtnText[i]->SetAlpha(0.0f);
	}
}

//==============================================================================
//開いている間の操作
//==============================================================================
ExitDialog::RESULT ExitDialog::Update()
{
	if (!m_IsOpen)
	{
		return RESULT_NONE;		// 閉じているときは何もしない
	}

	//ホバーで選ぶ
	int hover = HitButton(Input::GetMouseX(), Input::GetMouseY());
	if (hover >= 0 && hover != m_Select)
	{
		m_Select = hover;
		UpdateLook();
		m_SelectSE->Play();
	}

	//ボタンの上でクリックしたら決定
	if (Input::GetMouseTrigger(Input::MOUSE_LEFT))
	{
		int hit = HitButton(Input::GetMouseX(), Input::GetMouseY());
		if (hit >= 0)
		{
			m_DecideSE->Play();

			if (hit == BUTTON_YES)
			{
				return RESULT_YES;	// どうするかは使う側が決める
			}

			Close();
			return RESULT_NO;
		}
	}

	//Esc は NO と同じ(閉じる)
	if (Input::GetKeyTrigger(VK_ESCAPE))
	{
		Close();
		return RESULT_NO;
	}

	return RESULT_NONE;
}

//==============================================================================
//ボタンの見た目を選択に合わせる
//==============================================================================
void ExitDialog::UpdateLook()
{
	for (int i = 0; i < BUTTON_MAX; i++)
	{
		bool selected = (i == m_Select);
		m_BtnSelected[i]->SetAlpha(selected ? 1.0f : 0.0f);	// 選んでいる枠だけ見せる
		m_BtnNormal[i]->SetAlpha(selected ? 0.0f : 1.0f);
		m_BtnText[i]->SetColor(selected ? COLOR_SELECTED : COLOR_NORMAL);
	}
}

//==============================================================================
//画面の (x, y) がどのボタンの上にあるか調べる
//==============================================================================
int ExitDialog::HitButton(int x, int y)
{
	for (int i = 0; i < BUTTON_MAX; i++)
	{
		float left = BTN_X[i] + BTN_MARGIN;	// 光の余白を除いた枠の左端
		float top = BTN_Y;
		float right = left + BTN_W;
		float bottom = top + BTN_H;

		if (x >= left && x < right && y >= top && y < bottom)
		{
			return i;
		}
	}

	return -1;
}