#pragma once
#include <vector>

class ExitDialog
{
public:
	// Update の結果
	enum RESULT
	{
		RESULT_NONE,	// まだ決まっていない(開いていないときもこれ)
		RESULT_YES,		// YES が押された
		RESULT_NO,		// NO が押された / Esc で閉じた
	};

private:
	// ボタン(左から順)
	enum BUTTON
	{
		BUTTON_YES,
		BUTTON_NO,

		BUTTON_MAX
	};

	bool m_IsOpen = false;								// true: 表示中
	int  m_Select = BUTTON_YES;							// 選択中のボタン(BUTTON の値)

	class Polygon2D* m_Dim = nullptr;					// 後ろを暗くする黒い板
	std::vector<class Polygon2D*> m_Parts;				// 枠・見出し・質問(出す/消すだけのもの)
	class Polygon2D* m_BtnNormal[BUTTON_MAX] = {};		// 選んでいないボタンの枠
	class Polygon2D* m_BtnSelected[BUTTON_MAX] = {};	// 選んでいるボタンの枠
	class Polygon2D* m_BtnText[BUTTON_MAX] = {};		// YES / NO
	class Polygon2D* m_BtnSub = nullptr;				// 「終了する」(YES の横)

	class Audio* m_SelectSE = nullptr;					// 選択が変わったときの音(シーンから借りる)
	class Audio* m_DecideSE = nullptr;					// 決定したときの音(シーンから借りる)

	void UpdateLook();									// 選択に合わせてボタンの見た目を変える
	int  HitButton(int x, int y);						// (x, y) にあるボタンの番号。無ければ -1

public:
	void Init(class Audio* selectSE, class Audio* decideSE);	// 部品を作る(最初は隠しておく)
	void Open();												// 開く
	void Close();												// 閉じる
	bool IsOpen() const { return m_IsOpen; }					// 開いているか
	RESULT Update();											// 開いている間の操作。結果を返す
};