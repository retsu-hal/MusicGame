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
#include "SongList.h"
#include "SettingsScene.h"
#include "Chart.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>
//==============================================================================
//マクロ宣言
//==============================================================================
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
#define INFO_SCALE       (1.8f)	// 右の文字の倍率(リストは等倍)

// BPM / LENGTH / NOTES
#define LABEL_Y          (305.0f)	// 見出しの上端
#define BPM_X            (1185.0f)	// それぞれの左端(見出しと数字で同じ)
#define LENGTH_X         (1295.0f)
#define NOTES_X          (1446.0f)
#define NUMBER_CENTER_Y  (360.0f)	// 数字の中心の y
#define DIGIT_SIZE       (48.0f)	// 1マスの大きさ(アトラスの1マスを正方形で描く)
#define DIGIT_STEP       (26.0f)	// 1文字ぶんの送り幅
#define NARROW_STEP      (14.0f)	// 「.」「:」の送り幅(数字より細いので詰める)

// 難易度
#define DIFF_LABEL_Y     (418.0f)	// DIFFICULTY の見出しの上端
#define DIFF_Y           (459.0f)	// 枠の上端
#define DIFF_W           (225.0f)	// 枠の大きさ
#define DIFF_H           (83.0f)
#define DIFF_PITCH       (246.0f)	// EASY の枠の左端から HARD の枠の左端まで
#define DIFF_TEXT_MARGIN (27.0f)	// 枠の左端から文字の左端まで
#define DIFF_TEXT_SCALE  (0.8f)		// 枠の中の文字の倍率

// 歯車(右上)
#define GEAR_X           (1791.0f)
#define GEAR_Y           (27.0f)
#define GEAR_SIZE        (72.0f)

// チュートリアル用の PLAY
#define PLAY_W           (DIFF_PITCH + DIFF_W)	// PLAY の枠の幅(EASY の左端から HARD の右端まで = 471)

