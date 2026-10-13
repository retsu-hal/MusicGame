#pragma once
#include "GameObject.h"
#include "Chart.h"
#include <string>
#include <vector>

class Audio;

// 譜面エディタ（ImGui のウィンドウとして表示する）
class ChartEditor : public GameObject
{
private:
	Chart       m_Chart;
	char        m_Path[256] = "asset\\chart\\test.json";   // 編集中のファイル
	Audio*      m_Music{ nullptr };
	bool        m_Playing = false;    // エディタ上で再生中か
	bool        m_Dirty   = false;    // 保存していない変更があるか
	std::string m_Message;            // 画面下に出すお知らせ（保存しました など）

	// 表示の設定
	int    m_LaneCount     = 4;
	float  m_LaneWidth     = 70.0f;
	float  m_PixelsPerBeat = 120.0f;  // 1拍分の高さ（ズーム）
	double m_ViewTick      = -240.0;  // 画面の一番下にある tick（スクロール位置）
	int    m_CursorTick    = 0;       // 再生を始める位置
	int    m_SnapIndex     = 3;       // SNAP_DIVS の何番目を使うか

	// Undo / Redo（変更する前の譜面を丸ごと保存しておく方式）
	std::vector<Chart> m_UndoStack;   // 戻す用
	std::vector<Chart> m_RedoStack;   // やり直す用
	int  m_MergeId = 0;               // 同じ入力欄の連続編集を1回にまとめるための番号

	// 終了の確認
	bool m_OpenExitPopup = false;     // 「保存していません」ダイアログを開く
	bool m_ExitRequested = false;     // シーン側に「戻りたい」と伝える

	int  GetSnapTicks() const;
	int  FindNote(int lane, int tick) const;   // 見つからなければ -1
	void LoadChart();
	void SaveChart();
	void StartPlay();
	void StopPlay();

	void RecordUndo(int mergeId = 0); // ★ 譜面を変更する「直前」に必ず呼ぶ
	void Undo();
	void Redo();

	void DrawToolbar();
	void DrawTimeline();
	void DrawExitPopup();

public:
	void Init() override;
	void Update() override;

	bool IsExitRequested() const { return m_ExitRequested; }
};
