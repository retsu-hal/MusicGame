/*==============================================================================

[Combo.cpp]
														Author :Watanabe Retsu
														Date   :2026/10/26
--------------------------------------------------------------------------------

==============================================================================*/

//==============================================================================
//インクルード
//==============================================================================
#include "main.h"
#include "Renderer.h"
#include "Manager.h"
#include "Combo.h"
#include <algorithm>

//==============================================================================
//マクロ宣言
//==============================================================================
#define TEXTURE_NUM_WIDTH 4
#define TEXTURE_NUM_HEIGHT 4
#define DIGIT_WIDTH  112.0f      // play.png のコンボは大きい
#define DIGIT_HEIGHT 112.0f
#define DIGIT_STEP   66.0f
#define COMBO_X      1560.0f     // 数字の真ん中(レーンの右)
#define COMBO_Y      600.0f      // 数字の上端
#define POP_TIME     0.15f

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
void Combo::Init()
{
	m_Layer = 3;
	m_Position = Vector3(COMBO_X, COMBO_Y, 0.0f);


	// 中身は Draw で毎回書くので、ここでは全部 0 でいい
	VERTEX_3D vertex[4 * DIGIT_NUM] = {};

	for (int i = 0; i < DIGIT_NUM; i++)
	{
		float offsetX = DIGIT_STEP * i;

		vertex[i * 4 + 0].Position = XMFLOAT3(offsetX, 0.0f, 0.0f);
		vertex[i * 4 + 0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 0].TexCoord = XMFLOAT2(0.0f, 0.0f);

		vertex[i * 4 + 1].Position = XMFLOAT3(offsetX + DIGIT_WIDTH, 0.0f, 0.0f);
		vertex[i * 4 + 1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 1].TexCoord = XMFLOAT2(1.0f, 0.0f);

		vertex[i * 4 + 2].Position = XMFLOAT3(offsetX, DIGIT_HEIGHT, 0.0f);
		vertex[i * 4 + 2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 2].TexCoord = XMFLOAT2(0.0f, 1.0f);

		vertex[i * 4 + 3].Position = XMFLOAT3(offsetX + DIGIT_WIDTH, DIGIT_HEIGHT, 0.0f);
		vertex[i * 4 + 3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		vertex[i * 4 + 3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		vertex[i * 4 + 3].TexCoord = XMFLOAT2(1.0f, 1.0f);
	}

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
void Combo::Uninit()
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
void Combo::Update()
{
}

//==============================================================================
//描画処理
//==============================================================================
void Combo::Draw()
{
	if (m_Combo < 2) return;

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

	// 桁に分ける(下の桁から)
	int digit[DIGIT_NUM];
	int length = 0;
	int value = std::min(m_Combo, 9999);   // 5桁になると四角形が足りないので止める
	do
	{
		digit[length++] = value % 10;
		value /= 10;
	} while (value > 0);

	D3D11_MAPPED_SUBRESOURCE msr;
	Renderer::GetDeviceContext()->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

	const float sizeX = 1.0f / TEXTURE_NUM_WIDTH;
	const float sizeY = 1.0f / TEXTURE_NUM_HEIGHT;

	// 真ん中そろえ:右端 = 全体の幅の半分。そこから左へ置いていく
	float x = DIGIT_STEP * length * 0.5f;
	for (int i = 0; i < length; i++)
	{
		int num = digit[i];
		x -= DIGIT_STEP;
		float left = x + DIGIT_STEP * 0.5f - DIGIT_WIDTH * 0.5f;
		SetQuad(&vertex[i * 4], left, 0.0f, DIGIT_WIDTH, DIGIT_HEIGHT,
				sizeX * (num % TEXTURE_NUM_WIDTH), sizeY * (num / TEXTURE_NUM_WIDTH), sizeX, sizeY);
	}

	Renderer::GetDeviceContext()->Unmap(m_vertexBuffer, 0);

	for (int i = 0; i < length; i++)
		Renderer::GetDeviceContext()->Draw(4, i * 4);
}

void Combo::Reset()
{
	m_Combo = 0;
	m_MaxCombo = 0;
}

void Combo::AddCombo()
{
	m_Combo++;
	m_MaxCombo = std::max(m_MaxCombo, m_Combo);
}

void Combo::ResetCombo()
{
	m_Combo = 0;
}