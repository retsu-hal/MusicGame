#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "Polygon2D.h"
#include "Audio.h"
#include "TutorialWindow.h"

// ---- 置く場所(1920×1080 の画面で、画像の左上) ----
#define WINDOW_X (340.0f)	// 枠 1240×860 を真ん中に
#define WINDOW_Y (110.0f)
#define WINDOW_W (1240.0f)
#define WINDOW_H (860.0f)

#define PAGE_X (WINDOW_X + 50.0f)	// ページ 1140×600(枠の見出しの下)
#define PAGE_Y (WINDOW_Y + 95.0f)
#define PAGE_W (1140.0f)
#define PAGE_H (600.0f)

#define ARROW_W (80.0f)
#define ARROW_H (110.0f)
#define ARROW_L_X (WINDOW_X - 100.0f)				// 枠の左の外
#define ARROW_R_X (WINDOW_X + WINDOW_W + 20.0f)		// 枠の右の外
#define ARROW_Y (WINDOW_Y + 340.0f)

#define DOT_SIZE (32.0f)
#define DOT_PITCH (36.0f)	// 点の間隔
#define DOT_X ((SCREEN_WIDTH - DOT_PITCH * (PAGE_MAX - 1) - DOT_SIZE) * 0.5f)	// 4つを真ん中にそろえる
#define DOT_Y (WINDOW_Y + 700.0f)

#define BTN_W (300.0f)
#define BTN_H (72.0f)
#define BTN_X ((SCREEN_WIDTH - BTN_W) * 0.5f)	// 810
#define BTN_Y (WINDOW_Y + 755.0f)

#define CLOSE_W (124.0f)	// 「閉じる」の文字
#define CLOSE_H (41.0f)

#define DIM_ALPHA (0.6f)	// 後ろの暗さ(ポーズと同じ)★

void TutorialWindow::Init()
{
	// 効果音(ポーズと同じ音を使う)
	GameObject* sound = Manager::AddGameObject<GameObject>();
	m_SelectSE = sound->AddComponent<Audio>();
	m_SelectSE->Load("asset\\audio\\se\\se_select.wav", Audio::TYPE_SE);
	m_DecideSE = sound->AddComponent<Audio>();
	m_DecideSE->Load("asset\\audio\\se\\se_click.wav", Audio::TYPE_SE);

	// 後ろを暗くする板
	m_Dim = Manager::AddGameObject<Polygon2D>();
	m_Dim->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_Dim->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

	// 枠
	m_Window = Manager::AddGameObject<Polygon2D>();
	m_Window->Init(WINDOW_X, WINDOW_Y, WINDOW_W, WINDOW_H, L"asset\\texture\\02_Play\\Tutorial\\help_window.png");

	// ページ(4枚を同じ場所に重ねておいて、今のページだけ見せる)
	const WCHAR* pageFile[PAGE_MAX] = {
		L"asset\\texture\\02_Play\\Tutorial\\help_page_1.png",
		L"asset\\texture\\02_Play\\Tutorial\\help_page_2.png",
		L"asset\\texture\\02_Play\\Tutorial\\help_page_3.png",
		L"asset\\texture\\02_Play\\Tutorial\\help_page_4.png" };
	for (int i = 0; i < PAGE_MAX; i++)
	{
		m_PageImg[i] = Manager::AddGameObject<Polygon2D>();
		m_PageImg[i]->Init(PAGE_X, PAGE_Y, PAGE_W, PAGE_H, pageFile[i]);
	}

	// 矢印
	m_ArrowL = Manager::AddGameObject<Polygon2D>();
	m_ArrowL->Init(ARROW_L_X, ARROW_Y, ARROW_W, ARROW_H, L"asset\\texture\\02_Play\\Tutorial\\help_arrow_left.png");
	m_ArrowR = Manager::AddGameObject<Polygon2D>();
	m_ArrowR->Init(ARROW_R_X, ARROW_Y, ARROW_W, ARROW_H, L"asset\\texture\\02_Play\\Tutorial\\help_arrow_right.png");

	// ページの点(白抜きを先に足して、光る点を上に描く)
	for (int i = 0; i < PAGE_MAX; i++)
	{
		m_DotOff[i] = Manager::AddGameObject<Polygon2D>();
		m_DotOff[i]->Init(DOT_X + DOT_PITCH * i, DOT_Y, DOT_SIZE, DOT_SIZE, L"asset\\texture\\02_Play\\Tutorial\\help_dot_off.png");
	}
	m_DotOn = Manager::AddGameObject<Polygon2D>();
	m_DotOn->Init(DOT_X, DOT_Y, DOT_SIZE, DOT_SIZE, L"asset\\texture\\02_Play\\Tutorial\\help_dot_on.png");	// 1ページ目の位置

	// 「閉じる」ボタン(板2枚 → 文字の順に足す)
	m_BtnNormal = Manager::AddGameObject<Polygon2D>();
	m_BtnNormal->Init(BTN_X, BTN_Y, BTN_W, BTN_H, L"asset\\texture\\02_Play\\Tutorial\\help_btn_normal.png");
	m_BtnHover = Manager::AddGameObject<Polygon2D>();
	m_BtnHover->Init(BTN_X, BTN_Y, BTN_W, BTN_H, L"asset\\texture\\02_Play\\Tutorial\\help_btn_hover.png");
	m_CloseText = Manager::AddGameObject<Polygon2D>();
	m_CloseText->Init((SCREEN_WIDTH - CLOSE_W) * 0.5f, BTN_Y + (BTN_H - CLOSE_H) * 0.5f, CLOSE_W, CLOSE_H,
		L"asset\\texture\\02_Play\\Tutorial\\help_close.png");

	// 全部レイヤー3(ポーズと同じ。GameScene の最後のほうで足すので、一番上に描かれる)
	for (Polygon2D* p : { m_Dim, m_Window, m_ArrowL, m_ArrowR, m_DotOn, m_BtnNormal, m_BtnHover, m_CloseText })
		p->SetLayer(3);
	for (int i = 0; i < PAGE_MAX; i++)
	{
		m_PageImg[i]->SetLayer(3);
		m_DotOff[i]->SetLayer(3);
	}

	Close();	// 最初は隠しておく
}

