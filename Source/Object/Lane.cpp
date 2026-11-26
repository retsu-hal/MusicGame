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

// キーの文字の画像と幅(高さはどれも 41)
static const WCHAR* KEY_FILE[LANE_COUNT] = {
	L"asset\\texture\\02_Play\\text\\lane_key\\key_a.png",
	L"asset\\texture\\02_Play\\text\\lane_key\\key_s.png",
	L"asset\\texture\\02_Play\\text\\lane_key\\key_d.png",
	L"asset\\texture\\02_Play\\text\\lane_key\\key_f.png",
	L"asset\\texture\\02_Play\\text\\lane_key\\key_space.png" };
static const float KEY_W[LANE_COUNT] = { 26.0f, 26.0f, 27.0f, 26.0f, 81.0f };
static constexpr float KEY_Z = -50.0f; // キー枠の真ん中の奥行き

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
	m_NeonShader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\LaneLinePS.cso");

	// テクスチャ読み込み
	m_WhiteTex = TextureManager::Load(L"asset\\texture\\00_Common\\icon\\white_square.png");
	m_LaneTex = TextureManager::Load(L"asset\\texture\\02_Play\\lane.png");

	for (int i = 0; i < LANE_COUNT; i++)
		m_KeyTex[i] = TextureManager::Load(KEY_FILE[i]);

	m_upperrow.height = UPPER_HEIGHT;
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

	m_bottomrow.height = UPPER_HEIGHT * anim; // 今いる面(上下する)

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

	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	MATERIAL material{};

		// ----- 1. 床(lane.png) -----
	for (int i = 0; i < LANE_COUNT; i++)
	{
		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_LaneTex);

		ScaleMatrix = XMMatrixScaling(Layout::laneWidth, Z_FAR - Z_NEAR, 1.0f);
		RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
		TransMatrix = XMMatrixTranslation((i + 0.5f - LANE_COUNT * 0.5f) * m_bottomrow.laneWidth, m_bottomrow.height, (Z_FAR + Z_NEAR) * 0.5f);
		
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.95f);
		material.TextureEnable = true; //ture: テクスチャ使用する　false:テクスチャ使用しない
		Renderer::SetMaterial(material);

		Renderer::GetDeviceContext()->Draw(4, 0);

		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_WhiteTex);
	}

		// ----- 2. 押しているレーンの光 -----
		ScaleMatrix = XMMatrixScaling(m_bottomrow.laneWidth, Z_FAR - Z_NEAR, 1.0f);
		for (int i = 0; i < LANE_COUNT; i++)
		{
			if (m_Glow[i] <= 0.0f)
				continue;

			TransMatrix = XMMatrixTranslation((i + 0.5f - LANE_COUNT * 0.5f) * m_bottomrow.laneWidth, m_bottomrow.height, (Z_FAR + Z_NEAR) * 0.5f);			
			WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
			Renderer::SetWorldMatrix(WorldMatrix);

			material.Diffuse = XMFLOAT4(0.59f, 0.47f, 1.0f, 0.45f * m_Glow[i]);
			Renderer::SetMaterial(material);

			Renderer::GetDeviceContext()->Draw(4, 0);
		}

		// ----- 3. 拍の線 -----
		int beat = (int)std::floor(chart.SecToTick(now) / TICKS_PER_BEAT);
		for (int k = 0; k < 64; k++, beat++) // 念のため本数に上限を付ける
		{
			float z = ((float)chart.TickToSec(beat * TICKS_PER_BEAT) - now) * speed;
			if (z > Z_FAR)break;
			if (z < 0.0f)continue;
			bool bar = (beat % 4 == 0);
			float fade = 1.0f - z / Z_FAR;

			// 横いっぱいの細い板。
			ScaleMatrix = XMMatrixScaling(LANE_COUNT * m_bottomrow.laneWidth, bar ? 50.0f : 35.0f, 1.0f);
			TransMatrix = XMMatrixTranslation(0.0f, m_bottomrow.height, z);
			WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
			Renderer::SetWorldMatrix(WorldMatrix);

			material.Diffuse = XMFLOAT4(0.67f, 0.59f, 1.0f, (bar ? 0.35f : 0.12f) * fade); // (170, 150, 255)
			Renderer::SetMaterial(material);

			Renderer::GetDeviceContext()->Draw(4, 0);
		}

		// ----- 両端のネオン(laneNeonPS。加算) -----
		m_NeonShader->Set();
		Renderer::SetAddEnable(true);

		
		ScaleMatrix = XMMatrixScaling(LANE_COUNT * m_bottomrow.laneWidth, Z_FAR - Z_NEAR, 1.0f);
		TransMatrix = XMMatrixTranslation(0.0f, m_bottomrow.height, (Z_FAR + Z_NEAR) * 0.5f);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		// シェーダーに渡す。MATERIAL の使ってない欄を借りる(ringPS と同じやり方)
		material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		material.Ambient = XMFLOAT4(m_Neon.left.x, m_Neon.left.y, m_Neon.left.z, 0.0f);     // 左の色
		material.Specular = XMFLOAT4(m_Neon.right.x, m_Neon.right.y, m_Neon.right.z, 0.0f);  // 右の色
		material.Emission = XMFLOAT4(m_Neon.falloff, m_Neon.intensity, m_Neon.core, LANE_COUNT * Layout::laneWidth);   // 太さ・強さ・芯
		Renderer::SetMaterial(material);

		Renderer::GetDeviceContext()->Draw(4, 0);

		Renderer::SetAddEnable(false);
		m_Shader->Set(); // 元のシェーダーに戻す

		// ----- 5. キーの枠。押すと濃くなる。横はレーンの 0.76、奥行き 60 -----
		for (int i = 0; i < LANE_COUNT; i++)
		{
			ScaleMatrix = XMMatrixScaling(0.76f * m_bottomrow.laneWidth, 60.0f, 1.0f);
			TransMatrix = XMMatrixTranslation((i + 0.5f - LANE_COUNT * 0.5f) * m_bottomrow.laneWidth, m_bottomrow.height, KEY_Z);
			WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
			Renderer::SetWorldMatrix(WorldMatrix);

			material.Diffuse = XMFLOAT4(0.59f, 0.47f, 1.0f, 0.10f + 0.35f * m_Glow[i]); 
			Renderer::SetMaterial(material);

			Renderer::GetDeviceContext()->Draw(4, 0);
		}

		// ----- 6. キーの文字。キー枠の真ん中に立てる。押している間だけ白く -----
		for (int i = 0; i < LANE_COUNT; i++)
		{
			// 映ったときに、画像のピクセルどおりの大きさになるように k で割る
			ScaleMatrix = XMMatrixScaling(KEY_W[i] * (CAMERA_DIST + KEY_Z) / CAMERA_DIST, 41.0f * (CAMERA_DIST + KEY_Z) / CAMERA_DIST, 1.0f);			TransMatrix = XMMatrixTranslation((i + 0.5f - LANE_COUNT * 0.5f) * m_bottomrow.laneWidth, m_bottomrow.height, KEY_Z);
			WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
			Renderer::SetWorldMatrix(WorldMatrix);

			material.Diffuse = Input::GetKeyPress(Lane::KEYS[i]) ? XMFLOAT4(0.949f, 0.941f, 0.980f, 1.0f)  // 押している #F2F0FA
				: XMFLOAT4(0.541f, 0.525f, 0.659f, 1.0f); // 押していない #8A86A8
			Renderer::SetMaterial(material);

			Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_KeyTex[i]); // キーごとに画像が違う
			Renderer::GetDeviceContext()->Draw(4, 0);
		}

		Renderer::SetDepthEnable(true); // 122行目で切った深度を戻す
}

