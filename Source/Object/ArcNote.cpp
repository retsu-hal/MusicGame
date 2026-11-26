#include "main.h"
#include "Manager.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "TextureManager.h"
#include "ArcNote.h"
#include "Judge.h"
#include "NoteManager.h"
#include "Renderer.h"
#include "Camera.h"
#include "ShaderManager.h"
#include <algorithm>
#include <cmath>

static constexpr int TICK_STEP = TICKS_PER_BEAT / 2; // 中継点の間隔(8分音符)
static constexpr float ON_WIDTH = 0.5f;				 // 線の上とみなす幅(中心から。レーン単位)★

// t(0〜1、区間のどこまで進んだか)→ 横にどこまで動いたか(0〜1)
static float Ease(float t, int ease)
{
	switch (ease)
	{
	case 1:
		return 1.0f - std::cos(t * XM_PIDIV2); // だんだん速く(最初ゆっくり)
	case 2:
		return std::sin(t * XM_PIDIV2); // だんだん遅く(最初に一気に)
	case 3:
		return 0.5f - 0.5f * std::cos(t * XM_PI); // S字(両端ゆっくり)
	default:
		return t; // まっすぐ
	}
}

// ===============================================================
// 初期化・読み込み
// ===============================================================
void ArcNote::Init()
{
	m_Layer = 2;

	m_BandTex = TextureManager::Load(L"asset\\texture\\02_Play\\icon\\arc_band.png");
	m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");

	{
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(VERTEX_3D) * BAND_MAX_POINTS * 2;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_vertexBuffer);
	}

	// 頂点:原点が真ん中の 1×1 の板。大きさと位置は Draw の行列で決める
	{
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

		// 頂点バッファ生成(TapNote と同じ。入れる先だけ m_HeadVertexBuffer)
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(VERTEX_3D) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = vertex;

		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_HeadVertexBuffer);
	}

	m_HeadTex = TextureManager::Load(L"asset\\texture\\02_Play\\icon\\arc_head.png");
}

void ArcNote::Uninit()
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
	if (m_HeadVertexBuffer)
	{
		m_HeadVertexBuffer->Release();
		m_HeadVertexBuffer = nullptr;
	}
	GameObject::Uninit();
}

void ArcNote::Update()
{
	if (!Conductor::IsPlaying())
		return; // ポーズ中・再開のカウント中は何もしない

	const double now = Conductor::GetSongTime();
	const float cursorX = m_Manager->GetCursorX();

	for (ArcData &a : m_Arcs)
	{
		// 今乗っているか(見た目用。アークの途中のときだけ)
		bool inside = (now >= a.checks.front() && now <= a.checks.back());
		a.OnCursor = inside && std::fabs(cursorX - XAt(a, now)) <= ON_WIDTH;

		// 時刻を過ぎた判定を、その瞬間に乗っているかで決める
		while (a.nextCheck < (int)a.checks.size() && now >= a.checks[a.nextCheck])
		{
			double t = a.checks[a.nextCheck];
			bool ok = std::fabs(cursorX - XAt(a, t)) <= ON_WIDTH;

			int lane = std::clamp((int)XAt(a, t), 0, LANE_COUNT - 1); // 光らせるレーン(x の下のレーン)
			m_Manager->OnJudge(ok ? Judge::Perfect : Judge::Miss, lane, 0.0, 0);
			a.nextCheck++;
		}
	}
}

