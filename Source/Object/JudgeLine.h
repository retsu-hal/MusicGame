#pragma once
#include "main.h"
#include "GameObject.h"
#include "Lane.h"

struct ShaderSet;

// 判定ライン・キー枠・アークのカーソル
// 判定ラインは上段(固定)と今いる面、キー枠とカーソルは今いる面。レイヤー3 = ノーツより手前
class JudgeLine : public GameObject
{
private:
	Layout m_play;
	Layout m_upper;
	ID3D11ShaderResourceView *m_WhiteTex = nullptr;
	ID3D11ShaderResourceView *m_CursorTex = nullptr;
	ID3D11ShaderResourceView *m_KeyTex[LANE_COUNT] = {};
	const ShaderSet *m_Shader = nullptr;
	float m_Glow[LANE_COUNT] = {};

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};