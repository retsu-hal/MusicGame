#pragma once

#include "GameObject.h"
#include "Renderer.h"

class Polygon2D : public GameObject
{
private:
	XMFLOAT4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };	
	VERTEX_3D m_Vertex[4];	// 頂点(あとから色を変えるので覚えておく)
	XMFLOAT4 m_Param = { 0.0f, 0.0f, 0.0f, 0.0f };	// シェーダーに渡す数値(時間など)


public:
	void Init() override {}
	void Init(float x, float y, float width, float height, const WCHAR* TextureName);
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void SetColor(const XMFLOAT4& color) { m_Color = color; }
	XMFLOAT4 GetColor() const { return m_Color; }
	void SetAlpha(float alpha) { m_Color.w = alpha; }
	void SetGradientAlpha(float leftAlpha, float rightAlpha);	// 左端と右端の透明度を変える

	void SetPixelShader(const char* FileName);		// ピクセルシェーダーを差し替える
	void SetParam(const XMFLOAT4& param) { m_Param = param; }
};