void ArcNote::Draw()
{
	const float now = (float)Conductor::GetSongTime();

	// ----- 1. 帯(頂点を自分で並べる) -----
	if (CAMERA *camera = Manager::GetGameObject<CAMERA>())
		camera->Draw(); // ビュー・射影をセット。ワールドは単位行列になる

	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.TextureEnable = true;//true:テクスチャ使用　false:テクスチャ使用しない
	Renderer::SetMaterial(material);

	m_Shader->Set();
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_BandTex);
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Renderer::SetDepthEnable(false); // 前後は描く順番で決める


	for (const ArcData &arc : m_Arcs)
	{
		m_Layout.height = (arc.layer == 1) ? UPPER_HEIGHT : 0.0f;

		D3D11_MAPPED_SUBRESOURCE msr;
		Renderer::GetDeviceContext()->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		VERTEX_3D *vertex = (VERTEX_3D *)msr.pData;
		int count = 0; // 並べた点の数

		for (size_t i = 0; i < arc.samples.size(); i++)
		{
			const ArcSample &s = arc.samples[i];
			float z = ((float)s.time - now) * Settings::noteSpeed;

			// 判定ラインを過ぎた点は使わない
			if (z < 0.0f)
				continue;
			if (count >= BAND_MAX_POINTS - 1)
				break; // バッファがいっぱい(先頭に1点足すぶん空けておく)

			// アークが始まってる(手前に捨てた点がある)なら、
			// 最初に判定ライン上の点を1つ足す
			if (count == 0 && i > 0)
			{
				vertex[0].Position = m_Layout.World(XAt(arc, now) + 0.3f, 0.0f);
				vertex[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
				vertex[0].Diffuse = XMFLOAT4 (1.0f, 1.0f, 1.0f, 1.0f);
				vertex[0].TexCoord = XMFLOAT2(1.0f, 0.5f);

				vertex[1].Position = m_Layout.World(XAt(arc, now) - 0.3f, 0.0f);
				vertex[1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
				vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				vertex[1].TexCoord = XMFLOAT2(0.0f, 0.5f);

				count++;
			}

			// 奥ほど薄く(TapNote と同じ式)
			float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);

			// 右 → 左の順(時計回りが表)。画像は真ん中の1行だけ貼る(帯の「断面」を引き伸ばす)
			vertex[count * 2 + 0].Position = m_Layout.World(s.x + 0.3f, z);
			vertex[count * 2 + 0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			vertex[count * 2 + 0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, fade);
			vertex[count * 2 + 0].TexCoord = XMFLOAT2(1.0f, 0.5f);

			vertex[count * 2 + 1].Position = m_Layout.World(s.x - 0.3f, z);
			vertex[count * 2 + 1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			vertex[count * 2 + 1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, fade);
			vertex[count * 2 + 1].TexCoord = XMFLOAT2(0.0f, 0.5f);
			count++;

			if (z > Z_FAR)
				break; // この点で奥の端を超えた。ここから先は全部まだ遠い
		}
		Renderer::GetDeviceContext()->Unmap(m_vertexBuffer, 0);

		if (count >= 2)
			Renderer::GetDeviceContext()->Draw(count * 2, 0); // 2点(4頂点)以上あれば帯になる
	}

	// ----- 2. 判定ラインのところの印。乗っていると明るい -----
	// 頂点バッファ設定(印の板に切りかえる)
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_HeadVertexBuffer, &stride, &offset);

	// テクスチャ設定
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_HeadTex);

	// 大きさと回転は全部同じ(0.8 レーンの正方形、床に寝かせる)
	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	ScaleMatrix = XMMatrixScaling(0.8f * m_Layout.laneWidth, 0.8f * m_Layout.laneWidth, 1.0f);
	RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);

	for (const ArcData& arc : m_Arcs)
	{
		if (now < arc.samples.front().time || now > arc.samples.back().time)
			continue;
		if (!arc.OnCursor)
			continue; // 乗ってないときは印を出さない

		m_Layout.height = (arc.layer == 1) ? UPPER_HEIGHT : 0.0f;
		TransMatrix = XMMatrixTranslation((XAt(arc, now) - LANE_COUNT * 0.5f) * m_Layout.laneWidth, m_Layout.height, 0.0f);
		WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
		Renderer::SetWorldMatrix(WorldMatrix);

		// マテリアル設定(ここに来るのは乗ってるときだけなので、いつも不透明)
		MATERIAL material{};
		material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		material.TextureEnable = true;
		Renderer::SetMaterial(material);

		// 描画
		Renderer::GetDeviceContext()->Draw(4, 0);
	}

	Renderer::SetDepthEnable(true);
}

void ArcNote::Load(const Chart &chart)
{
	m_Arcs.clear();
	for (const ArcLine &arc : chart.arcs)
	{
		ArcData data;
		data.layer = arc.layer;

		// 点と点のあいだ(区間)ごとに、DIVISION 個に区切る
		for (size_t i = 0; i + 1 < arc.points.size(); i++)
		{
			const ArcPoint &a = arc.points[i];
			const ArcPoint &b = arc.points[i + 1];

			for (int k = 0; k < DIVISION; k++)
			{
				float t = (float)k / DIVISION; //floatにしないと曲線が直線になるから
				int tick = a.tick + (int)std::lround((b.tick - a.tick) * t);

				ArcSample s;
				s.time = chart.TickToSec(tick);
				s.x = a.x + (b.x - a.x) * Ease(t, a.ease);
				data.samples.push_back(s);
			}
		}

		// 最後の点(上のループは「次の点の手前まで」なので、終点だけ足りない)
		const ArcPoint &last = arc.points.back();
		data.samples.push_back({chart.TickToSec(last.tick), last.x});

		// 判定する時刻:始点・8分ごとの中継点・終点
		int startTick = arc.points.front().tick;
		int endTick = arc.points.back().tick;
		data.checks.push_back(chart.TickToSec(startTick));
		for (int t = startTick + TICK_STEP; t < endTick; t += TICK_STEP)
			data.checks.push_back(chart.TickToSec(t));
		data.checks.push_back(chart.TickToSec(endTick));

		m_Arcs.push_back(data);
	}
}

double ArcNote::GetLastTime() const
{
	double last = 0.0;
	for (const ArcData &a : m_Arcs)
		last = std::max(last, a.samples.back().time);
	return last;
}

void ArcNote::Reset(double startSec)
{
	for (ArcData &a : m_Arcs)
	{
		a.OnCursor = false;
		// 途中から再生なら、始まっているアークは丸ごと飛ばす(ロングと同じ)
		a.nextCheck = (a.checks.front() < startSec) ? (int)a.checks.size() : 0;
	}
}

int ArcNote::GetJudgeCount() const
{
	int count = 0;
	for (const ArcData &a : m_Arcs)
		count += (int)a.checks.size(); // 始点 + 中継点 + 終点
	return count;
}

// その時刻のアークの x。始まる前は始点の x、終わったあとは終点の x
float ArcNote::XAt(const ArcData &arc, double time)
{
	const std::vector<ArcSample> &s = arc.samples;
	if (time <= s.front().time)
		return s.front().x;

	for (size_t i = 0; i + 1 < s.size(); i++)
	{
		if (time > s[i + 1].time)
			continue; // まだ先

		// s[i] と s[i+1] のあいだ → 割合で混ぜる
		double span = s[i + 1].time - s[i].time;
		float t = (span > 0.0) ? (float)((time - s[i].time) / span) : 1.0f;
		return s[i].x + (s[i + 1].x - s[i].x) * t;
	}
	return s.back().x;
}