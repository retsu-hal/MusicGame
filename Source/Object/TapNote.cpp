#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Input.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "NoteManager.h"
#include "TapNote.h"
#include "TextureManager.h"
#include <cmath>
#include <algorithm>

static constexpr float NOTE_WIDTH = 0.92f;
static constexpr float NOTE_DEPTH = 28.0f;

// ===============================================================
// 初期化・読み込み・やり直し
// ===============================================================
void TapNote::Init()
{
	m_Layer = 2; // 床(1)と判定ライン(3)の間

	// 頂点:原点が真ん中の 1×1 の板。大きさと位置は Draw の行列で決める
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
	Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_PixelShader, "shader\\unlitTexturePS.cso");

	// テクスチャ読み込み
	m_Texture = TextureManager::Load(L"asset\\texture\\tapnote.png");
}

void TapNote::Uninit()
{
	m_vertexBuffer->Release();

	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();

	GameObject::Uninit();
}

// ===============================================================
// 判定
// ===============================================================
void TapNote::Update()
{
	if (!Conductor::IsPlaying())
		return; // ポーズ中・再開のカウント中は何もしない
	if (m_Manager->IsFinished())
		return;

	const double now = Conductor::GetSongTime();
	const int layer = m_Manager->GetPlayLayer(); // 今いる段のノーツだけ叩ける

	// 1. キーが押されたレーンを判定(押された瞬間の時刻を使う)
	for (const KeyEvent &e : Input::GetKeyEvents())
	{
		for (int lane = 0; lane < m_Layout.laneCount; lane++)
		{
			if (e.key == Lane::KEYS[lane])
				JudgeLane(lane, layer, Conductor::CounterToSongTime(e.time));
		}
	}

	// 2. 叩かれないまま BAD の幅も過ぎたノーツは MISS(段に関係なく)
	for (TapData &n : m_Notes)
	{
		if (n.judge == Judge::None && now - n.time > WINDOW_BAD)
			SetJudge(n, Judge::Miss, 0.0);
	}
}

// ===============================================================
// 描画
// ===============================================================
void TapNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;

	// ----- 全ノーツ共通の設定(最初に 1 回) -----
	// カメラ(3D のビュー・射影)
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw();

	// 入力レイアウト設定
	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

	// シェーダー設定
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

	// テクスチャ設定
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);

	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	Renderer::SetDepthEnable(false); // 前後は描く順番で決める

	// ----- ノーツ 1 個ずつ -----
	for (const TapData &n : m_Notes)
	{
		if (n.judge != Judge::None)
			continue; // 判定が済んだものは消える

		float z = ((float)n.time - now) * speed;
		if (z > Z_FAR || z < -60.0f)
			continue; // 遠すぎ・通り過ぎ

		// 奥から出てくるときにふわっと出す
		float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);

		// レーンの真ん中(レーン全体の真ん中が x = 0)、段の高さ、奥行き z
		float x = (n.lane + 0.5f - m_Layout.laneCount * 0.5f) * m_Layout.laneWidth;
		float y = (n.layer == 1) ? UPPER_HEIGHT : 0.0f;

		// マトリックス設定
		XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
		ScaleMatrix = XMMatrixScaling(m_Layout.laneWidth * NOTE_WIDTH, NOTE_DEPTH, 1.0f); // 拡大縮小
		RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);				  // 回転(床に寝かせる)
		TransMatrix = XMMatrixTranslation(x, y, z);										  // 平行移動
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;

		Renderer::SetWorldMatrix(WorldMatrix);

		// マテリアル設定(段ごとの色)
		MATERIAL material{};
		material.Diffuse = (n.layer == 1) ? XMFLOAT4(0.78f, 0.59f, 1.0f, fade)	// 上段:紫 (200, 150, 255)
										  : XMFLOAT4(0.43f, 0.88f, 1.0f, fade); // 下段:水色 (110, 225, 255)
		material.TextureEnable = true;
		Renderer::SetMaterial(material);

		// 描画
		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	Renderer::SetDepthEnable(true);
}

void TapNote::Load(const Chart &chart)
{
	m_Notes.clear();
	for (const Note &n : chart.notes)
	{
		if (n.type != 0)
			continue; // タップだけ拾う

		TapData data;
		data.time = chart.TickToSec(n.tick);
		data.lane = n.lane;
		data.layer = n.layer;
		m_Notes.push_back(data);
	}
}

void TapNote::Reset(double startSec)
{
	for (TapData &n : m_Notes)
		n.judge = (n.time < startSec) ? Judge::Skip : Judge::None; // 途中から再生なら、前のノーツは飛ばす
}

void TapNote::JudgeLane(int lane, int layer, double hitTime)
{
	for (TapData &n : m_Notes) // 時刻の順
	{
		if (n.judge != Judge::None)
			continue; // もう判定した
		if (n.lane != lane || n.layer != layer)
			continue; // 違うレーン・違う段

		double diff = hitTime - n.time; // +なら遅い、-なら早い
		if (diff > WINDOW_BAD)
			continue; // もう過ぎてる(このフレームで MISS になる)
		if (diff < -WINDOW_BAD)
			return; // まだ遠い。後ろのノーツはもっと遠いので終わり

		double a = std::fabs(diff);
		Judge judge;
		if (a <= WINDOW_PERFECT)
			judge = Judge::Perfect;
		else if (a <= WINDOW_GREAT)
			judge = Judge::Great;
		else if (a <= WINDOW_GOOD)
			judge = Judge::Good;
		else
			judge = Judge::Bad;

		SetJudge(n, judge, diff);
		return; // 1回押したら1個だけ
	}
}

void TapNote::SetJudge(TapData &note, Judge judge, double diff)
{
	note.judge = judge;
	m_Manager->OnJudge(judge, note.lane, diff);
}
