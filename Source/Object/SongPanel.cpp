/*==============================================================================

[SongPanel.cpp]
														Author :Watanabe Retsu
														Date   :2026/11/25
--------------------------------------------------------------------------------
・プレイ画面の曲情報(ジャケット・曲名・難易度)
==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Renderer.h"
#include "Manager.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "GameSession.h"
#include "SongList.h"
#include "SongPanel.h"

//==============================================================================
//マクロ宣言
//==============================================================================
#define PANEL_X      1456.0f    // パネルの左上(画面)
#define PANEL_Y      196.0f     // ③ の達成率パネル(44〜184)の下
#define PANEL_W      420.0f     // パネルの大きさ
#define PANEL_H      104.0f

#define JACKET_SIZE  80.0f      // ジャケット(正方形)
#define JACKET_X     (PANEL_W - 12.0f - JACKET_SIZE)   // 右から 12 の余白
#define JACKET_Y     12.0f

#define TEXT_RIGHT   (JACKET_X - 16.0f)   // 文字の右端(ジャケットの左 16)
#define TITLE_H      40.0f      // 曲名の高さ(幅は画像の比率から出す)
#define TITLE_Y      10.0f
#define DIFF_H       34.0f      // 「EASY」の高さ
#define DIFF_Y       60.0f
#define LABEL_H      22.0f      // 「DIFFICULTY」の高さ
#define LABEL_Y      66.0f
#define LABEL_GAP    10.0f      // 「DIFFICULTY」と「EASY」のすき間

//==============================================================================
//グローバル変数
//==============================================================================
// 難易度の文字(DIFFICULTY の順)。画像の元の大きさも一緒に持つ
static const wchar_t* DIFF_FILE[DIFFICULTY_MAX] =
{
	L"asset\\texture\\02_Play\\text\\easy.png",
	L"asset\\texture\\02_Play\\text\\hard.png",
};
static const float DIFF_SRC_W[DIFFICULTY_MAX] = { 117.0f, 126.0f };
static const float DIFF_SRC_H[DIFFICULTY_MAX] = { 60.0f, 62.0f };

//==============================================================================
//初期化処理
//==============================================================================
void SongPanel::Init()
{
	m_Layer = 3;
	m_Position = Vector3(PANEL_X, PANEL_Y, 0.0f);

	// 曲の番号が範囲外なら何も出さない(曲一覧を読まずにプレイ画面から始めたときなど)
	if (GameSession::songIndex < 0 || GameSession::songIndex >= (int)SongList::songs.size())
	{
		m_Visible = false;
		return;
	}
	m_Visible = true;

	const SongInfo& song = SongList::songs[GameSession::songIndex];
	int diff = GameSession::difficulty;
	m_ShowDiff = !song.tutorial;   // ← 11/25 追加。チュートリアルは難易度を出さない

	// ---- 四角ごとの位置と大きさ(パネルの左上が 0,0) ----
	float x[QUAD_NUM], y[QUAD_NUM], w[QUAD_NUM], h[QUAD_NUM];

	// 枠:パネルいっぱいに引き延ばす
	x[QUAD_FRAME] = 0.0f;
	y[QUAD_FRAME] = 0.0f;
	w[QUAD_FRAME] = PANEL_W;
	h[QUAD_FRAME] = PANEL_H;

	// ジャケット
	x[QUAD_JACKET] = JACKET_X;
	y[QUAD_JACKET] = JACKET_Y;
	w[QUAD_JACKET] = JACKET_SIZE;
	h[QUAD_JACKET] = JACKET_SIZE;

	// 曲名:高さを決めて、幅は画像の比率から。右そろえ
	w[QUAD_TITLE] = song.titleWidth * (TITLE_H / song.titleHeight);
	h[QUAD_TITLE] = TITLE_H;
	x[QUAD_TITLE] = TEXT_RIGHT - w[QUAD_TITLE];
	y[QUAD_TITLE] = TITLE_Y;

	// 「EASY」「HARD」:曲名と同じく、高さを決めて右そろえ
	w[QUAD_DIFF] = DIFF_SRC_W[diff] * (DIFF_H / DIFF_SRC_H[diff]);
	h[QUAD_DIFF] = DIFF_H;
	x[QUAD_DIFF] = TEXT_RIGHT - w[QUAD_DIFF];
	y[QUAD_DIFF] = DIFF_Y;

	// 「DIFFICULTY」:「EASY」の左にすき間をあけて置く(元は 144x36)
	w[QUAD_DIFF_LABEL] = 144.0f * (LABEL_H / 36.0f);
	h[QUAD_DIFF_LABEL] = LABEL_H;
	x[QUAD_DIFF_LABEL] = x[QUAD_DIFF] - LABEL_GAP - w[QUAD_DIFF_LABEL];
	y[QUAD_DIFF_LABEL] = LABEL_Y;

	// ---- 頂点 ----
	VERTEX_3D vertex[4 * QUAD_NUM];

	for (int i = 0; i < QUAD_NUM; i++)
	{
		vertex[i * 4 + 0].Position = XMFLOAT3(x[i], y[i], 0.0f);
		vertex[i * 4 + 0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 0].TexCoord = XMFLOAT2(0.0f, 0.0f);

		vertex[i * 4 + 1].Position = XMFLOAT3(x[i] + w[i], y[i], 0.0f);
		vertex[i * 4 + 1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 1].TexCoord = XMFLOAT2(1.0f, 0.0f);

		vertex[i * 4 + 2].Position = XMFLOAT3(x[i], y[i] + h[i], 0.0f);
		vertex[i * 4 + 2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 2].TexCoord = XMFLOAT2(0.0f, 1.0f);

		vertex[i * 4 + 3].Position = XMFLOAT3(x[i] + w[i], y[i] + h[i], 0.0f);
		vertex[i * 4 + 3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 3].TexCoord = XMFLOAT2(1.0f, 1.0f);
	}

	// 頂点バッファ生成(中身は変わらないので DEFAULT。Draw で書き直さない)
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4 * QUAD_NUM;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

	// シェーダー読み込み
	m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");

	// テクスチャ読み込み(番号は四角の番号とそろえる)
	m_Tex[QUAD_FRAME] = TextureManager::Load(L"asset\\texture\\02_Play\\icon\\panel_frame.png");
	m_Tex[QUAD_JACKET] = TextureManager::Load(song.jacket.c_str());
	m_Tex[QUAD_TITLE] = TextureManager::Load(song.titleImage.c_str());
	m_Tex[QUAD_DIFF_LABEL] = TextureManager::Load(L"asset\\texture\\02_Play\\text\\label_difficulty.png");
	m_Tex[QUAD_DIFF] = TextureManager::Load(DIFF_FILE[diff]);
}

//==============================================================================
//終了処理
//==============================================================================
void SongPanel::Uninit()
{
	if (m_vertexBuffer) m_vertexBuffer->Release();   // 曲が無くて作ってないこともある
	GameObject::Uninit();
}

//==============================================================================
//更新処理
//==============================================================================
void SongPanel::Update()
{}

//==============================================================================
//描画処理
//==============================================================================
void SongPanel::Draw()
{
	if (!m_Visible) return;

	// 入力レイアウト・シェーダー設定
	m_Shader->Set();

	// マトリックス設定
	Renderer::SetWorldViewProjection2D();

	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
	ScaleMatrix = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	RotMatrix = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	TransMatrix = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;

	Renderer::SetWorldMatrix(WorldMatrix);

	// マテリアル設定
	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.TextureEnable = true;
	Renderer::SetMaterial(material);

	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 描画:画像をセットして 1 枚描く、を 5 回
	for (int i = 0; i < QUAD_NUM; i++)
	{
		// チュートリアルは難易度の行(DIFFICULTY と EASY)を飛ばす  ← 11/25 追加
		if (!m_ShowDiff && (i == QUAD_DIFF_LABEL || i == QUAD_DIFF))
			continue;

		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Tex[i]);
		Renderer::GetDeviceContext()->Draw(4, i * 4);
	}
}