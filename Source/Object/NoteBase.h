#pragma once
#include "GameObject.h"

class Chart;
class NoteManager;

// ノーツの種類の親クラス。
// 種類ごとに1個の GameObject で、その種類のノーツ全部を持って、判定と描画をする
class NoteBase : public GameObject
{
protected:
	NoteManager* m_Manager = nullptr;   // 判定結果を知らせる先・今の段を聞く先

public:
	void SetManager(NoteManager* manager) { m_Manager = manager; }

	// 譜面から自分の種類のノーツだけ拾う
	virtual void   Load(const Chart& chart) = 0;
	// 最初から(途中から)やり直す。startSec より前のノーツは Skip にする
	virtual void   Reset(double startSec) = 0;
	// 判定の回数(達成率の分母)。タップは1個 = 1回、ロングは始点・中継点・終点の合計
	virtual int    GetJudgeCount() const = 0;
	// いちばん最後のノーツの時刻(秒)。ノーツがなければ 0
	virtual double GetLastTime() const = 0;
};