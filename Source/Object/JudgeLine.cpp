#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "NoteManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "JudgeLine.h"

void JudgeLine::Init()
{
	m_Layer = 3; // ノーツより手前

	// 頂点:原点が真ん中の 1×1 の板
	VERTEX_3D vertex[4];
	{
		vertex[0].Position = XMFLOAT3(-0.5f, 0.5f, 0.0f); // 左上
		vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

		vertex[1].Position = XMFLOAT3(0.5f, 0.5f, 0.0f); // 右上
		vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

		vertex[2].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f); // 左下
		vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

		vertex[3].Position = XMFLOAT3(0.5f, -0.5f, 0.0f); // 右下
		vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);
	}

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

	// シェーダー読み込み
	m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");

	// テクスチャ読み込み
	m_WhiteTex = TextureManager::Load(L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_CursorTex = TextureManager::Load(L"asset\\texture\\02_Play\\icon\\cursor.png");

}

void JudgeLine::Uninit()
{
	m_vertexBuffer->Release();

	GameObject::Uninit();
}

void JudgeLine::Update()
{
}

void JudgeLine::Draw()
{
	NoteManager *noteManager = Manager::GetGameObject<NoteManager>();
	if (!noteManager) return;

	m_play.height = UPPER_HEIGHT * noteManager->GetPlayLayerAnim(); // 今いる面(上下する)

	// ----- 共通の設定(最初に 1 回) -----
	Renderer::ClearDepth(); // 前のまま

	// カメラ(3D のビュー・射影)
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>()) camera->Draw();

	// 入力レイアウト・シェーダー設定
	m_Shader->Set();

	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	Renderer::SetDepthEnable(false);

	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	MATERIAL material{};
	material.TextureEnable = true;//true:テクスチャ使用　false:テクスチャ使用しない

	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_WhiteTex);

	// ----- 1. 判定ライン(上段の固定と、今いる面) -----
	ScaleMatrix = XMMatrixScaling((float)LANE_COUNT * m_play.laneWidth, 6.0f, 1.0f);
	RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
	TransMatrix = XMMatrixTranslation(0.0f, m_play.height, 0.0f);
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
	Renderer::SetWorldMatrix(WorldMatrix);

	material.Diffuse = XMFLOAT4(1.0f, 0.78f, 0.96f, 1.0f);// ピンク
	Renderer::SetMaterial(material);

	Renderer::GetDeviceContext()->Draw(4, 0);

	// ----- 3. アークのカーソル(今いる面の判定ラインの上) -----
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_CursorTex);

	ScaleMatrix = XMMatrixScaling(m_play.laneWidth*0.6f,m_play.laneWidth*0.6f, 1.0f);
	TransMatrix = XMMatrixTranslation((noteManager->GetCursorX() - (float)LANE_COUNT * 0.5f) * m_play.laneWidth, m_play.height, 0.0f);
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
	Renderer::SetWorldMatrix(WorldMatrix);

	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Renderer::SetMaterial(material);

	Renderer::GetDeviceContext()->Draw(4, 0);

	Renderer::SetDepthEnable(true);// 元に戻す
}