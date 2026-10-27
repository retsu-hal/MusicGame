#pragma once
#include <vector>
#include <string>

constexpr int TICKS_PER_BEAT = 480;   // 1拍 = 480 tick

// BPM の変化点
struct TempoEvent
{
	int    tick = 0;   // どの位置から
	double bpm = 120.0; // この BPM になる
};

// ノーツ1つ
struct Note
{
	int tick = 0;   // 位置
	int lane = 0;   // レーン番号(0〜)
	int type = 0;   // 0:タップ 1:ロング など
	int length = 0;   // ロングノーツの長さ(tick)
	int layer = 0;   // 0:下段 1:上段
};

// アークの点1個
struct ArcPoint
{
	int   tick = 0;      // 位置
	float x = 0.0f;   // 横の位置(レーン単位。0 = 左端、レーン i の真ん中 = i + 0.5)
	int   ease = 0;      // 次の点までのつなぎ方 0:まっすぐ 1:だんだん速く 2:だんだん遅く 3:S字
};

// アーク1本
struct ArcLine	
{
	int                   layer = 0;   // 見た目の高さ 0:下段 1:上段
	std::vector<ArcPoint> points;      // tick の順
};

class Chart
{
public:
	std::string title;
	std::string audioPath;
	double offset = 0.0;              // 曲の頭から1拍目までの秒数（曲ごとに違う）

	std::vector<TempoEvent> tempos;   // tick 順に並べる。先頭は必ず tick 0
	std::vector<Note>       notes;
	std::vector<ArcLine>        arcs;

	double TickToSec(int tick) const;  // tick → 秒
	double SecToTick(double sec) const;// 秒 → tick（小数あり。ノーツ移動用）

	bool Save(const std::string& path) const;
	bool Load(const std::string& path);
};