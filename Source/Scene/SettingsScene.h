#pragma once
#include "Scene.h"

class SettingsScene : public Scene
{
public:
	// タブ(上から順)
	enum TAB
	{
		TAB_GAMEPLAY,
		TAB_AUDIO,
		TAB_DISPLAY,

		TAB_MAX
	};

	//  GAMEPLAY タブの行(上から。ITEM_IMAGE の GAMEPLAY と同じ並び)
	enum GAMEPLAY_ROW
	{
		GP_HI_SPEED,
		GP_JUDGE_OFFSET,
		GP_VISUAL_OFFSET,
		GP_LAYER_GUIDE,
		GP_FAST_SLOW,
	};

	// ★ DISPLAY タブの行(上から。ITEM_IMAGE の DISPLAY と同じ並び)
	enum DISPLAY_ROW
	{
		DP_SCREEN_MODE,
		DP_RESOLUTION,
		DP_V_SYNC,
		DP_BG_BRIGHTNESS,
		DP_SHOW_FPS,
	};

	static const int ROW_MAX = 5;					// 1つのタブに並ぶ項目の最大数(GAMEPLAY と DISPLAY が5つ)
	static const int VOLUME_NUM = 3;				// AUDIO タブの行数(MASTER / MUSIC / SE)
	static const int DIGIT_MAX = 3;					// 音量の数字の桁数(最大 100 なので3桁)
	static const int VALUE_CHAR_MAX = 4;			//  GAMEPLAY の値の文字数の最大("+200" "20.0" で4文字)

	class Polygon2D* m_TabText[TAB_MAX];			// GAMEPLAY / AUDIO / DISPLAY
	class Polygon2D* m_TabCursor;					//  選択中のタブの帯
	class Polygon2D* m_TabCursorLine;				// タブの帯の左の水色の線
	int m_Tab;										// 選択中のタブ(TAB の値)
	int m_HoverTab;									//  前のフレームでマウスが乗っていたタブ(-1: どれにも乗っていない)

	class Polygon2D* m_SliderBack[VOLUME_NUM];					//  スライダーのうすい線(全体)
	class Polygon2D* m_SliderFill[VOLUME_NUM];					//  スライダーの色付きの線(値のところまで)
	class Polygon2D* m_SliderKnob[VOLUME_NUM];					//  スライダーのつまみ
	class Polygon2D* m_VolumeDigit[VOLUME_NUM][DIGIT_MAX];		// 数字 [行][桁]。桁は 0 が一の位
	int  m_DragRow;												// ドラッグ中のスライダーの行(-1: ドラッグしていない)
	bool m_DragChanged;											// 今回のドラッグで値が変わったか(離したときの音に使う)

	class Polygon2D* m_ValueChar[ROW_MAX][VALUE_CHAR_MAX];		//  GAMEPLAY の値の文字 [行][右から何文字目]。使わない行は nullptr
	class Polygon2D* m_ValueMs[ROW_MAX];						// 「ms」(OFFSET の2行だけ)
	class Polygon2D* m_ValueOn[ROW_MAX];						// 「ON」(LAYER GUIDE / FAST・SLOW の2行だけ)
	class Polygon2D* m_ValueOff[ROW_MAX];						// 「OFF」(同じ2行だけ)
	class Polygon2D* m_ArrowLeft;								//  選択中の行の ◀
	class Polygon2D* m_ArrowRight;								//  選択中の行の ▶
	float m_ArrowLeftX;											// ◀ を置いた x(クリックの当たり判定に使う)

	class Polygon2D* m_ScreenWindow;							//  SCREEN MODE の「WINDOW」
	class Polygon2D* m_ScreenFull;								//  SCREEN MODE の「FULLSCREEN」
	class Polygon2D* m_FpsOn;									// SHOW FPS の「ON」
	class Polygon2D* m_FpsOff;									//  SHOW FPS の「OFF」
	bool m_ShownFullscreen;										// 今の表示がフルスクリーンのつもりで描いているか(F11 との食い違いに気づくため)


	class Polygon2D* m_ItemName[TAB_MAX][ROW_MAX];	// 項目名(HI-SPEED など)。項目が無いところは nullptr
	class Polygon2D* m_ItemDesc[TAB_MAX][ROW_MAX];	// 項目の説明(日本語)
	class Polygon2D* m_RowLine[ROW_MAX];			//  行の下の区切り線
	class Polygon2D* m_RowCursor;					// 選択中の行の帯
	class Polygon2D* m_RowCursorLine;				// 行の帯の左の水色の線
	int m_Row;										//  選択中の行(上から 0, 1, 2 …)
	int m_HoverRow;									//  前のフレームでマウスが乗っていた行(-1: どれにも乗っていない)




	class Polygon2D* m_Fade;		// フェード用の黒い板
	float m_FadeTime;				// フェードが始まってからの時間(秒)
	bool  m_IsLeaving;				// true: タイトルへ戻る途中(フェードアウト中)

	class Audio* m_SelectSE;		
	class Audio* m_DecideSE;		
	class Audio* m_BGM;				



	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void UpdateLook();				//  選択に合わせて色・帯の位置・見せる項目を変える
	int  GetRowCount() const;		// 今のタブの項目の数

	int  HitTab(int x, int y);		// (x, y) にあるタブの番号。どれにも当たらなければ -1
	int  HitRow(int x, int y);		// (x, y) にある行の番号。どれにも当たらなければ -1
	int  HitSlider(int x, int y);	//  (x, y) にあるスライダーの行。どれにも当たらなければ -1
	bool ChangeVolume(int row, int value);	//  row 行目の音量を value にする。変わったら true
	bool ChangeGameplay(int row, int dir);	//  GAMEPLAY の row 行目の値を dir(-1 / +1)の向きに変える。変わったら true
	int  HitArrow(int x, int y);			// (x, y) が ◀ なら -1、▶ なら +1、どちらでもなければ 0
	float SetValueText(int row, const char* text, float right, float centerY, const XMFLOAT4& color);	// 文字を右揃えで並べる。左端の x を返す
	bool ChangeDisplay(int row, int dir);	//  DISPLAY の row 行目の値を変える。変わったら true
	void PlaceArrows(float left, float centerY);	//  値の左端 left の左と、行の右端に ◀ ▶ を置く
};