#pragma once

// チュートリアル曲の最初に1回だけ出す「遊び方」の画面
// PauseMenu と同じく、GameScene が持って Update を呼ぶ
class TutorialWindow
{
public:
	// Update の結果
	enum RESULT
	{
		RESULT_NONE,	// まだ開いている
		RESULT_CLOSE,	// 「閉じる」が押された → 曲を始める
	};

private:
	static constexpr int PAGE_MAX = 4;	// ページの数

	bool m_Open = false;		// 開いているか
	int  m_Page = 0;			// 今のページ(0 〜 PAGE_MAX-1)
	bool m_HoverClose = false;	// 「閉じる」の上にマウスが乗っているか

	class Polygon2D* m_Dim = nullptr;				// 後ろを暗くする黒い板
	class Polygon2D* m_Window = nullptr;			// 枠
	class Polygon2D* m_PageImg[PAGE_MAX] = {};			// ページの中身
	class Polygon2D* m_ArrowL = nullptr;			// ← 前のページ
	class Polygon2D* m_ArrowR = nullptr;			// → 次のページ
	class Polygon2D* m_DotOff[PAGE_MAX] = {};		// ページの点(白抜き)
	class Polygon2D* m_DotOn = nullptr;				// 今のページの点(1つを動かす)
	class Polygon2D* m_BtnNormal = nullptr;			// 「閉じる」の板(ふだん)
	class Polygon2D* m_BtnHover = nullptr;			// 「閉じる」の板(マウスが乗ったとき)
	class Polygon2D* m_CloseText = nullptr;			// 「閉じる」の文字

	class Audio* m_SelectSE = nullptr;	// ページをめくった・ボタンに乗ったときの音
	class Audio* m_DecideSE = nullptr;	// 「閉じる」を押したときの音

	void UpdateLook();	// 今のページとホバーに合わせて、出す・消すを切り替える
	static bool Hit(int x, int y, float left, float top, float width, float height);	// (x, y) が四角の中か

public:
	void Init();		// 部品を作る(最初は隠しておく)
	void Open();		// 1ページ目から開く
	void Close();		// 全部消す
	bool IsOpen() const { return m_Open; }
	RESULT Update();	// 開いている間の操作
};