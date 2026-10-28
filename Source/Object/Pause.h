#pragma once

class PauseMenu
{
public:
    // Update の結果
    enum RESULT
    {
        RESULT_NONE,
        RESULT_RESTART,
        RESULT_RETRY,
        RESULT_SELECT,
    };

private:
    // 今の状態
    enum STATE
    {
        STATE_CLOSED,
        STATE_MENU,
        STATE_COUNT,
    };

    // メニューの項目(上から順)
    enum MENU
    {
        MENU_CONTINUE,
        MENU_RETRY,
        MENU_SELECT,

        MENU_MAX
    };

    STATE m_State = STATE_CLOSED;
    int m_Select = MENU_CONTINUE; // 選択中の項目(MENU の値)
    float m_CountTime = 0.0f;     // カウントが始まってからの時間(秒)

    class Polygon2D *m_Dim = nullptr;           // 後ろを暗くする黒い板
    class Polygon2D *m_Pause = nullptr;         // 「PAUSE」
    class Polygon2D *m_Cursor = nullptr;        // 選択中の帯
    class Polygon2D *m_CursorLine = nullptr;    // 帯の左の水色の線
    class Polygon2D *m_MenuText[MENU_MAX] = {}; // CONTINUE / RETRY / SELECT
    class Polygon2D *m_Guide[2] = {};           // キーガイド「クリック 決定」「Esc 続ける」
    class Polygon2D *m_Count = nullptr;         // 3・2・1 の数字

    class Audio *m_SelectSE = nullptr; // 選択が変わったときの音
    class Audio *m_DecideSE = nullptr; // 決定したときの音

    void ShowMenu(bool show);  // メニューの部品を出す / 消す
    void UpdateMenuLook();     // 選択に合わせて色と帯の位置を変える
    int HitMenu(int x, int y); // (x, y) にある項目の番号。無ければ -1

public:
    void Init();                                            // 部品を作る(最初は隠しておく)
    void Open();                                            // メニューを開く
    void Close();                                           // 全部消す
    bool IsOpen() const { return m_State != STATE_CLOSED; } // 開いているか(カウント中も含む)
    RESULT Update();                                        // 開いている間の操作。結果を返す
};