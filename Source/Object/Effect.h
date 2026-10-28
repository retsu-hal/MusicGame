#pragma once
#include "GameObject.h"
#include "Judge.h"
#include <vector>

// 広がって消える輪1個ぶん
struct HitRing
{
    float lane;  // 輪を出す範囲の左端(レーン単位)
    float width; // 輪を出す範囲の幅(レーン数)
    float time;  // 出してからの時間(秒)
    Judge judge; // 色を決めるのに使う
};

// 判定文字・FAST / SLOW・叩いたときの輪。レイヤー3(ノーツより手前)
class Effect : public GameObject
{
private:
    ID3D11ShaderResourceView *m_JudgeTex[6] = {}; // Judge の番号で引く(0 = None は空)
    ID3D11ShaderResourceView *m_FastTex = nullptr;
    ID3D11ShaderResourceView *m_SlowTex = nullptr;
    ID3D11ShaderResourceView *m_RingTex = nullptr;

    Judge m_Judge = Judge::None; // 今出している判定文字(None = 出していない)
    float m_JudgeTime = 0.0f;    // 判定文字を出してからの時間(秒)
    int m_Timing = 0;            // -1:FAST 1:SLOW 0:出さない

    std::vector<HitRing> m_Rings; // 今出ている輪

public:
    void Init() override;
    void Update() override;
    void Draw() override;

    void Show(Judge judge, int lane, int width, double diff); // NoteManager が判定のたびに呼ぶ
};