#pragma once
#include "Scene.h"
#include <vector>
#include "ExitDialog.h"

class TitleScene :public Scene
{
protected:
	// メニューの項目(上から順)
	enum MENU
	{
		MENU_START,
		MENU_SETTINGS,
		MENU_EXIT,

		MENU_MAX
	};


public:
	class Polygon2D* m_Logo;

	class Polygon2D* m_MenuText[MENU_MAX];
	class Polygon2D* m_Cursor;				// 選択中の帯
	class Polygon2D* m_CursorLine;			// 帯の左の水色の線
	int m_Select;							// 選択中の項目(MENU の値)


	class Polygon2D* m_Fade;				// フェード用の黒い板
	float m_FadeTime;						// フェードが始まってからの時間(秒)
	bool  m_IsLeaving;						// true: 次の画面へ出ていく途中(フェードアウト中)

	float m_Time;							// タイトルを開いてからの時間(ロゴの光に使う)

	class Audio* m_SelectSE;				// 選択が変わったときの音
	class Audio* m_DecideSE;				// 決定したときの音
	class Audio* m_BGM;						// タイトルのBGM

	ExitDialog m_ExitDialog;				// 終了確認
	class Polygon2D* m_GuideEscExit;		// キーガイド「Esc 終了」
	class Polygon2D* m_GuideEscCancel;		// キーガイド「Esc キャンセル」

	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void UpdateMenuLook();					// 選択に合わせて色と帯の位置を変える
	int  HitMenu(int x, int y);				// (x, y) にある項目の番号。どれにも当たらなければ -1

	void OpenExitDialog();					// 終了確認を開いて、キーガイドを切り替える
	void SetEscGuide(bool cancel);			// true:「Esc キャンセル」 false:「Esc 終了」
};