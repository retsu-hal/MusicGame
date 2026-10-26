/*==============================================================================

[Score.cpp]
														Author :Watanabe Retsu
														Date   :2026/06/30
--------------------------------------------------------------------------------

==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Renderer.h"
#include "Manager.h"
#include "Score.h"
#include <cmath>

//==============================================================================
//マクロ宣言
//==============================================================================
#define TEXTURE_NUM_WIDTH 4      // 数字のスプライトは 4x4 マス
#define TEXTURE_NUM_HEIGHT 4
#define DIGIT_WIDTH  64.0f       // 1文字の横幅(スプライトの1マス分)
#define DIGIT_HEIGHT 64.0f       // 1文字の高さ
#define DIGIT_STEP   38.0f       // 次の文字までの送り幅
#define NARROW_STEP  18.0f       // 「.」の送り幅(細い)
#define SCORE_RIGHT  (SCREEN_WIDTH - 20.0f)   // 右端の位置
#define SCORE_TOP    40.0f

//==============================================================================
//プロトタイプ宣言
//==============================================================================

//==============================================================================
//グローバル変数
//==============================================================================
// 四角形1枚ぶんの頂点(左上・右上・左下・右下。TRIANGLESTRIP の順)
static void SetQuad(VERTEX_3D* v, float x, float y, float w, float h,
					float u, float vv, float uw, float vh)
{
	XMFLOAT3 pos[4] = { {x, y, 0.0f}, {x + w, y, 0.0f}, {x, y + h, 0.0f}, {x + w, y + h, 0.0f} };
	XMFLOAT2 uv[4] = { {u, vv}, {u + uw, vv}, {u, vv + vh}, {u + uw, vv + vh} };
	for (int i = 0; i < 4; i++)
	{
		v[i].Position = pos[i];
		v[i].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		v[i].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		v[i].TexCoord = uv[i];
	}
}
//==============================================================================
//初期化処理
//==============================================================================
void Score::Init()
{
	m_Layer = 3;

	// 位置 = 右端・上端。文字は右端(x = 0)から左へ並べる
	m_Position = Vector3(SCORE_RIGHT, SCORE_TOP, 0.0f);

	// 中身は Draw で毎回書くので、ここでは全部 0 でいい
	VERTEX_3D vertex[4 * DIGIT_NUM] = {};

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4 * DIGIT_NUM;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

	//シェーダー読み込み
	Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_PixelShader, "shader\\unlitTexturePS.cso");

	//テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\texture\\number_atlas_512.png", WIC_FLAGS_NONE, &metadata, image);	// 背景が透明な数字アトラス
	CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &m_Texture);
	assert(m_Texture);
}

//==============================================================================
//終了処理
//==============================================================================
void Score::Uninit()
{
	m_vertexBuffer->Release();

	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();
	m_Texture->Release();
}

//==============================================================================
//更新処理
//==============================================================================
void Score::Update()
{
	float dt=Manager::GetDeltaTime();

	//カウントアップ処理
	if (m_Score < m_TargetScore)
	{

		int diff = m_TargetScore - m_Score;
		int step = (diff / 10 > 1) ? diff / 10 : 1;

			m_Score += step;
			if (m_Score > m_TargetScore) m_Score = m_TargetScore;
	}

}

//==============================================================================
//描画処理
//==============================================================================
void Score::Draw()
{
	//入力レイアウト設定
	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

	//シェーダー設定
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

	//マトリックス設定
	Renderer::SetWorldViewProjection2D();

	XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix, PopScaleMatrix;
	ScaleMatrix = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	// 右端・上下中央を中心に膨らませる（画面の右からはみ出さないように）
	const float scoreW = DIGIT_STEP * (DIGIT_NUM - 1) + DIGIT_WIDTH;
	PopScaleMatrix = XMMatrixTranslation(0.0f, -DIGIT_HEIGHT * 0.5f, 0.0f)
		* XMMatrixScaling(1.0f, 1.0f, 1.0f)
		* XMMatrixTranslation(0.0f, DIGIT_HEIGHT * 0.5f, 0.0f);
	RotMatrix = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	TransMatrix = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	WorldMatrix = ScaleMatrix * PopScaleMatrix * RotMatrix * TransMatrix;

	Renderer::SetWorldMatrix(WorldMatrix);

	//マテリアル設定
	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.TextureEnable = true;
	Renderer::SetMaterial(material);

	//テクスチャ設定
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);

	//頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	//プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 表示するマスを、右から順に決める(授業と同じく % 10 で1桁ずつ取り出す)
	// 10100 → 「%」「0」「0」「.」「1」「0」「1」
	int cell[DIGIT_NUM];
	int length = 0;
	int value = m_Score;                       // 0.01% 単位

	cell[length++] = 11;                        // 「%」はスプライトの 11 番目のマス
	cell[length++] = value % 10; value /= 10;   // 小数第2位
	cell[length++] = value % 10; value /= 10;   // 小数第1位
	cell[length++] = 10;                        // 「.」はスプライトの 10 番目のマス
	do
	{
		cell[length++] = value % 10;            // 整数の部分を1桁ずつ
		value /= 10;
	} while (value > 0 && length < DIGIT_NUM);

	D3D11_MAPPED_SUBRESOURCE msr;
	Renderer::GetDeviceContext()->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

	const float sizeX = 1.0f / TEXTURE_NUM_WIDTH;
	const float sizeY = 1.0f / TEXTURE_NUM_HEIGHT;

	// 右そろえ:右端(x = 0)から左へ、1文字ずつ置いていく
	float x = 0.0f;
	for (int i = 0; i < length; i++)
	{
		int   num = cell[i];
		float step = (num == 10) ? NARROW_STEP : DIGIT_STEP;   // 「.」だけ細い
		x -= step;                                             // この文字の送り幅の左端

		// 送り幅の真ん中に、1マス分の四角形を置く
		float left = x + step * 0.5f - DIGIT_WIDTH * 0.5f;
		SetQuad(&vertex[i * 4], left, 0.0f, DIGIT_WIDTH, DIGIT_HEIGHT,
				sizeX * (num % TEXTURE_NUM_WIDTH), sizeY * (num / TEXTURE_NUM_WIDTH), sizeX, sizeY);
	}

	Renderer::GetDeviceContext()->Unmap(m_vertexBuffer, 0);

	// 描画(使う文字の数だけ)
	for (int i = 0; i < length; i++)
		Renderer::GetDeviceContext()->Draw(4, i * 4);
}

void Score::Reset()
{
	m_RateSum = 0.0;
	m_Score = 0;
	m_TargetScore = 0;
}

double Score::GetAchievement() const
{
	if (m_TotalJudges <= 0) return 0.0;
	return m_RateSum / m_TotalJudges * 101.0;   // 全部 PERFECT で 101%
}

void Score::AddScore(double rate)
{
	m_RateSum += rate;

	// 0.01% 単位の整数にする。
	// ほんの少し足してから切り捨てるのは、101.00 が計算の誤差で 100.9999… になって
	// 100.99 と表示されるのを防ぐため
	m_TargetScore = (int)std::floor(GetAchievement() * 100.0 + 1e-6);
}