#pragma once
#include "NoteBase.h"
#include "Lane.h"
#include "Judge.h"
#include <vector>

class PlaneRenderer;

// 左右フリック1個ぶんのデータ
struct FlickData
{
    double time = 0.0; // 振る時刻(秒)
    int lane = 0;      // 左端のレーン
    int width = 1;     // 何レーンぶんか
    int layer = 0;     // 見た目の高さ 0:下段 1:上段
    int dir = 0;       // 1:右 -1:左
    Judge judge = Judge::None;
};

// 左右フリック全部。レイヤー2(床と判定ラインの間)
class FlickNote : public NoteBase
{
private:
    std::vector<FlickData> m_Notes; // 時刻の順
    Layout m_Layout;
    PlaneRenderer *m_RightTex = nullptr;
    PlaneRenderer *m_LeftTex = nullptr;

    void DrawDir(int dir, float now, float speed); // 片方の向きだけ描く
    void SetJudge(FlickData &note, Judge judge, double diff);

public:
    void Init() override;
    void Update() override;
    void Draw() override;

    void Load(const Chart &chart) override;
    void Reset(double startSec) override;
    int GetJudgeCount() const override { return (int)m_Notes.size(); }
    double GetLastTime() const override { return m_Notes.empty() ? 0.0 : m_Notes.back().time; }
};