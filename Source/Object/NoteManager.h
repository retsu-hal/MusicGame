#pragma once
#include "GameObject.h"
#include "Chart.h"
#include "Judge.h"
#include <vector>
#include <string>
#include <deque>

class Audio;
class Score;
class Combo;
class NoteBase;

// マウスの移動の記録1個(左右フリック用)
struct MouseSample
{
	double time;  // 曲の時刻(秒)
	float move;	  // そのフレームで動いた量(レーン単位。端で止める前)
	float startX; // 動く前のカーソルの位置
};

// ノーツの管理:譜面を読んで各ノーツに配る・段の状態・判定結果を配る・リザルト用の集計
// 描画はしない(ノーツは各ノーツが自分で描く)
class NoteManager : public GameObject
{
private:
	Chart m_Chart;
	Audio *m_Music = nullptr;		 // ステップ5で GameScene に引っ越す
	std::vector<NoteBase *> m_Notes; // ノーツの種類(TapNote など)。中身は Manager が持っている

	Score *m_Score = nullptr;
	Combo *m_Combo = nullptr;

	// 段
	int m_PlayLayer = 0;		  // 今いる段 0:下段 1:上段(判定はこっちを見る)
	float m_PlayLayerAnim = 0.0f; // 見た目用。0〜1 をなめらかに動く

	// カーソル
	float m_CursorX = 0.0f;
	std::deque<MouseSample> m_MouseLog;

	// 集計(リザルト用)
	int m_Count[(int)Judge::Max] = {}; // 判定ごとの回数
	int m_TotalJudges = 0;			   // 判定の回数の合計(達成率の分母)

	// 曲の終わり
	double m_EndTime = 0.0;
	bool m_Finished = false;

	void Finish();

public:
	void Init() override;
	void Uninit() override;
	void Update() override;

	bool LoadChart(const std::string &path);
	void Play();
	void OnJudge(Judge judge, int lane, double diff); // ノーツが判定したら呼ぶ

	const Chart &GetChart() const { return m_Chart; }
	int GetPlayLayer() const { return m_PlayLayer; }
	float GetPlayLayerAnim() const { return m_PlayLayerAnim; }
	bool IsFinished() const { return m_Finished; }
	float GetCursorX() const { return m_CursorX; }
	float GetFlickMove() const;
	float GetFlickStartX() const;
	void ClearFlick() { m_MouseLog.clear(); }
};