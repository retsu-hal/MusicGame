#pragma once
#include "Scene.h"
#include "ExitDialog.h"
#include "SongList.h"		
#include <vector>

class SelectScene : public Scene
{
public:

	// 譜面から計算した、表示用の曲の情報
	struct ChartInfo
	{
		bool loaded = false;	// 譜面を読めたか
		int  bpmMin = 0;		// 一番遅い BPM
		int  bpmMax = 0;		// 一番速い BPM
		int  notes = 0;		// ノーツの数
		int  lengthSec = 0;		// 最後のノーツが終わるまでの秒数
		std::string audioPath;	// 譜面に書いてある曲のファイル
	};

	static const int INFO_CHAR_MAX = 7;				// 数字の最大文字数("120-180" で7文字)

	class Polygon2D* m_Fade;				// フェード用の黒い板
	float m_FadeTime;						// フェードが始まってからの時間(秒)
	bool  m_IsLeaving;						// true: 次の画面へ出ていく途中(フェードアウト中)

	class Audio* m_SelectSE;				// 選択が変わったときの音
	class Audio* m_DecideSE;				// 決定したときの音
	std::vector<class Audio*> m_Preview;	// プレビューの音(曲ごとに1つ。SongList::songs と同じ並び。音が無い曲は nullptr)
	float m_PreviewTime;					// 今のプレビューを流し始めてからの時間(秒)

	ExitDialog m_ExitDialog;				// 終了確認
	class Polygon2D* m_GuideEscExit;		// キーガイド「Esc 終了」
	class Polygon2D* m_GuideEscCancel;		// キーガイド「Esc キャンセル」

	int m_Select;							// 選択中の曲(SongList::songs の番号)
	int m_Difficulty;						// 選択中の難易度(DIFFICULTY の値)

	std::vector<class Polygon2D*> m_SongTitle;		// 曲名(曲ごとに1つ。SongList::songs と同じ並び)
	std::vector<class Polygon2D*> m_SongArtist;		// アーティスト名(同じ並び)
	class Polygon2D* m_ListCursor;					// 選択中の行の帯
	class Polygon2D* m_ListCursorLine;				// 帯の左の水色の線

	std::vector<class Polygon2D*> m_Jacket;			// ジャケット(曲ごとに1つ。選択中の曲だけ見せる)
	std::vector<class Polygon2D*> m_InfoTitle;		// 右の大きい曲名(同じ)
	std::vector<class Polygon2D*> m_InfoArtist;		// 右の大きいアーティスト名(同じ)

	std::vector<ChartInfo> m_ChartInfo[DIFFICULTY_MAX];	// [難易度][曲] の順。SongList::songs と同じ並び

	class Polygon2D* m_BpmChar[INFO_CHAR_MAX];			// BPM の数字(左から1文字ずつ)
	class Polygon2D* m_LengthChar[INFO_CHAR_MAX];		// LENGTH の数字
	class Polygon2D* m_NotesChar[INFO_CHAR_MAX];		// NOTES の数字

	class Polygon2D* m_DiffNormal[DIFFICULTY_MAX];		// 選んでいない難易度の枠
	class Polygon2D* m_DiffSelected[DIFFICULTY_MAX];	// 選んでいる難易度の枠
	class Polygon2D* m_DiffText[DIFFICULTY_MAX];		// EASY / HARD の文字

	class Polygon2D* m_DiffLabel;						// 見出し「DIFFICULTY」(チュートリアルのときは隠す)
	class Polygon2D* m_PlayFrame;						// チュートリアル用の PLAY の枠
	class Polygon2D* m_PlayText;						// PLAY の文字

	class Polygon2D* m_Gear;							// 右上の歯車(設定画面へ)

	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void StartGame();						// 選んだ譜面を GameSession に入れて、ゲームへ
	void SetEscGuide(bool cancel);			// true:「Esc キャンセル」 false:「Esc 終了」

	void UpdateListLook();					// 選択に合わせて曲の位置と色を変える
	int  GetSlot(int song) const;			// song 番目の曲が、選択中の行から何行ずれた所にあるか

	void LoadChartInfo();									// 全曲・全難易度の譜面を読んで m_ChartInfo を作る
	void UpdateInfoText();									// 選択中の曲・難易度の数字を出す
	void CreateNumberText(class Polygon2D* chars[], float left);	// 数字用の Polygon2D を INFO_CHAR_MAX 個作る
	void SetNumberText(class Polygon2D* chars[], const char* text, float left);	// 文字を左揃えで並べる

	void UpdateDifficultyLook();							// 選択中の難易度に合わせて枠と文字の見た目を変える

	int  HitSong(int x, int y) const;						// (x, y) が画面に出ている曲の行の上なら、その曲の番号。どれでもなければ -1
	bool HitJacket(int x, int y) const;						// (x, y) がジャケットの上か
	int  HitDifficulty(int x, int y) const;					// (x, y) が難易度の枠の上なら、その難易度。どれでもなければ -1

	bool HitGear(int x, int y) const;						// (x, y) が歯車の上か
	void OpenSettings();									// 設定画面へ

	void PlayPreview();										// 選択中の曲のプレビューを頭(previewStart)から流す。ほかの曲は止める

	bool IsTutorial() const;								// 選択中の曲がチュートリアルか
	int  GetPlayDifficulty() const;							// 実際に遊ぶ難易度(チュートリアルなら EASY、それ以外は m_Difficulty)
	bool HitPlay(int x, int y) const;						// (x, y) が PLAY の枠の上か
};