void TutorialWindow::Open()
{
	m_Open = true;
	m_Page = 0;
	m_HoverClose = false;

	m_Dim->SetAlpha(DIM_ALPHA);
	m_Window->SetAlpha(1.0f);
	m_CloseText->SetAlpha(1.0f);
	for (Polygon2D* dot : m_DotOff)
		dot->SetAlpha(1.0f);

	UpdateLook();	// ページ・矢印・光る点・ボタンの板は、ここで決める
}

void TutorialWindow::Close()
{
	m_Open = false;

	for (Polygon2D* p : { m_Dim, m_Window, m_ArrowL, m_ArrowR, m_DotOn, m_BtnNormal, m_BtnHover, m_CloseText })
		p->SetAlpha(0.0f);
	for (int i = 0; i < PAGE_MAX; i++)
	{
		m_PageImg[i]->SetAlpha(0.0f);
		m_DotOff[i]->SetAlpha(0.0f);
	}
}

TutorialWindow::RESULT TutorialWindow::Update()
{
	if (!m_Open)
		return RESULT_NONE;

	int x = Input::GetMouseX();
	int y = Input::GetMouseY();

	// 「閉じる」に乗ったら板を明るくする(乗った瞬間だけ音)
	bool hover = Hit(x, y, BTN_X, BTN_Y, BTN_W, BTN_H);
	if (hover != m_HoverClose)
	{
		m_HoverClose = hover;
		if (hover)
			m_SelectSE->Play();
		UpdateLook();
	}

	// ここから下はクリックしたフレームだけ
	if (!Input::GetMouseTrigger(Input::MOUSE_LEFT))
		return RESULT_NONE;

	// 閉じる → 曲スタート
	if (hover)
	{
		m_DecideSE->Play();
		Close();
		return RESULT_CLOSE;
	}

	// ← 前のページ(1ページ目では矢印が出ていないので押せない)
	if (m_Page > 0 && Hit(x, y, ARROW_L_X, ARROW_Y, ARROW_W, ARROW_H))
	{
		m_Page--;
		m_SelectSE->Play();
		UpdateLook();
	}
	// → 次のページ(最後のページでは押せない)
	else if (m_Page < PAGE_MAX - 1 && Hit(x, y, ARROW_R_X, ARROW_Y, ARROW_W, ARROW_H))
	{
		m_Page++;
		m_SelectSE->Play();
		UpdateLook();
	}

	return RESULT_NONE;
}

void TutorialWindow::UpdateLook()
{
	// 今のページだけ見せる
	for (int i = 0; i < PAGE_MAX; i++)
		m_PageImg[i]->SetAlpha(i == m_Page ? 1.0f : 0.0f);

	// 端のページでは、その先の矢印を消す
	m_ArrowL->SetAlpha(m_Page > 0 ? 1.0f : 0.0f);
	m_ArrowR->SetAlpha(m_Page < PAGE_MAX - 1 ? 1.0f : 0.0f);

	// 光る点を今のページの位置へ(Init で置いた 1ページ目からのずらし量)
	m_DotOn->SetPosition(Vector3(DOT_PITCH * m_Page, 0.0f, 0.0f));
	m_DotOn->SetAlpha(1.0f);

	// ボタンの板はどっちか1枚
	m_BtnNormal->SetAlpha(m_HoverClose ? 0.0f : 1.0f);
	m_BtnHover->SetAlpha(m_HoverClose ? 1.0f : 0.0f);
}

bool TutorialWindow::Hit(int x, int y, float left, float top, float width, float height)
{
	return x >= left && x < left + width && y >= top && y < top + height;
}