// プレビュー
#define PREVIEW_LENGTH   (15.0f)	// 1回に流す長さ(秒)。過ぎたら頭に戻る
#define PREVIEW_VOLUME   (0.3f)
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
	SongList::Load("asset\\chart\\00_SongList.json");

	m_Difficulty = GameSession::difficulty;

	//前に選んでいた曲と難易度から始める(曲の数が減っていたら1曲目に)
	m_Select = GameSession::songIndex;
	if (m_Select < 0 || m_Select >= (int)SongList::songs.size())
	{
		m_Select = 0;
	}

	//全曲・全難易度の譜面を読んで、BPM・ノーツ数・長さを計算しておく
	LoadChartInfo();

	//効果音とプレビュー
	GameObject* sound = Manager::AddGameObject<GameObject>();
	m_SelectSE = sound->AddComponent<Audio>();
	m_SelectSE->Load("asset\\audio\\se\\se_select.wav", Audio::TYPE_SE);

	m_DecideSE = sound->AddComponent<Audio>();
	m_DecideSE->Load("asset\\audio\\se\\se_click.wav", Audio::TYPE_SE);

	//プレビューの音(曲ごとに1つ作っておく)
	m_Preview.clear();
	for (int i = 0; i < (int)SongList::songs.size(); i++)
	{
		const SongInfo& song = SongList::songs[i];

		//"preview" があればそれ、無ければ EASY の譜面に書いてある曲
		std::string path = song.previewAudio;
		if (path.empty())
		{
			path = m_ChartInfo[DIFFICULTY_EASY][i].audioPath;
		}

		if (path.empty())
		{
			m_Preview.push_back(nullptr);		// 音が無い曲(譜面が読めなかったとき)。並びをそろえるために nullptr を入れる
			continue;
		}

		Audio* preview = sound->AddComponent<Audio>();
		preview->Load(path.c_str(), Audio::TYPE_MUSIC);
		preview->SetVolume(PREVIEW_VOLUME);
		m_Preview.push_back(preview);
	}

	//背景
	Manager::AddGameObject<Polygon2D>()->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\00_Common\\TitleBG.png");

	//曲リスト
	//選択中の帯と線(行は動かさず、曲のほうを動かす。文字より先に足して、文字の下に描く)
	m_ListCursor = Manager::AddGameObject<Polygon2D>();
	m_ListCursor->Init(LIST_CURSOR_X, LIST_CURSOR_Y, LIST_CURSOR_W, LIST_CURSOR_H, L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_ListCursor->SetColor(COLOR_CURSOR);
	m_ListCursor->SetGradientAlpha(1.0f, 0.0f);		// 左はそのまま、右端で消える

	m_ListCursorLine = Manager::AddGameObject<Polygon2D>();
	m_ListCursorLine->Init(LIST_CURSOR_X, LIST_CURSOR_Y + 12.0f, 6.0f, LIST_CURSOR_H - 24.0f, L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_ListCursorLine->SetColor(COLOR_ACCENT);

	//アーティスト名が無い曲(チュートリアルなど)に使う画像。大きさ 0 で置くので見えない
	const WCHAR* EMPTY_IMAGE = L"asset\\texture\\00_Common\\icon\\white_square.png";

	//曲名とアーティスト名(全部「選択中の行」に置いておき、UpdateListLook でずらす)
	m_SongTitle.clear();
	m_SongArtist.clear();
	for (const SongInfo& song : SongList::songs)
	{
		Polygon2D* title = Manager::AddGameObject<Polygon2D>();
		title->Init(LIST_X, LIST_TITLE_Y, song.titleWidth, song.titleHeight, song.titleImage.c_str());
		m_SongTitle.push_back(title);

		Polygon2D* artist = Manager::AddGameObject<Polygon2D>();
		const WCHAR* artistFile = song.artistImage.empty() ? EMPTY_IMAGE : song.artistImage.c_str();
		artist->Init(LIST_X, LIST_ARTIST_Y, song.artistWidth, song.artistHeight, artistFile);
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
		const WCHAR* artistFile = song.artistImage.empty() ? EMPTY_IMAGE : song.artistImage.c_str();
		artist->Init(INFO_X, INFO_ARTIST_Y, song.artistWidth * INFO_SCALE, song.artistHeight * INFO_SCALE, artistFile);
		artist->SetColor(COLOR_NORMAL);
		m_InfoArtist.push_back(artist);
	}

	//ジャケットの四隅の括弧(ジャケットより後に足して、上に重ねる)
	Polygon2D* bracket = Manager::AddGameObject<Polygon2D>();
	bracket->Init(JACKET_X - BRACKET_MARGIN, JACKET_Y - BRACKET_MARGIN,
		JACKET_SIZE + BRACKET_MARGIN * 2.0f, JACKET_SIZE + BRACKET_MARGIN * 2.0f,
		L"asset\\texture\\02_Play\\icon\\jacket_bracket.png");

	//BPM / LENGTH / NOTES の見出し
	const WCHAR* labelFile[3] = {
		L"asset\\texture\\02_Play\\text\\label_bpm.png",
		L"asset\\texture\\02_Play\\text\\label_length.png",
		L"asset\\texture\\02_Play\\text\\label_notes.png" };
	const float labelX[3] = { BPM_X, LENGTH_X, NOTES_X };
	const float labelW[3] = { 59.0f, 101.0f, 86.0f };
	const float labelH[3] = { 36.0f, 38.0f, 38.0f };

	for (int i = 0; i < 3; i++)
	{
		Polygon2D* label = Manager::AddGameObject<Polygon2D>();
		label->Init(labelX[i], LABEL_Y, labelW[i], labelH[i], labelFile[i]);
		label->SetColor(COLOR_NORMAL);
	}

	//数字(中身は UpdateInfoText で入れる)
	CreateNumberText(m_BpmChar, BPM_X);
	CreateNumberText(m_LengthChar, LENGTH_X);
	CreateNumberText(m_NotesChar, NOTES_X);

	//難易度
//見出し「DIFFICULTY」
	m_DiffLabel = Manager::AddGameObject<Polygon2D>();
	m_DiffLabel->Init(BPM_X, DIFF_LABEL_Y, 144.0f, 36.0f, L"asset\\texture\\02_Play\\text\\label_difficulty.png");
	m_DiffLabel->SetColor(COLOR_NORMAL);

	//枠と文字(左から EASY / HARD)
	const WCHAR* diffFile[DIFFICULTY_MAX] = {
		L"asset\\texture\\02_Play\\text\\easy.png",
		L"asset\\texture\\02_Play\\text\\hard.png" };
	const float diffTextW[DIFFICULTY_MAX] = { 117.0f, 126.0f };
	const float diffTextH[DIFFICULTY_MAX] = { 60.0f, 62.0f };

	for (int d = 0; d < DIFFICULTY_MAX; d++)
	{
		float x = BPM_X + DIFF_PITCH * d;		// d 番目の枠の左端

		m_DiffNormal[d] = Manager::AddGameObject<Polygon2D>();
		m_DiffNormal[d]->Init(x, DIFF_Y, DIFF_W, DIFF_H, L"asset\\texture\\00_Common\\icon\\btn_normal.png");

		m_DiffSelected[d] = Manager::AddGameObject<Polygon2D>();
		m_DiffSelected[d]->Init(x, DIFF_Y, DIFF_W, DIFF_H, L"asset\\texture\\00_Common\\icon\\btn_selected.png");

		//文字は枠の縦の真ん中にそろえる
		float textH = diffTextH[d] * DIFF_TEXT_SCALE;
		float textY = DIFF_Y + (DIFF_H - textH) * 0.5f;

		m_DiffText[d] = Manager::AddGameObject<Polygon2D>();
		m_DiffText[d]->Init(x + DIFF_TEXT_MARGIN, textY, diffTextW[d] * DIFF_TEXT_SCALE, textH, diffFile[d]);
	}

	//チュートリアル用の PLAY(EASY と HARD の2つ分の幅。文字は真ん中)
	m_PlayFrame = Manager::AddGameObject<Polygon2D>();
	m_PlayFrame->Init(BPM_X, DIFF_Y, PLAY_W, DIFF_H, L"asset\\texture\\00_Common\\icon\\btn_selected.png");

	float playTextW = 115.0f * DIFF_TEXT_SCALE;
	float playTextH = 60.0f * DIFF_TEXT_SCALE;
	m_PlayText = Manager::AddGameObject<Polygon2D>();
	m_PlayText->Init(BPM_X + (PLAY_W - playTextW) * 0.5f, DIFF_Y + (DIFF_H - playTextH) * 0.5f,
		playTextW, playTextH, L"asset\\texture\\02_Play\\text\\play.png");
	m_PlayText->SetColor(COLOR_SELECTED);

	//歯車(右上。終了確認より前に足して、終了確認を開いたときに一緒に暗くなるようにする)
	m_Gear = Manager::AddGameObject<Polygon2D>();
	m_Gear->Init(GEAR_X, GEAR_Y, GEAR_SIZE, GEAR_SIZE, L"asset\\texture\\02_Play\\icon\\gear.png");
	m_Gear->SetColor(COLOR_NORMAL);

	UpdateListLook();		// 中で UpdateDifficultyLook も呼ぶ

	//終了確認(キーガイドより前に足す。キーガイドだけは暗くしないため)
	m_ExitDialog.Init(m_SelectSE, m_DecideSE);

	//キーガイド(画面下)
	//区切り線
	Polygon2D* guideLine = Manager::AddGameObject<Polygon2D>();
	guideLine->Init(0.0f, 1002.0f, SCREEN_WIDTH, 2.0f, L"asset\\texture\\00_Common\\icon\\white_square.png");
	guideLine->SetColor(XMFLOAT4(0.541f, 0.525f, 0.659f, 0.3f));

	//キーの画像を左から順に並べる
	const int GUIDE_NUM = 4;
	const WCHAR* guideFile[GUIDE_NUM] = {
		L"asset\\texture\\02_Play\\keyguide\\kg_wheel_song.png",
		L"asset\\texture\\02_Play\\keyguide\\kg_ad_difficulty.png",
		L"asset\\texture\\00_Common\\keyguide\\kg_click_decide.png",		
		L"asset\\texture\\00_Common\\keyguide\\kg_esc_exit.png" };
	const float guideWidth[GUIDE_NUM] = { 162.0f, 194.0f, 150.0f, 137.0f };	
	float guideX = 66.0f;
	float escX = 0.0f;			// 「Esc 終了」を置いた x
	for (int i = 0; i < GUIDE_NUM; i++)
	{
		Polygon2D* guide = Manager::AddGameObject<Polygon2D>();
		guide->Init(guideX, 1005.0f, guideWidth[i], 78.0f, guideFile[i]);
		guide->SetColor(COLOR_NORMAL);

		if (i == GUIDE_NUM - 1)	// 最後の1つが「Esc 終了」
		{
			m_GuideEscExit = guide;
			escX = guideX;
		}

		guideX += guideWidth[i] + 30.0f;
	}

	//「Esc キャンセル」(終了確認を開いている間だけ「Esc 終了」と入れ替える)
	m_GuideEscCancel = Manager::AddGameObject<Polygon2D>();
	m_GuideEscCancel->Init(escX, 1005.0f, 204.0f, 78.0f, L"asset\\texture\\00_Common\\keyguide\\kg_esc_cancel.png");
	m_GuideEscCancel->SetColor(COLOR_NORMAL);

	SetEscGuide(false);

	//フェード用の黒い板(一番最後に足して、全部の上に描く)
	m_Fade = Manager::AddGameObject<Polygon2D>();
	m_Fade->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_Fade->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
	m_FadeTime = 0.0f;
	m_IsLeaving = false;

	PlayPreview();		// 最初に選ばれている曲を流す
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

	//プレビュー(PREVIEW_LENGTH 秒たったら頭に戻す。終了確認中も戻すので、終了確認の return より前)
	m_PreviewTime += Manager::GetDeltaTime();
	if (m_PreviewTime >= PREVIEW_LENGTH)
	{
		PlayPreview();
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
		PlayPreview();
		m_SelectSE->Play();
	}

	//難易度の選択(A で左、D で右。端で止まる)
	int diffDir = 0;
	if (Input::GetKeyTrigger('A')) diffDir = -1;
	if (Input::GetKeyTrigger('D')) diffDir = +1;

	if (diffDir != 0 && !IsTutorial())		// チュートリアルは難易度を選ばない
	{
		int difficulty = std::clamp(m_Difficulty + diffDir, 0, DIFFICULTY_MAX - 1);

		if (difficulty != m_Difficulty)		// 端でさらに押したときは何もしない(SE も鳴らさない)
		{
			m_Difficulty = difficulty;
			UpdateDifficultyLook();
			UpdateInfoText();				// 数字をこの難易度の譜面のものに
			m_SelectSE->Play();
		}
	}
	//マウスの位置
	int mouseX = Input::GetMouseX();
	int mouseY = Input::GetMouseY();

	//難易度の選択(マウスを動かしたときだけ、乗っている枠を選ぶ)
	bool mouseMoved = (Input::GetMouseMoveX() != 0 || Input::GetMouseMoveY() != 0);
	int  hoverDiff = HitDifficulty(mouseX, mouseY);
	if (mouseMoved && !IsTutorial() && hoverDiff >= 0 && hoverDiff != m_Difficulty)
	{
		m_Difficulty = hoverDiff;
		UpdateDifficultyLook();
		UpdateInfoText();
		m_SelectSE->Play();
	}

	//歯車(乗っている間は明るくする)
	bool onGear = HitGear(mouseX, mouseY);
	m_Gear->SetColor(onGear ? COLOR_SELECTED : COLOR_NORMAL);

	//左クリック
	if (Input::GetMouseTrigger(Input::MOUSE_LEFT))
	{
		int hitSong = HitSong(mouseX, mouseY);

		//歯車なら設定画面へ
		if (onGear)
		{
			OpenSettings();
			return;
		}

		//選択中の曲の行・ジャケット・(チュートリアルなら)PLAY なら、ゲーム開始
		bool hitPlay = IsTutorial() && HitPlay(mouseX, mouseY);
		if (hitSong == m_Select || HitJacket(mouseX, mouseY) || hitPlay)
		{
			StartGame();
			return;
		}

		//ほかの曲の行なら、その曲を選ぶだけ
		if (hitSong >= 0)
		{
			m_Select = hitSong;
			UpdateListLook();
			PlayPreview();
			m_SelectSE->Play();
		}
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
	GameSession::chartPath = SongList::songs[m_Select].chartPath[GetPlayDifficulty()];	// チュートリアルは EASY

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
	UpdateInfoText();		// 数字も選択中の曲に合わせる
	UpdateDifficultyLook();	// チュートリアルかどうかで、難易度と PLAY を切り替える
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
				info.audioPath = chart.audioPath;		// プレビューで使う

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

	const ChartInfo& info = m_ChartInfo[GetPlayDifficulty()][m_Select];
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
		chars[i]->Init(left, NUMBER_CENTER_Y - DIGIT_SIZE * 0.5f, DIGIT_SIZE, DIGIT_SIZE, L"asset\\texture\\00_Common\\number_atlas_512.png");
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

//==============================================================================
//難易度の枠と文字の見た目を選択に合わせる
//==============================================================================
void SelectScene::UpdateDifficultyLook()
{
	bool tutorial = IsTutorial();

	//チュートリアルなら PLAY だけ、そうでなければ難易度だけを見せる
	m_DiffLabel->SetAlpha(tutorial ? 0.0f : 1.0f);
	m_PlayFrame->SetAlpha(tutorial ? 1.0f : 0.0f);
	m_PlayText->SetAlpha(tutorial ? 1.0f : 0.0f);

	for (int d = 0; d < DIFFICULTY_MAX; d++)
	{
		bool selected = (d == m_Difficulty);

		m_DiffNormal[d]->SetAlpha((tutorial || selected) ? 0.0f : 1.0f);
		m_DiffSelected[d]->SetAlpha((!tutorial && selected) ? 1.0f : 0.0f);

		//SetColor はアルファも上書きするので、色の w で隠す
		XMFLOAT4 color = selected ? COLOR_SELECTED : COLOR_NORMAL;
		color.w = tutorial ? 0.0f : 1.0f;
		m_DiffText[d]->SetColor(color);
	}
}

//==============================================================================
//(x, y) が画面に出ている曲の行の上なら、その曲の番号を返す
//==============================================================================
int SelectScene::HitSong(int x, int y) const
{
	for (int i = 0; i < (int)SongList::songs.size(); i++)
	{
		int slot = GetSlot(i);
		if (slot < -LIST_ROWS_ABOVE || slot > LIST_ROWS_BELOW)
		{
			continue;		// 画面に出ていない曲は当たらない
		}

		//i 番目の曲の行(選択中の帯の位置から slot 行ずらした四角)
		float left = LIST_CURSOR_X;
		float top = LIST_CURSOR_Y + LIST_PITCH * slot;
		float right = left + LIST_CURSOR_W;
		float bottom = top + LIST_CURSOR_H;

		if (x >= left && x < right && y >= top && y < bottom)
		{
			return i;
		}
	}

	return -1;
}

//==============================================================================
//(x, y) がジャケットの上か
//==============================================================================
bool SelectScene::HitJacket(int x, int y) const
{
	return x >= JACKET_X && x < JACKET_X + JACKET_SIZE
		&& y >= JACKET_Y && y < JACKET_Y + JACKET_SIZE;
}

//==============================================================================
//(x, y) が難易度の枠の上なら、その難易度を返す
//==============================================================================
int SelectScene::HitDifficulty(int x, int y) const
{
	for (int d = 0; d < DIFFICULTY_MAX; d++)
	{
		float left = BPM_X + DIFF_PITCH * d;		// Init で枠を置いたのと同じ式
		float top = DIFF_Y;

		if (x >= left && x < left + DIFF_W && y >= top && y < top + DIFF_H)
		{
			return d;
		}
	}

	return -1;
}

//==============================================================================
//(x, y) が歯車の上か
//==============================================================================
bool SelectScene::HitGear(int x, int y) const
{
	return x >= GEAR_X && x < GEAR_X + GEAR_SIZE
		&& y >= GEAR_Y && y < GEAR_Y + GEAR_SIZE;
}

//==============================================================================
//設定画面へ(戻ってきたときに同じ曲・難易度から始められるよう、今の選択を覚えておく)
//==============================================================================
void SelectScene::OpenSettings()
{
	GameSession::songIndex = m_Select;
	GameSession::difficulty = m_Difficulty;
	GameSession::settingsFromSelect = true;		// 戻り先は選曲画面

	m_DecideSE->Play();
	Manager::ChangeScene<SettingsScene>(FADE_TIME);
	m_IsLeaving = true;
	m_FadeTime = 0.0f;
}

//==============================================================================
//選択中の曲のプレビューを頭から流す(ほかの曲は止める)
//==============================================================================
void SelectScene::PlayPreview()
{
	//いったん全部止める
	for (Audio* preview : m_Preview)
	{
		if (preview)
		{
			preview->Pause();
		}
	}

	m_PreviewTime = 0.0f;

	if (SongList::songs.empty())
	{
		return;
	}

	//選択中の曲を previewStart から流す
	Audio* preview = m_Preview[m_Select];
	if (preview)
	{
		preview->PlayFrom(SongList::songs[m_Select].previewStart);
	}
}

//==============================================================================
//選択中の曲がチュートリアルか
//==============================================================================
bool SelectScene::IsTutorial() const
{
	if (SongList::songs.empty())
	{
		return false;
	}
	return SongList::songs[m_Select].tutorial;
}

//==============================================================================
//実際に遊ぶ難易度(チュートリアルは難易度を選ばないので、いつも EASY)
//==============================================================================
int SelectScene::GetPlayDifficulty() const
{
	return IsTutorial() ? DIFFICULTY_EASY : m_Difficulty;
}

//==============================================================================
//(x, y) が PLAY の枠の上か
//==============================================================================
bool SelectScene::HitPlay(int x, int y) const
{
	return x >= BPM_X && x < BPM_X + PLAY_W
		&& y >= DIFF_Y && y < DIFF_Y + DIFF_H;
}
