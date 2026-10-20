/*==============================================================================

[SelectScene.cpp]
														Author :Watanabe Retsu
														Date   :2026/10/20
--------------------------------------------------------------------------------

==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "Polygon2D.h"
#include "audio.h"
#include "GameSession.h"
#include "SelectScene.h"
#include "GameScene.h"
#include <algorithm>
#include "SongList.h"
#include <cstdio>
#include "Chart.h"
#include <cmath>
#include <cstring>
//==============================================================================
//マクロ宣言
//==============================================================================
#define UI_SCALE   (0.75f)		// UIのスケール倍率
#define FADE_TIME  (0.5f)		// フェードにかける時間(秒)

// 曲リスト(選択中の行の位置。ほかの行はここから LIST_PITCH ずつずらす)
#define LIST_X           (102.0f)	// 曲名・アーティスト名の左端
#define LIST_TITLE_Y     (381.0f)	// 選択中の曲名の上端
#define LIST_ARTIST_Y    (417.0f)	// 選択中のアーティスト名の上端
#define LIST_PITCH       (96.0f)	// 行の間隔
#define LIST_ROWS_ABOVE  (1)		// 選択中より上に見せる行の数
#define LIST_ROWS_BELOW  (2)		// 選択中より下に見せる行の数

#define LIST_CURSOR_X    (72.0f)	// 選択中の帯
#define LIST_CURSOR_Y    (363.0f)
#define LIST_CURSOR_W    (438.0f)
#define LIST_CURSOR_H    (87.0f)

// ジャケット
#define JACKET_X         (630.0f)
#define JACKET_Y         (180.0f)
#define JACKET_SIZE      (450.0f)
#define BRACKET_MARGIN   (15.0f)	// 括弧がジャケットからはみ出す幅(上下左右とも)

// 右の曲の情報
#define INFO_X           (1185.0f)	// 曲名・アーティスト名の左端
#define INFO_TITLE_Y     (183.0f)
#define INFO_ARTIST_Y    (249.0f)
#define INFO_SCALE       (1.35f)	// 右の文字の倍率(リストは UI_SCALE = 0.75)

// BPM / LENGTH / NOTES
#define LABEL_Y          (305.0f)	// 見出しの上端
#define BPM_X            (1185.0f)	// それぞれの左端(見出しと数字で同じ)
#define LENGTH_X         (1295.0f)
#define NOTES_X          (1446.0f)
#define NUMBER_CENTER_Y  (360.0f)	// 数字の中心の y
#define DIGIT_SIZE       (48.0f)	// 1マスの大きさ(アトラスの1マスを正方形で描く)
#define DIGIT_STEP       (26.0f)	// 1文字ぶんの送り幅
#define NARROW_STEP      (14.0f)	// 「.」「:」の送り幅(数字より細いので詰める)
//==============================================================================
//グローバル変数
//==============================================================================
static const XMFLOAT4 COLOR_SELECTED = { 0.949f, 0.941f, 0.980f, 1.0f };	// #F2F0FA 選択中
static const XMFLOAT4 COLOR_NORMAL = { 0.541f, 0.525f, 0.659f, 1.0f };	// #8A86A8 選択していない
static const XMFLOAT4 COLOR_ACCENT = { 0.435f, 0.890f, 1.0f,   1.0f };	// #6FE3FF 強調(水色)
static const XMFLOAT4 COLOR_CURSOR = { 0.486f, 0.424f, 1.0f,   0.25f };	// 帯のうす紫(半透明)

//数字アトラスのマスの番号(左上から 0,1,2 … 右下が 15)
static int AtlasCell(char c)
{
	if (c >= '0' && c <= '9') return c - '0';	// '0'〜'9' → 0〜9
	if (c == '.') return 10;
	if (c == '%') return 11;
	if (c == '+') return 13;
	if (c == '-') return 14;
	return 15;									// ':'
}

//==============================================================================
//初期化処理
//==============================================================================
void SelectScene::Init()
{
	//曲の一覧を読む
	SongList::Load("asset\\chart\\songs.json");

	m_Difficulty = GameSession::difficulty;

	//全曲・全難易度の譜面を読んで、BPM・ノーツ数・長さを計算しておく
	LoadChartInfo();

	//前に選んでいた曲と難易度から始める(曲の数が減っていたら1曲目に)
	m_Select = GameSession::songIndex;
	if (m_Select < 0 || m_Select >= (int)SongList::songs.size())
	{
		m_Select = 0;
	}
	m_Difficulty = GameSession::difficulty;

	//効果音とBGM
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

	//曲リスト
	//選択中の帯と線(行は動かさず、曲のほうを動かす。文字より先に足して、文字の下に描く)
	m_ListCursor = Manager::AddGameObject<Polygon2D>();
	m_ListCursor->Init(LIST_CURSOR_X, LIST_CURSOR_Y, LIST_CURSOR_W, LIST_CURSOR_H, L"asset\\texture\\ui\\assets\\white_square.png");
	m_ListCursor->SetColor(COLOR_CURSOR);
	m_ListCursor->SetGradientAlpha(1.0f, 0.0f);		// 左はそのまま、右端で消える

	m_ListCursorLine = Manager::AddGameObject<Polygon2D>();
	m_ListCursorLine->Init(LIST_CURSOR_X, LIST_CURSOR_Y + 12.0f, 6.0f, LIST_CURSOR_H - 24.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	m_ListCursorLine->SetColor(COLOR_ACCENT);

	//曲名とアーティスト名(全部「選択中の行」に置いておき、UpdateListLook でずらす)
	m_SongTitle.clear();
	m_SongArtist.clear();
	for (const SongInfo& song : SongList::songs)
	{
		Polygon2D* title = Manager::AddGameObject<Polygon2D>();
		title->Init(LIST_X, LIST_TITLE_Y, song.titleWidth * UI_SCALE, song.titleHeight * UI_SCALE, song.titleImage.c_str());
		m_SongTitle.push_back(title);

		Polygon2D* artist = Manager::AddGameObject<Polygon2D>();
		artist->Init(LIST_X, LIST_ARTIST_Y, song.artistWidth * UI_SCALE, song.artistHeight * UI_SCALE, song.artistImage.c_str());
		m_SongArtist.push_back(artist);
	}

	//ジャケットと右の曲の情報(曲ごとに作って、選択中の曲だけ見せる)
	m_Jacket.clear();
	m_InfoTitle.clear();
	m_InfoArtist.clear();
	for (const SongInfo& song : SongList::songs)
	{
		Polygon2D* jacket = Manager::AddGameObject<Polygon2D>();
		jacket->Init(JACKET_X, JACKET_Y, JACKET_SIZE, JACKET_SIZE, song.jacket.c_str());
		m_Jacket.push_back(jacket);

		Polygon2D* title = Manager::AddGameObject<Polygon2D>();
		title->Init(INFO_X, INFO_TITLE_Y, song.titleWidth * INFO_SCALE, song.titleHeight * INFO_SCALE, song.titleImage.c_str());
		title->SetColor(COLOR_SELECTED);
		m_InfoTitle.push_back(title);

		Polygon2D* artist = Manager::AddGameObject<Polygon2D>();
		artist->Init(INFO_X, INFO_ARTIST_Y, song.artistWidth * INFO_SCALE, song.artistHeight * INFO_SCALE, song.artistImage.c_str());
		artist->SetColor(COLOR_NORMAL);
		m_InfoArtist.push_back(artist);
	}

	//ジャケットの四隅の括弧(ジャケットより後に足して、上に重ねる)
	Polygon2D* bracket = Manager::AddGameObject<Polygon2D>();
	bracket->Init(JACKET_X - BRACKET_MARGIN, JACKET_Y - BRACKET_MARGIN,
		JACKET_SIZE + BRACKET_MARGIN * 2.0f, JACKET_SIZE + BRACKET_MARGIN * 2.0f,
		L"asset\\texture\\ui\\assets\\jacket_bracket.png");

	//BPM / LENGTH / NOTES の見出し
	const WCHAR* labelFile[3] = {
		L"asset\\texture\\ui\\text\\label\\bpm.png",
		L"asset\\texture\\ui\\text\\label\\length.png",
		L"asset\\texture\\ui\\text\\label\\notes.png" };
	const float labelX[3] = { BPM_X, LENGTH_X, NOTES_X };
	const float labelW[3] = { 78.0f, 134.0f, 114.0f };
	const float labelH[3] = { 48.0f, 50.0f, 50.0f };

	for (int i = 0; i < 3; i++)
	{
		Polygon2D* label = Manager::AddGameObject<Polygon2D>();
		label->Init(labelX[i], LABEL_Y, labelW[i] * UI_SCALE, labelH[i] * UI_SCALE, labelFile[i]);
		label->SetColor(COLOR_NORMAL);
	}

	//数字(中身は UpdateInfoText で入れる)
	CreateNumberText(m_BpmChar, BPM_X);
	CreateNumberText(m_LengthChar, LENGTH_X);
	CreateNumberText(m_NotesChar, NOTES_X);


	UpdateListLook();

	//終了確認(キーガイドより前に足す。キーガイドだけは暗くしないため)
	m_ExitDialog.Init(m_SelectSE, m_DecideSE);

	//キーガイド(画面下)
	//区切り線
	Polygon2D* guideLine = Manager::AddGameObject<Polygon2D>();
	guideLine->Init(0.0f, 1002.0f, SCREEN_WIDTH, 2.0f, L"asset\\texture\\ui\\assets\\white_square.png");
	guideLine->SetColor(XMFLOAT4(0.541f, 0.525f, 0.659f, 0.3f));

	//キーの画像を左から順に並べる(Wheel・A/D は曲リストと難易度ができたら足す)
	//キーの画像を左から順に並べる(A/D は難易度ができたら足す)
	const int GUIDE_NUM = 3;
	const WCHAR* guideFile[GUIDE_NUM] = {
		L"asset\\texture\\ui\\keyguide\\kg_wheel_song.png",
		L"asset\\texture\\ui\\keyguide\\kg_enter_decide.png",
		L"asset\\texture\\ui\\keyguide\\kg_esc_exit.png" };
	const float guideWidth[GUIDE_NUM] = { 216.0f, 208.0f, 182.0f };

	float guideX = 66.0f;
	float escX = 0.0f;			// 「Esc 終了」を置いた x
	for (int i = 0; i < GUIDE_NUM; i++)
	{
		Polygon2D* guide = Manager::AddGameObject<Polygon2D>();
		guide->Init(guideX, 1005.0f, guideWidth[i] * UI_SCALE, 104.0f * UI_SCALE, guideFile[i]);
		guide->SetColor(COLOR_NORMAL);

		if (i == GUIDE_NUM - 1)	// 最後の1つが「Esc 終了」
		{
			m_GuideEscExit = guide;
			escX = guideX;
		}

		guideX += guideWidth[i] * UI_SCALE + 30.0f;
	}

	//「Esc キャンセル」(終了確認を開いている間だけ「Esc 終了」と入れ替える)
	m_GuideEscCancel = Manager::AddGameObject<Polygon2D>();
	m_GuideEscCancel->Init(escX, 1005.0f, 272.0f * UI_SCALE, 104.0f * UI_SCALE, L"asset\\texture\\ui\\keyguide\\kg_esc_cancel.png");
	m_GuideEscCancel->SetColor(COLOR_NORMAL);

	SetEscGuide(false);

	//フェード用の黒い板(一番最後に足して、全部の上に描く)
	m_Fade = Manager::AddGameObject<Polygon2D>();
	m_Fade->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\ui\\assets\\white_square.png");
	m_Fade->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
	m_FadeTime = 0.0f;
	m_IsLeaving = false;
}

//==============================================================================
//終了処理
//==============================================================================
void SelectScene::Uninit()
{

}

//==============================================================================
//更新処理
//==============================================================================
void SelectScene::Update()
{
	//フェード
	m_FadeTime += Manager::GetDeltaTime();
	float t = std::min(m_FadeTime / FADE_TIME, 1.0f);
	m_Fade->SetAlpha(m_IsLeaving ? t : 1.0f - t);

	if (m_IsLeaving)
	{
		return;		// 出ていく途中は操作を受け付けない
	}

	//終了確認を開いている間は、そちらだけ動かす
	if (m_ExitDialog.IsOpen())
	{
		ExitDialog::RESULT result = m_ExitDialog.Update();

		if (result == ExitDialog::RESULT_YES)
		{
			PostQuitMessage(0);
		}
		else if (result == ExitDialog::RESULT_NO)
		{
			SetEscGuide(false);
		}
		return;
	}

	//曲の選択(ホイール)。端まで行ったら反対の端へ(ループ)
	int songNum = (int)SongList::songs.size();
	int wheel = Input::GetMouseWheel();
	if (wheel != 0 && songNum > 1)
	{
		int dir = (wheel > 0) ? -1 : 1;					// 奥に回すと上の曲、手前に回すと下の曲
		m_Select = (m_Select + dir + songNum) % songNum;	// 0 の上は最後の曲、最後の下は 0
		UpdateListLook();
		m_SelectSE->Play();
	}

	//Enter で決定
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		StartGame();
		return;
	}

	//Esc で終了確認を開く
	if (Input::GetKeyTrigger(VK_ESCAPE))
	{
		m_ExitDialog.Open();
		SetEscGuide(true);
	}
}

//==============================================================================
//描画処理
//==============================================================================
void SelectScene::Draw()
{

}

//==============================================================================
//選んだ譜面でゲームを始める
//==============================================================================
void SelectScene::StartGame()
{
	if (SongList::songs.empty())
	{
		return;		// 曲が読めていないときは何もしない(songs[0] を読むと落ちるので)
	}

	//選んでいる曲と難易度を覚えて、その譜面を GameScene に渡す
	GameSession::songIndex = m_Select;
	GameSession::difficulty = m_Difficulty;
	GameSession::chartPath = SongList::songs[m_Select].chartPath[m_Difficulty];

	m_DecideSE->Play();
	Manager::ChangeScene<GameScene>(FADE_TIME);
	m_IsLeaving = true;
	m_FadeTime = 0.0f;
}

//==============================================================================
//キーガイドの Esc を切り替える
//==============================================================================
void SelectScene::SetEscGuide(bool cancel)
{
	m_GuideEscExit->SetAlpha(cancel ? 0.0f : 1.0f);
	m_GuideEscCancel->SetAlpha(cancel ? 1.0f : 0.0f);
}

//==============================================================================
//song 番目の曲が、選択中の行から何行ずれた所にあるか(上なら マイナス、下なら プラス)
//==============================================================================
int SelectScene::GetSlot(int song) const
{
	int songNum = (int)SongList::songs.size();

	//選択中から下へ何曲目か(0 〜 songNum-1)
	int slot = (song - m_Select + songNum) % songNum;

	//下へ数えて最後のほうの曲は、上にあるものとして扱う(ループなので上下がつながっている)
	if (slot != 0 && slot >= songNum - LIST_ROWS_ABOVE)
	{
		slot -= songNum;
	}

	return slot;
}

//==============================================================================
//曲リストの見た目を選択に合わせる
//==============================================================================
void SelectScene::UpdateListLook()
{
	for (int i = 0; i < (int)SongList::songs.size(); i++)
	{
		int  slot = GetSlot(i);
		bool show = (slot >= -LIST_ROWS_ABOVE && slot <= LIST_ROWS_BELOW);	// 画面に出す行か
		bool selected = (slot == 0);

		//選択中の行から slot 行ぶんずらす
		Vector3 offset(0.0f, LIST_PITCH * slot, 0.0f);
		m_SongTitle[i]->SetPosition(offset);
		m_SongArtist[i]->SetPosition(offset);

		//色(見せない行は透明にする)
		XMFLOAT4 titleColor = selected ? COLOR_SELECTED : COLOR_NORMAL;
		XMFLOAT4 artistColor = COLOR_NORMAL;
		artistColor.w = selected ? 1.0f : 0.5f;			// 選んでいない曲のアーティスト名はもっと控えめに

		if (!show)
		{
			titleColor.w = 0.0f;
			artistColor.w = 0.0f;
		}

		m_SongTitle[i]->SetColor(titleColor);
		m_SongArtist[i]->SetColor(artistColor);

		//ジャケットと右の曲の情報は、選択中の曲だけ見せる
		float infoAlpha = selected ? 1.0f : 0.0f;
		m_Jacket[i]->SetAlpha(infoAlpha);
		m_InfoTitle[i]->SetAlpha(infoAlpha);
		m_InfoArtist[i]->SetAlpha(infoAlpha);
	}
	UpdateInfoText();	// 数字も選択中の曲に合わせる
}

//==============================================================================
//全曲・全難易度の譜面を読んで、表示用の情報を作る
//==============================================================================
void SelectScene::LoadChartInfo()
{
	for (int d = 0; d < DIFFICULTY_MAX; d++)
	{
		m_ChartInfo[d].clear();

		for (const SongInfo& song : SongList::songs)
		{
			ChartInfo info;
			Chart chart;

			if (chart.Load(song.chartPath[d]))
			{
				info.loaded = true;

				//BPM:テンポの変化点の中で一番遅いものと速いもの
				double bpmMin = chart.tempos[0].bpm;
				double bpmMax = chart.tempos[0].bpm;
				for (const TempoEvent& tempo : chart.tempos)
				{
					bpmMin = std::min(bpmMin, tempo.bpm);
					bpmMax = std::max(bpmMax, tempo.bpm);
				}
				info.bpmMin = (int)std::round(bpmMin);
				info.bpmMax = (int)std::round(bpmMax);

				//NOTES
				info.notes = (int)chart.notes.size();

				//LENGTH:一番最後に終わるノーツの終わり(ロングノーツは終点)までの秒数
				int endTick = 0;
				for (const Note& note : chart.notes)
				{
					endTick = std::max(endTick, note.tick + note.length);
				}
				info.lengthSec = (int)std::ceil(chart.TickToSec(endTick));
			}

			m_ChartInfo[d].push_back(info);		// 読めなかった曲も入れる(並びを songs とそろえるため)
		}
	}
}

//==============================================================================
//選択中の曲・難易度の数字を出す
//==============================================================================
void SelectScene::UpdateInfoText()
{
	if (SongList::songs.empty())
	{
		return;
	}

	const ChartInfo& info = m_ChartInfo[m_Difficulty][m_Select];
	char text[16];

	//BPM(途中で変わる曲は「最小-最大」)
	if (!info.loaded)                     strcpy_s(text, "---");
	else if (info.bpmMin == info.bpmMax)  sprintf_s(text, "%d", info.bpmMin);
	else                                  sprintf_s(text, "%d-%d", info.bpmMin, info.bpmMax);
	SetNumberText(m_BpmChar, text, BPM_X);

	//LENGTH(分:秒。秒は2桁)
	if (!info.loaded) strcpy_s(text, "-:--");
	else              sprintf_s(text, "%d:%02d", info.lengthSec / 60, info.lengthSec % 60);
	SetNumberText(m_LengthChar, text, LENGTH_X);

	//NOTES
	if (!info.loaded) strcpy_s(text, "---");
	else              sprintf_s(text, "%d", info.notes);
	SetNumberText(m_NotesChar, text, NOTES_X);
}

//==============================================================================
//数字用の Polygon2D を作る(最初は全部透明)
//==============================================================================
void SelectScene::CreateNumberText(Polygon2D* chars[], float left)
{
	for (int i = 0; i < INFO_CHAR_MAX; i++)
	{
		chars[i] = Manager::AddGameObject<Polygon2D>();
		chars[i]->Init(left, NUMBER_CENTER_Y - DIGIT_SIZE * 0.5f, DIGIT_SIZE, DIGIT_SIZE, L"asset\\texture\\number_atlas_512.png");
		chars[i]->SetAlpha(0.0f);
	}
}

//==============================================================================
//文字を左から順に並べる(使わない分は透明にする)
//==============================================================================
void SelectScene::SetNumberText(Polygon2D* chars[], const char* text, float left)
{
	int   length = (int)strlen(text);
	float x = left;						// 次の文字の左端(左から右へ進む)

	for (int i = 0; i < INFO_CHAR_MAX; i++)
	{
		if (i >= length)
		{
			chars[i]->SetAlpha(0.0f);	// 文字が足りない分は隠す("142" は3文字なので、4つ目からは使わない)
			continue;
		}

		char  c = text[i];
		int   cell = AtlasCell(c);
		float step = (c == '.' || c == ':') ? NARROW_STEP : DIGIT_STEP;	// 「.」「:」だけ細い

		float centerX = x + step * 0.5f;	// この文字の真ん中

		chars[i]->SetRect(centerX - DIGIT_SIZE * 0.5f, NUMBER_CENTER_Y - DIGIT_SIZE * 0.5f, DIGIT_SIZE, DIGIT_SIZE);
		chars[i]->SetUV((cell % 4) * 0.25f, (cell / 4) * 0.25f, 0.25f, 0.25f);
		chars[i]->SetColor(COLOR_SELECTED);

		x += step;							// 次の文字へ
	}
}