/*==============================================================================

[SettingsScene.cpp]
														Author :Watanabe Retsu
														Date   :2026/10/16
--------------------------------------------------------------------------------

==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "Polygon2D.h"
#include "Audio.h"
#include "Settings.h"
#include "SettingsScene.h"
#include "TitleScene.h"
#include "Conductor.h"	
#include <algorithm>
#include <cmath>
#include <cstdio>		
#include <cstring>			
//==============================================================================
//マクロ宣言
//==============================================================================
#define UI_SCALE  (0.75f)	// UIのスケール倍率(画像は2倍で作ってある × 画面は 1280→1920 で1.5倍)
#define FADE_TIME (0.5f)	// フェードにかける時間(秒)

// タブ(1つ目のタブの帯の位置。2つ目からは TAB_PITCH ずつ下)
#define TAB_Y        (150.0f)	// 帯の上端
#define TAB_H        (63.0f)	// 帯の高さ
#define TAB_PITCH    (75.0f)	// タブの間隔
#define TAB_CURSOR_X (60.0f)	// 帯の左端
#define TAB_CURSOR_W (330.0f)	// 帯の幅
#define TAB_TEXT_X   (97.0f)	// 文字の画像の左端

// 項目の行(1つ目の行。2つ目からは ROW_H ずつ下)
#define ROW_X        (450.0f)	// 行の左端
#define ROW_Y        (144.0f)	// 行の上端
#define ROW_W        (810.0f)	// 行の幅
#define ROW_H        (114.0f)	// 行の高さ(=行の間隔)
#define ITEM_X       (470.0f)	// 項目名・説明の画像の左端
#define NAME_OFFSET_Y (17.0f)	// 行の上端から項目名の画像の上端まで
#define DESC_OFFSET_Y (51.0f)	// 行の上端から説明の画像の上端まで

// 音量のスライダー(AUDIO タブの1行目。2行目からは ROW_H ずつ下)
#define SLIDER_X        (786.0f)	// 線の左端
#define SLIDER_W        (330.0f)	// 線の長さ(100 のときの長さ)
#define SLIDER_H        (4.0f)		// 線の太さ
#define SLIDER_OFFSET_Y (56.0f)		// 行の上端から、線の真ん中まで
#define KNOB_W          (6.0f)		// つまみの幅
#define KNOB_H          (24.0f)		// つまみの高さ
#define VOLUME_STEP     (1)			// A / D 1回で変わる量
#define SLIDER_HIT_H    (40.0f)		// スライダーをつかめる範囲の高さ(線の上下 20 ずつ)

// 音量の数字(右揃え)
#define NUM_RIGHT  (1230.0f)	// 数字の右端
#define DIGIT_SIZE (48.0f)		// 1マスの大きさ(アトラスの1マスを正方形で描く)
#define DIGIT_STEP (26.0f)		// 1桁ぶんの送り幅

//  GAMEPLAY の値
#define VALUE_RIGHT        (1230.0f)			// 値の右端(選択中の行は ▶ の右端)
#define VALUE_OFFSET_Y     (56.0f)				// 行の上端から、値の真ん中まで(スライダーと同じ高さ)
#define VALUE_IMAGE_H      (76.0f * UI_SCALE)	// ON / OFF / ms の画像の高さ
#define VALUE_IMAGE_CENTER (26.0f)				// 上の画像の上端から、文字の真ん中まで
#define ON_W               (78.0f * UI_SCALE)	// 「ON」の画像の幅
#define OFF_W              (98.0f * UI_SCALE)	// 「OFF」の画像の幅
#define MS_W               (78.0f * UI_SCALE)	// 「ms」の画像の幅
#define DOT_STEP           (14.0f)				// 「.」の送り幅(数字より細いので詰める)
#define ARROW_SIZE         (24.0f)				// ◀ ▶ の大きさ
#define ARROW_GAP          (12.0f)				// 矢印と値のすき間
#define ARROW_HIT_H        (40.0f)				// 矢印をクリックできる範囲の高さ
#define HI_SPEED_STEP      (10.0f)				// HI-SPEED が A / D 1回で変わる量(表示では 0.1)

// スプライトの番号
static int AtlasCell(char c)
{
	if (c >= '0' && c <= '9') return c - '0';	// '0'〜'9' → 0〜9
	if (c == '.') return 10;
	if (c == '+') return 13;
	if (c == '-') return 14;
	return 15;									// それ以外(ここには来ない想定)
}
//==============================================================================
//グローバル変数
//==============================================================================
static const XMFLOAT4 COLOR_SELECTED = { 0.949f, 0.941f, 0.980f, 1.0f };	// ★ #F2F0FA 選択中
static const XMFLOAT4 COLOR_NORMAL = { 0.541f, 0.525f, 0.659f, 1.0f };	// #8A86A8 選択していない
static const XMFLOAT4 COLOR_ACCENT = { 0.435f, 0.890f, 1.0f,   1.0f };	// ★ #6FE3FF 強調(水色)
static const XMFLOAT4 COLOR_CURSOR = { 0.486f, 0.424f, 1.0f,   0.25f };	// ★ 帯のうす紫(半透明)
static const XMFLOAT4 COLOR_SLIDER = { 0.549f, 0.486f, 1.0f, 1.0f };	// ★ #8C7CFF 選択していない行のスライダー(紫)

// ★ AUDIO タブの行と、Settings の音量の対応(上から MASTER / MUSIC / SE)
static int* const VOLUME[SettingsScene::VOLUME_NUM] =
{
	&Settings::masterVolume,
	&Settings::musicVolume,
	&Settings::seVolume,
};

// 項目1つ分の画像(ファイル名と、画像の元の大きさ)
struct ItemImage
{
	const WCHAR* name;			// 項目名の画像
	float        nameWidth;		// 項目名の画像の横幅(高さはどれも 62)
	const WCHAR* desc;			// 説明の画像
	float        descWidth;		// 説明の画像の横幅
	float        descHeight;	// 説明の画像の高さ(48 か 50)
};

// タブごとの項目の画像。書いていないところ(AUDIO の4・5行目)は全部 0 = nullptr になる
static const ItemImage ITEM_IMAGE[SettingsScene::TAB_MAX][SettingsScene::ROW_MAX] =
{
	// GAMEPLAY
	{
		{ L"asset\\texture\\ui\\text\\setting_item\\hi_speed.png",      180.0f, L"asset\\texture\\ui\\text\\jp_desc\\hi_speed.png",      282.0f, 48.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\judge_offset.png",  274.0f, L"asset\\texture\\ui\\text\\jp_desc\\judge_offset.png",  444.0f, 50.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\visual_offset.png", 282.0f, L"asset\\texture\\ui\\text\\jp_desc\\visual_offset.png", 468.0f, 50.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\layer_guide.png",   240.0f, L"asset\\texture\\ui\\text\\jp_desc\\layer_guide.png",   474.0f, 48.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\fast_slow.png",     236.0f, L"asset\\texture\\ui\\text\\jp_desc\\fast_slow.png",     470.0f, 50.0f },
	},
	// AUDIO
	{
		{ L"asset\\texture\\ui\\text\\setting_item\\master.png",        156.0f, L"asset\\texture\\ui\\text\\jp_desc\\master.png",        138.0f, 48.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\music.png",         126.0f, L"asset\\texture\\ui\\text\\jp_desc\\music.png",         138.0f, 50.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\se.png",             60.0f, L"asset\\texture\\ui\\text\\jp_desc\\se.png",            330.0f, 50.0f },
	},
	// DISPLAY
	{
		{ L"asset\\texture\\ui\\text\\setting_item\\screen_mode.png",   264.0f, L"asset\\texture\\ui\\text\\jp_desc\\screen_mode.png",   330.0f, 48.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\resolution.png",    236.0f, L"asset\\texture\\ui\\text\\jp_desc\\resolution.png",    306.0f, 50.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\v_sync.png",        144.0f, L"asset\\texture\\ui\\text\\jp_desc\\v_sync.png",        354.0f, 48.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\bg_brightness.png", 294.0f, L"asset\\texture\\ui\\text\\jp_desc\\bg_brightness.png", 282.0f, 50.0f },
		{ L"asset\\texture\\ui\\text\\setting_item\\show_fps.png",      196.0f, L"asset\\texture\\ui\\text\\jp_desc\\show_fps.png",      330.0f, 48.0f },
	},
};

//==============================================================================
//初期化処理
//==============================================================================
void SettingsScene::Init()
{
	//タブ
	m_Tab = TAB_GAMEPLAY;
	m_HoverTab = -1;	

	//項目の行
	m_Row = 0;
	m_HoverRow = -1;		
	m_DragRow = -1;
	m_DragChanged = false;
	//効果音
	GameObject* sound = Manager::AddGameObject<GameObject>();
	m_SelectSE = sound->AddComponent<Audio>();												
	m_SelectSE->Load("asset\\audio\\se\\se_select.wav", Audio::TYPE_SE);				

	m_DecideSE = sound->AddComponent<Audio>();
	m_DecideSE->Load("asset\\audio\\se\\se_click.wav", Audio::TYPE_SE);

	m_BGM = sound->AddComponent<Audio>();								
	m_BGM->Load("asset\\audio\\bgm\\bgm_title.wav", Audio::TYPE_MUSIC);	
	m_BGM->SetVolume(0.3f);											
	m_BGM->Play(true);													

	//背景
	Manager::AddGameObject<Polygon2D>()->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\TitleBG.png");

	//左上の見出し「SETTINGS」
	Polygon2D* heading = Manager::AddGameObject<Polygon2D>();
	heading->Init(72.0f, 50.0f, 166.0f * UI_SCALE, 48.0f * UI_SCALE, L"asset\\texture\\ui\\text\\label\\settings.png");
	heading->SetColor(COLOR_NORMAL);

	
	//タブと項目の間の縦線
	Polygon2D* divider = Manager::AddGameObject<Polygon2D>();
	divider->Init(393.0f, 144.0f, 2.0f, 810.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	divider->SetColor(XMFLOAT4(0.541f, 0.525f, 0.659f, 0.3f));	// #8A86A8 をうすく

	//タブ
	m_Tab = TAB_GAMEPLAY;

	//選択中の帯と線(文字より先に足して、文字の下に描かれるようにする)
	m_TabCursor = Manager::AddGameObject<Polygon2D>();
	m_TabCursor->Init(TAB_CURSOR_X, TAB_Y, TAB_CURSOR_W, TAB_H, L"asset\\texture\\ui\\assets\\white_square.png");
	m_TabCursor->SetColor(COLOR_CURSOR);
	m_TabCursor->SetGradientAlpha(1.0f, 0.0f);		// 左はそのまま、右端で消える

	m_TabCursorLine = Manager::AddGameObject<Polygon2D>();
	m_TabCursorLine->Init(72.0f, TAB_Y + 15.0f, 6.0f, 33.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	m_TabCursorLine->SetColor(COLOR_ACCENT);

	const WCHAR* tabFile[TAB_MAX] = {
		L"asset\\texture\\ui\\text\\menu\\gameplay.png",
		L"asset\\texture\\ui\\text\\menu\\audio.png",
		L"asset\\texture\\ui\\text\\menu\\display.png" };
	const float tabWidth[TAB_MAX] = { 312.0f, 188.0f, 252.0f };

	for (int i = 0; i < TAB_MAX; i++)
	{
		m_TabText[i] = Manager::AddGameObject<Polygon2D>();
		m_TabText[i]->Init(TAB_TEXT_X, TAB_Y + 4.0f + TAB_PITCH * i, tabWidth[i] * UI_SCALE, 80.0f * UI_SCALE, tabFile[i]);
	}

	//項目の行
	m_Row = 0;

	m_RowCursor = Manager::AddGameObject<Polygon2D>();
	m_RowCursor->Init(ROW_X, ROW_Y, ROW_W, ROW_H, L"asset\\texture\\ui\\assets\\white_square.png");
	m_RowCursor->SetColor(COLOR_CURSOR);
	m_RowCursor->SetGradientAlpha(1.0f, 0.0f);

	m_RowCursorLine = Manager::AddGameObject<Polygon2D>();
	m_RowCursorLine->Init(ROW_X, ROW_Y, 6.0f, ROW_H, L"asset\\texture\\ui\\assets\\white_square.png");
	m_RowCursorLine->SetColor(COLOR_ACCENT);

	//3タブ分の項目を全部作っておく(今のタブ以外は UpdateLook で透明にする)
	for (int t = 0; t < TAB_MAX; t++)
	{
		for (int r = 0; r < ROW_MAX; r++)
		{
			const ItemImage& image = ITEM_IMAGE[t][r];

			if (image.name == nullptr)		// この行には項目が無い
			{
				m_ItemName[t][r] = nullptr;
				m_ItemDesc[t][r] = nullptr;
				continue;
			}

			float y = ROW_Y + ROW_H * r;	// r 行目の上端

			m_ItemName[t][r] = Manager::AddGameObject<Polygon2D>();
			m_ItemName[t][r]->Init(ITEM_X, y + NAME_OFFSET_Y, image.nameWidth * UI_SCALE, 62.0f * UI_SCALE, image.name);

			m_ItemDesc[t][r] = Manager::AddGameObject<Polygon2D>();
			m_ItemDesc[t][r]->Init(ITEM_X, y + DESC_OFFSET_Y, image.descWidth * UI_SCALE, image.descHeight * UI_SCALE, image.desc);
		}
	}

	//行の下の区切り線
	for (int r = 0; r < ROW_MAX; r++)
	{
		m_RowLine[r] = Manager::AddGameObject<Polygon2D>();
		m_RowLine[r]->Init(ROW_X, ROW_Y + ROW_H * (r + 1) - 2.0f, ROW_W, 2.0f, L"asset\\texture\\ui\\assets\\white_square.png");
		m_RowLine[r]->SetColor(COLOR_NORMAL);
	}

	//音量のスライダーと数字(AUDIO タブの3行分。ほかのタブでは UpdateLook で隠す)
	for (int r = 0; r < VOLUME_NUM; r++)
	{
		float y = ROW_Y + ROW_H * r + SLIDER_OFFSET_Y;		// この行の線の真ん中の高さ

		m_SliderBack[r] = Manager::AddGameObject<Polygon2D>();
		m_SliderBack[r]->Init(SLIDER_X, y - SLIDER_H * 0.5f, SLIDER_W, SLIDER_H, L"asset\\texture\\ui\\assets\\white_square.png");

		m_SliderFill[r] = Manager::AddGameObject<Polygon2D>();
		m_SliderFill[r]->Init(SLIDER_X, y - SLIDER_H * 0.5f, SLIDER_W, SLIDER_H, L"asset\\texture\\ui\\assets\\white_square.png");

		m_SliderKnob[r] = Manager::AddGameObject<Polygon2D>();
		m_SliderKnob[r]->Init(SLIDER_X, y - KNOB_H * 0.5f, KNOB_W, KNOB_H, L"asset\\texture\\ui\\assets\\white_square.png");

		for (int d = 0; d < DIGIT_MAX; d++)
		{
			float centerX = NUM_RIGHT - DIGIT_STEP * (d + 0.5f);		// d 桁目の真ん中(右から数える)

			m_VolumeDigit[r][d] = Manager::AddGameObject<Polygon2D>();
			m_VolumeDigit[r][d]->Init(centerX - DIGIT_SIZE * 0.5f, y - DIGIT_SIZE * 0.5f, DIGIT_SIZE, DIGIT_SIZE, L"asset\\texture\\number_atlas_512.png");
		}
	}

	//GAMEPLAY の値(位置と見た目は UpdateLook で決めるので、ここでは作るだけ)
	for (int r = 0; r < ROW_MAX; r++)
	{
		//まず全部 nullptr にしてから、その行で使うものだけ作る
		for (int i = 0; i < VALUE_CHAR_MAX; i++)
		{
			m_ValueChar[r][i] = nullptr;
		}
		m_ValueMs[r] = nullptr;
		m_ValueOn[r] = nullptr;
		m_ValueOff[r] = nullptr;

		//数字の行(HI-SPEED / JUDGE OFFSET / VISUAL OFFSET)
		if (r == GP_HI_SPEED || r == GP_JUDGE_OFFSET || r == GP_VISUAL_OFFSET)
		{
			for (int i = 0; i < VALUE_CHAR_MAX; i++)
			{
				m_ValueChar[r][i] = Manager::AddGameObject<Polygon2D>();
				m_ValueChar[r][i]->Init(0.0f, 0.0f, DIGIT_SIZE, DIGIT_SIZE, L"asset\\texture\\number_atlas_512.png");
			}
		}

		//「ms」が付く行
		if (r == GP_JUDGE_OFFSET || r == GP_VISUAL_OFFSET)
		{
			m_ValueMs[r] = Manager::AddGameObject<Polygon2D>();
			m_ValueMs[r]->Init(0.0f, 0.0f, MS_W, VALUE_IMAGE_H, L"asset\\texture\\ui\\text\\setting_value\\ms.png");
		}

		//ON / OFF の行
		if (r == GP_LAYER_GUIDE || r == GP_FAST_SLOW)
		{
			m_ValueOn[r] = Manager::AddGameObject<Polygon2D>();
			m_ValueOn[r]->Init(0.0f, 0.0f, ON_W, VALUE_IMAGE_H, L"asset\\texture\\ui\\text\\setting_value\\on.png");

			m_ValueOff[r] = Manager::AddGameObject<Polygon2D>();
			m_ValueOff[r]->Init(0.0f, 0.0f, OFF_W, VALUE_IMAGE_H, L"asset\\texture\\ui\\text\\setting_value\\off.png");
		}
	}

	//選択中の行の矢印(1組だけ作って、選択中の行へ動かす)
	m_ArrowLeft = Manager::AddGameObject<Polygon2D>();
	m_ArrowLeft->Init(0.0f, 0.0f, ARROW_SIZE, ARROW_SIZE, L"asset\\texture\\ui\\assets\\arrow_left.png");

	m_ArrowRight = Manager::AddGameObject<Polygon2D>();
	m_ArrowRight->Init(0.0f, 0.0f, ARROW_SIZE, ARROW_SIZE, L"asset\\texture\\ui\\assets\\arrow_right.png");

	m_ArrowLeftX = 0.0f;

	UpdateLook();

	//キーガイド(画面下)
	//区切り線
	Polygon2D* guideLine = Manager::AddGameObject<Polygon2D>();
	guideLine->Init(0.0f, 1002.0f, SCREEN_WIDTH, 2.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	guideLine->SetColor(XMFLOAT4(0.541f, 0.525f, 0.659f, 0.3f));	// #8A86A8 をうすく

	//キーの画像を左から順に並べる
	const int GUIDE_NUM = 3;
	const WCHAR* guideFile[GUIDE_NUM] = {
		L"asset\\texture\\ui\\keyguide\\kg_ad_value.png",
		L"asset\\texture\\ui\\keyguide\\kg_tab_switch.png",
		L"asset\\texture\\ui\\keyguide\\kg_esc_save.png" };
	const float guideWidth[GUIDE_NUM] = { 288.0f, 240.0f, 302.0f };

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
}

//==============================================================================
//終了処理
//==============================================================================
void SettingsScene::Uninit()
{

}

//==============================================================================
//更新処理
//==============================================================================
void SettingsScene::Update()
{
	//フェード
	m_FadeTime += Manager::GetDeltaTime();
	float t = std::min(m_FadeTime / FADE_TIME, 1.0f);	// 0 → 1 に進む割合
	m_Fade->SetAlpha(m_IsLeaving ? t : 1.0f - t);		// 出ていくときは濃く、入ってきたときは薄く

	if (m_IsLeaving)
	{
		return;		// 出ていく途中は操作を受け付けない
	}

	int mouseX = Input::GetMouseX();
	int mouseY = Input::GetMouseY();

	//スライダーのドラッグ(AUDIO タブのとき)
	if (m_Tab == TAB_AUDIO)
	{
		//押した瞬間: スライダーの上ならつかむ
		if (Input::GetMouseTrigger(Input::MOUSE_LEFT))
		{
			int hit = HitSlider(mouseX, mouseY);
			if (hit >= 0)
			{
				m_DragRow = hit;
				m_DragChanged = false;

				if (m_Row != hit)		// つかんだ行を選択中にする
				{
					m_Row = hit;
					UpdateLook();
				}
			}
		}

		//押している間: マウスの x に合わせて値を変える
		if (m_DragRow >= 0 && Input::GetMousePress(Input::MOUSE_LEFT))
		{
			float rate = (mouseX - SLIDER_X) / SLIDER_W;		// 線の左端で 0.0、右端で 1.0(はみ出すとマイナスや 1 より大きくなる)
			int   value = (int)std::round(rate * 100.0f);		// 0〜100 に直す(はみ出した分は ChangeVolume で収める)

			if (ChangeVolume(m_DragRow, value))
			{
				m_DragChanged = true;
			}
		}

		//離したとき: 値が変わっていたら1回だけ音を鳴らす
		if (m_DragRow >= 0 && !Input::GetMousePress(Input::MOUSE_LEFT))
		{
			if (m_DragChanged)
			{
				m_SelectSE->Play();
			}
			m_DragRow = -1;
		}
	}
	//ホバーでの選択は、ドラッグしていないときだけ
	if (m_DragRow < 0)
	{
		//タブの選択(マウスが別のタブに乗った瞬間に切り替える)
		int hoverTab = HitTab(mouseX, mouseY);
		if (hoverTab != m_HoverTab)
		{
			m_HoverTab = hoverTab;

			if (hoverTab >= 0 && hoverTab != m_Tab)
			{
				m_Tab = hoverTab;
				m_Row = 0;				// 新しいタブでは1行目から
				UpdateLook();
				m_SelectSE->Play();
			}
		}

		//行の選択(マウスが別の行に乗った瞬間に選び直す)
		int hoverRow = HitRow(mouseX, mouseY);
		if (hoverRow != m_HoverRow)
		{
			m_HoverRow = hoverRow;

			if (hoverRow >= 0 && hoverRow != m_Row)
			{
				m_Row = hoverRow;
				UpdateLook();
				m_SelectSE->Play();
			}
		}

		if (Input::GetKeyTrigger(VK_TAB))
		{
			m_Tab = (m_Tab + 1) % TAB_MAX;		// 次のタブへ。DISPLAY の次は GAMEPLAY に戻る
			m_Row = 0;							// 新しいタブでは1行目から
			UpdateLook();
			m_SelectSE->Play();
		}

		// ★ 行の選択(ホイール)
		int wheel = Input::GetMouseWheel();
		if (wheel != 0)
		{
			int row = m_Row + (wheel > 0 ? -1 : 1);			// 奥に回すと上の行、手前に回すと下の行
			row = std::clamp(row, 0, GetRowCount() - 1);	// 一番上・一番下で止める

			if (row != m_Row)
			{
				m_Row = row;
				UpdateLook();
				m_SelectSE->Play();
			}
		}

		int dir = 0;
		if (Input::GetKeyTrigger('A')) dir = -1;
		if (Input::GetKeyTrigger('D')) dir = +1;

		if (dir != 0 && m_Tab == TAB_AUDIO)
		{
			int* volume = VOLUME[m_Row];				// 選択中の行の音量の場所
			int  before = *volume;

			*volume = std::clamp(*volume + VOLUME_STEP * dir, 0, 100);

			if (*volume != before)						// 端で止まって変わらなかったときは何もしない
			{
				Audio::ApplyVolume();					// その場で XAudio2 の音量に反映
				UpdateLook();
				m_SelectSE->Play();						// 変えたあとの音量で鳴るので、大きさを確かめられる
			}
		}

		//  GAMEPLAY の値
		if (dir != 0 && m_Tab == TAB_GAMEPLAY)
		{
			if (ChangeGameplay(m_Row, dir))
			{
				m_SelectSE->Play();
			}
		}

		//  GAMEPLAY の矢印をクリック
		if (m_Tab == TAB_GAMEPLAY && Input::GetMouseTrigger(Input::MOUSE_LEFT))
		{
			int arrow = HitArrow(mouseX, mouseY);		// ◀ なら -1、▶ なら +1
			if (arrow != 0 && ChangeGameplay(m_Row, arrow))
			{
				m_SelectSE->Play();
			}
		}
	}
	//Esc で保存してタイトルへ戻る(キーガイドの「Esc 保存して戻る」)
	if (Input::GetKeyTrigger(VK_ESCAPE))
	{
		Settings::Save();
		m_DecideSE->Play();

		Manager::ChangeScene<TitleScene>(FADE_TIME);
		m_IsLeaving = true;
		m_FadeTime = 0.0f;
	}
}

//==============================================================================
//描画処理
//==============================================================================
void SettingsScene::Draw()
{

}

//==============================================================================
//選択に合わせて見た目を変える
//==============================================================================
void SettingsScene::UpdateLook()
{
	//タブの色
	for (int t = 0; t < TAB_MAX; t++)
	{
		m_TabText[t]->SetColor(t == m_Tab ? COLOR_SELECTED : COLOR_NORMAL);
	}

	//タブの帯と線を選んでいるタブへ動かす(Init で置いた1つ目からのずらし量)
	Vector3 tabOffset(0.0f, TAB_PITCH * m_Tab, 0.0f);
	m_TabCursor->SetPosition(tabOffset);
	m_TabCursorLine->SetPosition(tabOffset);

	//項目(今のタブのものだけ見せる)
	for (int t = 0; t < TAB_MAX; t++)
	{
		for (int r = 0; r < ROW_MAX; r++)
		{
			if (m_ItemName[t][r] == nullptr)
			{
				continue;		// 項目が無いところ
			}

			bool show = (t == m_Tab);				// 今のタブの項目か
			bool selected = show && (r == m_Row);	// 選択中の行か

			XMFLOAT4 nameColor = selected ? COLOR_SELECTED : COLOR_NORMAL;
			XMFLOAT4 descColor = selected ? COLOR_ACCENT : COLOR_NORMAL;	// 選択中の説明は水色(モックアップどおり)
			nameColor.w = show ? 1.0f : 0.0f;		// 今のタブ以外は透明にして隠す
			descColor.w = show ? 1.0f : 0.0f;

			m_ItemName[t][r]->SetColor(nameColor);
			m_ItemDesc[t][r]->SetColor(descColor);
		}
	}

	//区切り線は、項目がある行の分だけ見せる
	int count = GetRowCount();
	for (int r = 0; r < ROW_MAX; r++)
	{
		m_RowLine[r]->SetAlpha(r < count ? 0.3f : 0.0f);
	}

	//行の帯と線を選んでいる行へ動かす
	Vector3 rowOffset(0.0f, ROW_H * m_Row, 0.0f);
	m_RowCursor->SetPosition(rowOffset);
	m_RowCursorLine->SetPosition(rowOffset);

	//音量のスライダーと数字(AUDIO タブのときだけ見せる)
	for (int r = 0; r < VOLUME_NUM; r++)
	{
		bool show = (m_Tab == TAB_AUDIO);
		bool selected = show && (r == m_Row);
		float alpha = show ? 1.0f : 0.0f;

		int   value = *VOLUME[r];			// この行の音量(0〜100)
		float rate = value / 100.0f;		// 0.0〜1.0 に直す
		float y = ROW_Y + ROW_H * r + SLIDER_OFFSET_Y;

		//うすい線(全体)
		XMFLOAT4 backColor = COLOR_NORMAL;
		backColor.w = show ? 0.25f : 0.0f;
		m_SliderBack[r]->SetColor(backColor);

		//色付きの線(左端から値のところまで)
		m_SliderFill[r]->SetRect(SLIDER_X, y - SLIDER_H * 0.5f, SLIDER_W * rate, SLIDER_H);
		XMFLOAT4 fillColor = selected ? COLOR_ACCENT : COLOR_SLIDER;	// 選択中は水色、ほかは紫
		fillColor.w = alpha;
		m_SliderFill[r]->SetColor(fillColor);

		//つまみ(値のところに置く)
		m_SliderKnob[r]->SetRect(SLIDER_X + SLIDER_W * rate - KNOB_W * 0.5f, y - KNOB_H * 0.5f, KNOB_W, KNOB_H);
		XMFLOAT4 knobColor = COLOR_SELECTED;
		knobColor.w = alpha;
		m_SliderKnob[r]->SetColor(knobColor);

		//数字(右揃え。上の桁の 0 は出さない)
		int place = 1;		// 今の桁の重み(1, 10, 100)
		for (int d = 0; d < DIGIT_MAX; d++)
		{
			int num = (value / place) % 10;		// d 桁目の数字

			//アトラスの num 番目のマスを貼る(4×4 マス。左上から 0,1,2,3 / 4,5,6,7 …)
			m_VolumeDigit[r][d]->SetUV((num % 4) * 0.25f, (num / 4) * 0.25f, 0.25f, 0.25f);

			bool visible = (d == 0) || (value >= place);	// 一の位はいつも出す。それより上は、その桁まで数があるときだけ
			XMFLOAT4 numColor = selected ? COLOR_SELECTED : COLOR_NORMAL;
			numColor.w = (show && visible) ? 1.0f : 0.0f;
			m_VolumeDigit[r][d]->SetColor(numColor);

			place *= 10;
		}
	}

	//GAMEPLAY の値(GAMEPLAY タブのときだけ見せる)
	bool showGameplay = (m_Tab == TAB_GAMEPLAY);

	m_ArrowLeft->SetAlpha(0.0f);		// 矢印はいったん隠し、選択中の行があればそこに出す
	m_ArrowRight->SetAlpha(0.0f);

	for (int r = 0; r < ROW_MAX; r++)
	{
		bool  selected = showGameplay && (r == m_Row);
		float centerY = ROW_Y + ROW_H * r + VALUE_OFFSET_Y;						// この行の値の真ん中の高さ
		float right = selected ? VALUE_RIGHT - ARROW_SIZE - ARROW_GAP : VALUE_RIGHT;	// 選択中は ▶ の分だけ内側
		float left = right;													// 値の左端(◀ を置く位置に使う)

		XMFLOAT4 color = selected ? COLOR_SELECTED : COLOR_NORMAL;
		color.w = showGameplay ? 1.0f : 0.0f;

		char text[8] = "";		// 値を文字にしたもの

		switch (r)
		{
			case GP_HI_SPEED:
				snprintf(text, sizeof(text), "%.1f", Settings::noteSpeed / 100.0f);	// 800 → "8.0"
				left = SetValueText(r, text, right, centerY, color);
				break;

			case GP_JUDGE_OFFSET:
			case GP_VISUAL_OFFSET:
				{
					float ms = (r == GP_JUDGE_OFFSET) ? Settings::offsetMs : Settings::visualOffsetMs;
					snprintf(text, sizeof(text), "%+d", (int)std::lround(ms));			// 0 → "+0"、-15 → "-15"

					float msLeft = right - MS_W;										// 「ms」を一番右に置く
					m_ValueMs[r]->SetRect(msLeft, centerY - VALUE_IMAGE_CENTER, MS_W, VALUE_IMAGE_H);
					m_ValueMs[r]->SetColor(color);

					left = SetValueText(r, text, msLeft, centerY, color);				// 数字は「ms」の左に
					break;
				}

			case GP_LAYER_GUIDE:
			case GP_FAST_SLOW:
				{
					bool on = (r == GP_LAYER_GUIDE) ? Settings::layerGuide : Settings::fastSlow;

					Polygon2D* show = on ? m_ValueOn[r] : m_ValueOff[r];		// 出す方
					Polygon2D* hide = on ? m_ValueOff[r] : m_ValueOn[r];		// 隠す方
					float width = on ? ON_W : OFF_W;

					left = right - width;
					show->SetRect(left, centerY - VALUE_IMAGE_CENTER, width, VALUE_IMAGE_H);
					show->SetColor(color);
					hide->SetAlpha(0.0f);
					break;
				}
		}

		//選択中の行なら、値の左右に矢印を置く
		if (selected)
		{
			m_ArrowLeftX = left - ARROW_GAP - ARROW_SIZE;
			m_ArrowLeft->SetRect(m_ArrowLeftX, centerY - ARROW_SIZE * 0.5f, ARROW_SIZE, ARROW_SIZE);
			m_ArrowLeft->SetColor(COLOR_ACCENT);

			m_ArrowRight->SetRect(VALUE_RIGHT - ARROW_SIZE, centerY - ARROW_SIZE * 0.5f, ARROW_SIZE, ARROW_SIZE);
			m_ArrowRight->SetColor(COLOR_ACCENT);
		}
	}
}

//==============================================================================
//今のタブの項目の数
//==============================================================================
int SettingsScene::GetRowCount() const
{
	int count = 0;
	for (int r = 0; r < ROW_MAX; r++)
	{
		if (ITEM_IMAGE[m_Tab][r].name != nullptr)
		{
			count++;
		}
	}
	return count;
}

//==============================================================================
//画面の (x, y) がどのタブの上にあるか調べる
//==============================================================================
int SettingsScene::HitTab(int x, int y)
{
	for (int i = 0; i < TAB_MAX; i++)
	{
		float left = TAB_CURSOR_X;
		float top = TAB_Y + TAB_PITCH * i;	// i 番目のタブの帯の上端
		float right = left + TAB_CURSOR_W;
		float bottom = top + TAB_H;

		if (x >= left && x < right && y >= top && y < bottom)
		{
			return i;	// i 番目のタブに当たった
		}
	}

	return -1;			// どれにも当たらなかった
}

//==============================================================================
//画面の (x, y) がどの行の上にあるか調べる
//==============================================================================
int SettingsScene::HitRow(int x, int y)
{
	for (int i = 0; i < GetRowCount(); i++)
	{
		float left = ROW_X;
		float top = ROW_Y + ROW_H * i;		// i 行目の上端
		float right = left + ROW_W;
		float bottom = top + ROW_H;

		if (x >= left && x < right && y >= top && y < bottom)
		{
			return i;	// i 行目に当たった
		}
	}

	return -1;			// どれにも当たらなかった
}

//==============================================================================
//row 行目の音量を value にする(0〜100 に収める)。変わったら true
//==============================================================================
bool SettingsScene::ChangeVolume(int row, int value)
{
	value = std::clamp(value, 0, 100);

	if (value == *VOLUME[row])
	{
		return false;			// 同じ値なら何もしない(端で止まったとき、ドラッグでマウスが動いていないとき)
	}

	*VOLUME[row] = value;		// Settings の音量を直接書き換える
	Audio::ApplyVolume();		// その場で XAudio2 の音量に反映
	UpdateLook();				// 線・つまみ・数字を新しい値に合わせる
	return true;
}

//==============================================================================
//画面の (x, y) がどのスライダーの上にあるか調べる
//==============================================================================
int SettingsScene::HitSlider(int x, int y)
{
	for (int r = 0; r < VOLUME_NUM; r++)
	{
		float centerY = ROW_Y + ROW_H * r + SLIDER_OFFSET_Y;	// r 行目の線の真ん中の高さ

		float left = SLIDER_X - KNOB_W;					// つまみが端にあっても押せるように、少し外まで
		float top = centerY - SLIDER_HIT_H * 0.5f;
		float right = SLIDER_X + SLIDER_W + KNOB_W;
		float bottom = centerY + SLIDER_HIT_H * 0.5f;

		if (x >= left && x < right && y >= top && y < bottom)
		{
			return r;	// r 行目のスライダーに当たった
		}
	}

	return -1;			// どれにも当たらなかった
}

//==============================================================================
//GAMEPLAY の row 行目の値を dir(-1 / +1)の向きに変える。変わったら true
//==============================================================================
bool SettingsScene::ChangeGameplay(int row, int dir)
{
	switch (row)
	{
		case GP_HI_SPEED:
			{
				float before = Settings::noteSpeed;
				Settings::noteSpeed = std::clamp(Settings::noteSpeed + HI_SPEED_STEP * dir, 200.0f, 2000.0f);	// 表示では 2.0〜20.0
				if (Settings::noteSpeed == before) return false;		// 端で止まった
				break;
			}

		case GP_JUDGE_OFFSET:
			{
				float before = Settings::offsetMs;
				Settings::offsetMs = std::clamp(Settings::offsetMs + dir, -200.0f, 200.0f);
				if (Settings::offsetMs == before) return false;
				Conductor::SetOffset(Settings::offsetMs / 1000.0);	// その場で反映(ミリ秒 → 秒)
				break;
			}

		case GP_VISUAL_OFFSET:
			{
				float before = Settings::visualOffsetMs;
				Settings::visualOffsetMs = std::clamp(Settings::visualOffsetMs + dir, -200.0f, 200.0f);
				if (Settings::visualOffsetMs == before) return false;
				break;												// まだゲームでは使っていないので、保存されるだけ
			}

		case GP_LAYER_GUIDE:
			Settings::layerGuide = !Settings::layerGuide;		// A でも D でも ON ⇔ OFF
			break;

		case GP_FAST_SLOW:
			Settings::fastSlow = !Settings::fastSlow;
			break;

		default:
			return false;
	}

	UpdateLook();
	return true;
}

//==============================================================================
//画面の (x, y) が選択中の行の矢印の上か調べる(◀: -1、▶: +1、どちらでもない: 0)
//==============================================================================
int SettingsScene::HitArrow(int x, int y)
{
	float centerY = ROW_Y + ROW_H * m_Row + VALUE_OFFSET_Y;
	float top = centerY - ARROW_HIT_H * 0.5f;
	float bottom = centerY + ARROW_HIT_H * 0.5f;

	if (y < top || y >= bottom)
	{
		return 0;		// 高さが合っていない
	}

	//矢印は小さいので、左右に ARROW_GAP ずつ広げて押しやすくする
	if (x >= m_ArrowLeftX - ARROW_GAP && x < m_ArrowLeftX + ARROW_SIZE + ARROW_GAP)
	{
		return -1;		// ◀
	}

	float rightX = VALUE_RIGHT - ARROW_SIZE;
	if (x >= rightX - ARROW_GAP && x < rightX + ARROW_SIZE + ARROW_GAP)
	{
		return +1;		// ▶
	}

	return 0;
}

//==============================================================================
//row 行目の数字の部品に text を右揃えで並べる。並べ終わった左端の x を返す
//==============================================================================
float SettingsScene::SetValueText(int row, const char* text, float right, float centerY, const XMFLOAT4& color)
{
	int   length = (int)strlen(text);	// 文字数
	float x = right;				// 次の文字の右端(右から左へ進む)

	for (int i = 0; i < VALUE_CHAR_MAX; i++)
	{
		Polygon2D* charPolygon = m_ValueChar[row][i];
		int index = length - 1 - i;		// 右から i 番目の文字が text の何文字目か

		if (index < 0)
		{
			charPolygon->SetAlpha(0.0f);	// 文字が足りない分は隠す("8.0" は3文字なので、4つ目は使わない)
			continue;
		}

		char  c = text[index];
		int   cell = AtlasCell(c);
		float step = (c == '.') ? DOT_STEP : DIGIT_STEP;	// 「.」だけ細い

		x -= step;											// この文字の左端
		float centerX = x + step * 0.5f;					// この文字の真ん中

		charPolygon->SetRect(centerX - DIGIT_SIZE * 0.5f, centerY - DIGIT_SIZE * 0.5f, DIGIT_SIZE, DIGIT_SIZE);
		charPolygon->SetUV((cell % 4) * 0.25f, (cell / 4) * 0.25f, 0.25f, 0.25f);
		charPolygon->SetColor(color);
	}

	return x;		// 一番左の文字の左端
}