#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Input.h"
#include "Lane.h"
#include "Chart.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "NoteManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include <cmath>
#include <algorithm>

// 奥行き方向の線(区切り線・両端)。床に寝た細長い板。手前の端から奥の端まで
void Lane::DrawDepthLine(const Layout &L, float laneX, float width, const XMFLOAT4 &color)
{
	// マトリックス設定
	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	ScaleMatrix = XMMatrixScaling(width, Z_FAR - Z_NEAR, 1.0f);
	RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
	TransMatrix = XMMatrixTranslation((laneX - L.laneCount * 0.5f) * L.laneWidth, L.height, (Z_FAR + Z_NEAR) * 0.5f);
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
	Renderer::SetWorldMatrix(WorldMatrix);

	// マテリアル設定
	MATERIAL material{};
	material.Diffuse = color;
	material.TextureEnable = true;
	Renderer::SetMaterial(material);

	// 描画(画像は呼ぶ前に white をセットしておく)
	Renderer::GetDeviceContext()->Draw(4, 0);
}

// ===============================================================
// Lane(床)
// ===============================================================
void Lane::Init()
{
	m_Layer = 1; // ノーツより奥

	// 頂点:原点が真ん中の 1×1 の板。
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
	m_LaneTex = TextureManager::Load(L"asset\\texture\\lane.png");

	m_upper.height = UPPER_HEIGHT;
}

void Lane::Uninit()
{
	m_vertexBuffer->Release();

	GameObject::Uninit();
}

void Lane::Update()
{
	float dt = Manager::GetDeltaTime();

	// 押したキーのレーンを光らせる（徐々に暗くする）
	for (int i = 0; i < LANE_COUNT; i++)
	{
		if (Input::GetKeyPress(Lane::KEYS[i]))
			m_Glow[i] = 1.0f;
		else
			m_Glow[i] = std::max(0.0f, m_Glow[i] - dt * 6.0f);
	}
}

void Lane::Draw()
{
	// 拍の線を引くのに譜面がいる。NoteManager がいなければ何も描かない
	NoteManager *noteManager = Manager::GetGameObject<NoteManager>();
	if (!noteManager)
		return;

	const Chart &chart = noteManager->GetChart();
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;
	const float anim = noteManager->GetPlayLayerAnim(); // 0 = 下段、1 = 上段

	m_play.height = UPPER_HEIGHT * anim; // 今いる面(上下する)

	// ----- 共通の設定(最初に 1 回) -----
	Renderer::ClearDepth(); // 背景(2D)が書いた深度を消す(前のまま)

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

	Renderer::SetDepthEnable(false); // 奥から順に描くので、深度は使わない

	// ----- 床 -----
	// 上段を先、今いる面を後に描く。後のほうが手前に重なるので、今いる面が隠れない
	DrawFloor(m_upper, chart, now, speed, 0.0f);
	DrawFloor(m_play, chart, now, speed, 1.0f);

	// ----- 両端のネオン(加算) -----
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_WhiteTex);
	Renderer::SetAddEnable(true);
	DrawNeon(m_upper);
	DrawNeon(m_play);
	Renderer::SetAddEnable(false);

	Renderer::SetDepthEnable(true);
}

void Lane::DrawFloor(const Layout &L, const Chart &chart, float now, float speed, float active)
{
	const float N = (float)L.laneCount;
	const float lw = L.laneWidth;

	// 全部床に寝かせる(回転は全部同じなので 1 回だけ作る)
	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
	MATERIAL material{};
	material.TextureEnable = true;

	// ----- 1. 床(lane.png)。今いる面は濃く(0.95)、上段の枠は薄く(0.2) -----
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_LaneTex);

	ScaleMatrix = XMMatrixScaling(N * lw, Z_FAR - Z_NEAR, 1.0f);
	TransMatrix = XMMatrixTranslation(0.0f, L.height, (Z_FAR + Z_NEAR) * 0.5f);
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
	Renderer::SetWorldMatrix(WorldMatrix);

	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.2f + 0.75f * active);
	Renderer::SetMaterial(material);

	Renderer::GetDeviceContext()->Draw(4, 0);

	// ここから先は全部、真っ白な画像に色を付ける
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_WhiteTex);

	// ----- 2. 押しているレーンの光(今いる面だけ)。判定ライン(0)から Z_FAR × 0.6 まで -----
	ScaleMatrix = XMMatrixScaling(lw, Z_FAR * 0.6f, 1.0f);
	for (int i = 0; i < L.laneCount; i++)
	{
		float g = m_Glow[i] * active;
		if (g <= 0.0f)
			continue;

		TransMatrix = XMMatrixTranslation((i + 0.5f - N * 0.5f) * lw, L.height, Z_FAR * 0.3f);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		material.Diffuse = XMFLOAT4(0.59f, 0.47f, 1.0f, 0.45f * g); // (150, 120, 255) ★
		Renderer::SetMaterial(material);

		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	// ----- 3. 拍の線 -----
	int beat = (int)std::floor(chart.SecToTick(now) / TICKS_PER_BEAT);
	for (int k = 0; k < 64; k++, beat++) // 念のため本数に上限を付ける
	{
		float z = ((float)chart.TickToSec(beat * TICKS_PER_BEAT) - now) * speed;
		if (z > Z_FAR)
			break;
		if (z < 0.0f)
			continue;
		bool bar = (beat % 4 == 0);
		float fade = 1.0f - z / Z_FAR;

		// 横いっぱいの細い板。太さは小節の頭 4、それ以外 2
		ScaleMatrix = XMMatrixScaling(N * lw, bar ? 4.0f : 2.0f, 1.0f);
		TransMatrix = XMMatrixTranslation(0.0f, L.height, z);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		material.Diffuse = XMFLOAT4(0.67f, 0.59f, 1.0f, (bar ? 0.35f : 0.12f) * fade); // (170, 150, 255)
		Renderer::SetMaterial(material);

		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	// ----- 4. 区切り線(今いる面だけ) -----
	if (active > 0.0f)
	{
		const XMFLOAT4 divider(0.63f, 0.55f, 0.90f, 0.25f); // (160, 140, 230)
		for (int i = 1; i < L.laneCount; i++)
			DrawDepthLine(L, (float)i, 1.5f, divider);
	}
}

void Lane::DrawNeon(const Layout &L)
{
	const float N = (float)L.laneCount;

	const XMFLOAT4 left(0.75f, 0.35f, 1.0f, 0.15f);	 // 左:紫 (190, 90, 255)
	const XMFLOAT4 right(1.0f, 0.27f, 0.75f, 0.15f); // 右:ピンク (255, 70, 190)
	const XMFLOAT4 core(1.0f, 1.0f, 1.0f, 0.8f);	 // 芯:白

	// 太い線から順に重ねて、にじんだ光にする(加算なので重なるほど明るい)
	for (float w : {16.0f, 8.0f, 4.0f})
	{
		DrawDepthLine(L, 0.0f, w, left);
		DrawDepthLine(L, N, w, right);
	}
	// 芯の白い線
	DrawDepthLine(L, 0.0f, 2.0f, core);
	DrawDepthLine(L, N, 2.0f, core);
}
