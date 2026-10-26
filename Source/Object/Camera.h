#pragma once

#include "main.h"
#include "Renderer.h"
#include "Vector3.h"
#include "GameObject.h"
class CAMERA : public GameObject
{
private:
	Vector3  m_Target{ 0.0f, 0.0f, 1.0f };     // 注視点
	float    m_FocalLength = 600.0f;           // 焦点距離(ピクセル)。大きいほど望遠で、奥行きの縮みが弱い
	XMFLOAT2 m_LensCenter{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f };  // 真正面が映る画面の位置
	float    m_Near = 10.0f;                   // これより近いものは描かない
	float    m_Far = 10000.0f;                // これより遠いものは描かない

	XMMATRIX m_ViewMatrix = XMMatrixIdentity();
	XMMATRIX m_ProjectionMatrix = XMMatrixIdentity();


public:

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void    SetTarget(const Vector3& target) { m_Target = target; }
	Vector3 GetTarget() const { return m_Target; }

	void SetFocalLength(float pixel) { m_FocalLength = pixel; }
	void SetLensCenter(float x, float y) { m_LensCenter = XMFLOAT2(x, y); }
	void SetClip(float nearZ, float farZ) { m_Near = nearZ; m_Far = farZ; }



	XMMATRIX GetViewMatrix() { return m_ViewMatrix; }
	XMMATRIX GetProjectionMatrix() const { return m_ProjectionMatrix; }

	Vector3 GetForward() override
	{
		Vector3 forward = m_Target - m_Position;
		forward.normalize();

		return forward;
	}

	Vector3 GetRight() override
	{
		Vector3 forward = m_Target - m_Position;
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		Vector3 right = Vector3::cross(up, forward);
		right.normalize();

		return right;
	}
};
