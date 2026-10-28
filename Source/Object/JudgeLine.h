#pragma once
#include "main.h"
#include "GameObject.h"
#include "Lane.h"

// 判定ライン・キー枠・アークのカーソル
// 判定ラインは上段(固定)と今いる面、キー枠とカーソルは今いる面。レイヤー3 = ノーツより手前
class JudgeLine : public GameObject
{
private:
	Layout m_play;	// 今いる面(上下する)
	Layout m_upper; // 上段の判定ライン(動かない)
	ID3D11ShaderResourceView *m_White = nullptr;
	ID3D11ShaderResourceView *m_Cursor = nullptr;
	ID3D11ShaderResourceView *m_KeyTex[LANE_COUNT] = {};
	float m_Glow[LANE_COUNT] = {};

public:
	void Init() override;
	void Update() override;
	void Draw() override;
};