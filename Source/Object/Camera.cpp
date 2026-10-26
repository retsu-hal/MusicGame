#include "Manager.h"
#include "Camera.h"
#include "Player.h"

#define POS_MAX 100.0f
#define POS_MIN -100.0f
#define FOV_MAX 46.0f
#define FOV_MIN 44.0f

void CAMERA::Init()
{
	m_Layer = 0;
}

void CAMERA::Uninit()
{

}

void CAMERA::Update()
{

}

void CAMERA::Draw()
{
	// ビュー行列：どこから、どこを見るか
	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_ViewMatrix = XMMatrixLookAtLH(
		XMLoadFloat3((XMFLOAT3*)&m_Position), XMLoadFloat3((XMFLOAT3*)&m_Target), XMLoadFloat3(&up));

	// 射影行列：近い面の上下左右を、画面のピクセルから決める
	//   近い面の大きさ = 画面のピクセル × (near ÷ 焦点距離)
	float k = m_Near / m_FocalLength;
	float left = -m_LensCenter.x * k;                     // 真正面より左に見える分
	float right = (SCREEN_WIDTH - m_LensCenter.x) * k;     // 右に見える分
	float top = m_LensCenter.y * k;                      // 上に見える分
	float bottom = -(SCREEN_HEIGHT - m_LensCenter.y) * k;   // 下に見える分
	m_ProjectionMatrix = XMMatrixPerspectiveOffCenterLH(left, right, bottom, top, m_Near, m_Far);

	Renderer::SetWorldMatrix(XMMatrixIdentity());
	Renderer::SetViewMatrix(m_ViewMatrix);
	Renderer::SetProjectionMatrix(m_ProjectionMatrix);
	Renderer::SetCameraPosition(m_Position);
}