void Lane::DrawNeon(const Layout &Layout)
{
	const float N = (float)LANE_COUNT;

	const XMFLOAT4 left(0.75f, 0.35f, 1.0f, 0.15f);	 // 左:紫 (190, 90, 255)
	const XMFLOAT4 right(1.0f, 0.27f, 0.75f, 0.15f); // 右:ピンク (255, 70, 190)
	const XMFLOAT4 core(1.0f, 1.0f, 1.0f, 0.8f);	 // 芯:白

	// 太い線から順に重ねて、にじんだ光にする(加算なので重なるほど明るい)
	for (float w : {16.0f, 8.0f, 4.0f})
	{
		DrawDepthLine(Layout, 0.0f, w, left);
		DrawDepthLine(Layout, N, w, right);
	}
	// 芯の白い線
	DrawDepthLine(Layout, 0.0f, 2.0f, core);
	DrawDepthLine(Layout, N, 2.0f, core);
}


// 奥行き方向の線(区切り線・両端)。床に寝た細長い板。手前の端から奥の端まで
void Lane::DrawDepthLine(const Layout& Layout, float laneX, float width, const XMFLOAT4& color)
{
	// マトリックス設定
	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	ScaleMatrix = XMMatrixScaling(width, Z_FAR - Z_NEAR, 1.0f);
	RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
	TransMatrix = XMMatrixTranslation((laneX - LANE_COUNT * 0.5f) * Layout.laneWidth, Layout.height, (Z_FAR + Z_NEAR) * 0.5f);
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