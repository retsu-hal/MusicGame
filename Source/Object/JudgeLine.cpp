#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Input.h"
#include "Camera.h"
#include "NoteManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "JudgeLine.h"
#include <algorithm>

// キーの文字の画像と幅(高さはどれも 41)
static const WCHAR *KEY_FILE[LANE_COUNT] = {
	L"asset\\texture\\ui\\text\\lane_key\\a.png",
	L"asset\\texture\\ui\\text\\lane_key\\s.png",
	L"asset\\texture\\ui\\text\\lane_key\\d.png",
	L"asset\\texture\\ui\\text\\lane_key\\f.png",
	L"asset\\texture\\ui\\text\\lane_key\\space.png"};
static const float KEY_W[LANE_COUNT] = {26.0f, 26.0f, 27.0f, 26.0f, 81.0f};
static constexpr float KEY_Z = -50.0f; // キー枠の真ん中の奥行き

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
	m_WhiteTex = TextureManager::Load(L"asset\\texture\\ui\\assets\\white_square.png");
	m_CursorTex = TextureManager::Load(L"asset\\texture\\ui\\assets\\cursor.png");
	for (int i = 0; i < LANE_COUNT; i++)
		m_KeyTex[i] = TextureManager::Load(KEY_FILE[i]);

	m_upper.height = UPPER_HEIGHT; // 上段は動かないので、最初に1回だけ
}

void JudgeLine::Uninit()
{
	m_vertexBuffer->Release();

	GameObject::Uninit();
}

void JudgeLine::Update()
{
	float dt = Manager::GetDeltaTime();

	// 押したキーの枠を濃くする(徐々に戻す)
	for (int i = 0; i < LANE_COUNT; i++)
	{
		if (Input::GetKeyPress(Lane::KEYS[i]))
			m_Glow[i] = 1.0f;
		else
			m_Glow[i] = std::max(0.0f, m_Glow[i] - dt * 6.0f);
	}
}

void JudgeLine::Draw()
{
	NoteManager *noteManager = Manager::GetGameObject<NoteManager>();
	if (!noteManager)
		return;

	m_play.height = UPPER_HEIGHT * noteManager->GetPlayLayerAnim(); // 今いる面(上下する)

	const float N = (float)LANE_COUNT;
	const float lw = m_play.laneWidth;

	// ----- 共通の設定(最初に 1 回) -----
	Renderer::ClearDepth(); // 前のまま

	// カメラ(3D のビュー・射影)
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw();

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
	material.TextureEnable = true;

	RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_WhiteTex);

	// ----- 1. 判定ライン(上段の固定と、今いる面)。太さ 6 -----
	ScaleMatrix = XMMatrixScaling(N * lw, 6.0f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 0.78f, 0.96f, 1.0f); // (255, 200, 245)
	Renderer::SetMaterial(material);
	for (const Layout &L : {m_upper, m_play})
	{
		TransMatrix = XMMatrixTranslation(0.0f, L.height, 0.0f);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	// ----- 2. キーの枠(今いる面)。押すと濃くなる。横はレーンの 0.12〜0.88、奥行きは -20〜-80 -----
	ScaleMatrix = XMMatrixScaling(0.76f * lw, 60.0f, 1.0f);
	for (int i = 0; i < LANE_COUNT; i++)
	{
		TransMatrix = XMMatrixTranslation((i + 0.5f - N * 0.5f) * lw, m_play.height, KEY_Z);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		material.Diffuse = XMFLOAT4(0.59f, 0.47f, 1.0f, 0.10f + 0.35f * m_Glow[i]); // (150, 120, 255)
		Renderer::SetMaterial(material);

		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	// ----- 3. アークのカーソル(今いる面の判定ラインの上)。0.6 レーンの正方形 -----
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_CursorTex);

	const float x = noteManager->GetCursorX();
	ScaleMatrix = XMMatrixScaling(0.6f * lw, 0.6f * lw, 1.0f);
	TransMatrix = XMMatrixTranslation((x - N * 0.5f) * lw, m_play.height, 0.0f);
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
	Renderer::SetWorldMatrix(WorldMatrix);

	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Renderer::SetMaterial(material);

	Renderer::GetDeviceContext()->Draw(4, 0);

	// ----- 4. キーの文字。キー枠の真ん中に立てる。押している間だけ白く -----
	RotMatrix = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f); // 立てる
	const float k = CAMERA_DIST / (CAMERA_DIST + KEY_Z);		// 奥行き -50 だと、この倍だけ大きく映る
	for (int i = 0; i < LANE_COUNT; i++)
	{
		// 映ったときに、画像のピクセルどおりの大きさになるように k で割る
		ScaleMatrix = XMMatrixScaling(KEY_W[i] / k, 41.0f / k, 1.0f);
		TransMatrix = XMMatrixTranslation((i + 0.5f - N * 0.5f) * lw, m_play.height, KEY_Z);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		material.Diffuse = Input::GetKeyPress(Lane::KEYS[i]) ? XMFLOAT4(0.949f, 0.941f, 0.980f, 1.0f)  // 押している #F2F0FA
															 : XMFLOAT4(0.541f, 0.525f, 0.659f, 1.0f); // 押していない #8A86A8
		Renderer::SetMaterial(material);

		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_KeyTex[i]); // キーごとに画像が違う
		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	Renderer::SetDepthEnable(true);
}