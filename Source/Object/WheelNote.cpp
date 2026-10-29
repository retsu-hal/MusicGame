#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "WheelNote.h"
#include "Input.h"
#include "NoteManager.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include <algorithm>
#include <cmath>

static constexpr float ARROW_SIZE = 120.0f; // 矢印の大きさ
static constexpr float ARROW_GAP = 10.0f;	// バーと矢印のすき間

// ===============================================================
// 初期化・読み込み
// ===============================================================
void WheelNote::Init()
{
	m_Layer = 2; // 床(1)と判定ライン(3)の間

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
	m_UpBarTex = TextureManager::Load(L"asset\\texture\\flick_wheel_up.png");
	m_DownBarTex = TextureManager::Load(L"asset\\texture\\flick_wheel_down.png");
	m_UpArrowTex = TextureManager::Load(L"asset\\texture\\wheel_arrow_up.png");
	m_DownArrowTex = TextureManager::Load(L"asset\\texture\\wheel_arrow_down.png");
}

void WheelNote::Uninit()
{
	m_vertexBuffer->Release();

	GameObject::Uninit();
}

void WheelNote::Update()
{
	if (!Conductor::IsPlaying())
		return; // ポーズ中・再開のカウント中は何もしない
	const double now = Conductor::GetSongTime();
	const int wheel = Input::GetMouseWheel();

	// 1. 回したら、その向きのノーツを1個だけ取る
	if (wheel != 0)
	{
		const int dir = (wheel > 0) ? 1 : -1; // 回した向き

		for (WheelData &n : m_Notes) // 時刻の順
		{
			if (n.judge != Judge::None)
				continue; // もう判定した

			double diff = now - n.time;
			if (diff > WINDOW_BAD)
				continue; // もう過ぎてる(このあと MISS になる)
			if (diff < -WINDOW_BAD)
				break; // ここから先は全部まだ遠い

			if (n.dir != dir)
				continue; // 向きが違う → 次のノーツを見る

			// タイミングはタップと同じ(ずれで PERFECT〜BAD。10/28 変更)
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
			break; // 1フレームで1個だけ
		}
	}

	// 2. 回されないまま BAD の幅も過ぎたノーツは MISS
	for (WheelData &n : m_Notes)
	{
		if (n.judge == Judge::None && now - n.time > WINDOW_BAD)
			SetJudge(n, Judge::Miss, 0.0);
	}
}

void WheelNote::SetJudge(WheelData &note, Judge judge, double diff)
{
	note.judge = judge;
	m_Manager->OnJudge(judge, 0, diff, LANE_COUNT);
}

// ===============================================================
// 描画
// ===============================================================
void WheelNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();
	const float speed = Settings::noteSpeed;

	// ----- 全ノーツ共通の設定(最初に 1 回) -----
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

	Renderer::SetDepthEnable(false); // 前後は描く順番で決める

	// ----- 向きごとに描く -----
	DrawDir(1, now, speed);	 // 上(マゼンタ)
	DrawDir(-1, now, speed); // 下(ブルー)

	Renderer::SetDepthEnable(true);
}

void WheelNote::DrawDir(int dir, float now, float speed)
{
	// 上フリックは下段にいるときに来る → 下段の高さ。下フリックはその逆
	const float height = (dir > 0) ? 0.0f : UPPER_HEIGHT;

	// バーの大きさ。幅 = レーン全体、厚み = 画像の縦横比(1440 × 128)を保つ
	const float barW = m_Layout.laneWidth * m_Layout.laneCount;
	const float barD = barW * (128.0f / 1440.0f);

	// 矢印の真ん中の高さ。上向きはバーの上に立てる、下向きはバーの下にぶら下げる
	const float arrowY = (dir > 0) ? height + ARROW_GAP + ARROW_SIZE * 0.5f : height - ARROW_GAP - ARROW_SIZE * 0.5f;

	// この向きで使う画像
	ID3D11ShaderResourceView *barTex = (dir > 0) ? m_UpBarTex : m_DownBarTex;
	ID3D11ShaderResourceView *arrowTex = (dir > 0) ? m_UpArrowTex : m_DownArrowTex;

	for (const WheelData &n : m_Notes)
	{
		if (n.dir != dir)
			continue;
		if (n.judge != Judge::None)
			continue;

		float z = ((float)n.time - now) * speed;
		if (z > Z_FAR || z < -60.0f)
			continue; // 遠すぎ・通り過ぎ

		// マテリアル設定。色は画像に入っているので白、奥からふわっと(TapNote と同じ式)
		float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);
		MATERIAL material{};
		material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, fade);
		material.TextureEnable = true;
		Renderer::SetMaterial(material);

		XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;

		// ----- バー:床に寝かせる -----
		ScaleMatrix = XMMatrixScaling(barW, barD, 1.0f);
		RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
		TransMatrix = XMMatrixTranslation(0.0f, height, z);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &barTex);
		Renderer::GetDeviceContext()->Draw(4, 0);

		// ----- 矢印:立ったまま(カメラに正面を向く) -----
		ScaleMatrix = XMMatrixScaling(ARROW_SIZE, ARROW_SIZE, 1.0f);
		RotMatrix = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
		TransMatrix = XMMatrixTranslation(0.0f, arrowY, z);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &arrowTex);
		Renderer::GetDeviceContext()->Draw(4, 0);
	}
}

void WheelNote::Load(const Chart &chart)
{
	m_Notes.clear();
	for (const Note &n : chart.notes)
	{
		if (n.type != 2)
			continue; // ホイールフリックだけ拾う
		if (n.dir == 0)
			continue; // 向きがないものは回しようがないので飛ばす

		WheelData data;
		data.time = chart.TickToSec(n.tick);
		data.dir = (n.dir > 0) ? 1 : -1; // 2 や -3 と書かれていても 1 / -1 にそろえる
		m_Notes.push_back(data);
	}
}

void WheelNote::Reset(double startSec)
{
	for (WheelData &n : m_Notes)
		n.judge = (n.time < startSec) ? Judge::Skip : Judge::None; // 途中から再生なら、前のノーツは飛ばす
}