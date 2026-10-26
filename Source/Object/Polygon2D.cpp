#include "main.h"
#include "Renderer.h"
#include "Polygon2D.h"

void Polygon2D::Init(float x, float y, float width, float height,const WCHAR* TextureName)
{
	{
		m_Vertex[0].Position = XMFLOAT3(x, y, 0.0f);
		m_Vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_Vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_Vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

		m_Vertex[1].Position = XMFLOAT3(x + width, y, 0.0f);
		m_Vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_Vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_Vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

		m_Vertex[2].Position = XMFLOAT3(x, y + height, 0.0f);
		m_Vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_Vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_Vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

		m_Vertex[3].Position = XMFLOAT3(x + width, y + height, 0.0f);
		m_Vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_Vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_Vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);
	}

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = m_Vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);
	//シェーダー読み込み
	Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_PixelShader, "shader\\unlitTexturePS.cso");
	
	//テクスチャ読み込み
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(TextureName, WIC_FLAGS_NONE, &metadata, image);//テクスチャは変更可
	CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
		image.GetImageCount(), metadata, &m_Texture);
	assert(m_Texture);//読み込み失敗時にダイアログを表示
}

void Polygon2D::Uninit()
{
	m_vertexBuffer->Release();

	m_VertexLayout->Release();
	m_VertexShader->Release();
	m_PixelShader->Release();
	m_Texture->Release();
}

void Polygon2D::Update()
{
}

void Polygon2D::Draw()
{
	//入力レイアウト設定
	Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

	//シェーダー設定
	Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

	//マトリックス設定
	Renderer::SetWorldViewProjection2D();

	XMMATRIX WorldMatrix,ScaleMatrix,RotMatrix,TransMatrix;
	ScaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);								//拡大縮小
	RotMatrix = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);		//回転
	TransMatrix = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);						//平行移動
	WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;

	Renderer::SetWorldMatrix(WorldMatrix);

	//マテリアル設定
	MATERIAL material{};
	material.Diffuse = m_Color;
	material.Emission = m_Param;			
	material.TextureEnable = true;			//true:テクスチャを使用する、false:テクスチャを使用しない
	Renderer::SetMaterial(material);


	//テクスチャ設定
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);
	//頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	//プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//描画
	Renderer::GetDeviceContext()->Draw(4, 0);
}

void Polygon2D::SetGradientAlpha(float leftAlpha, float rightAlpha)
{
	m_Vertex[0].Diffuse.w = leftAlpha;		// 左上
	m_Vertex[2].Diffuse.w = leftAlpha;		// 左下
	m_Vertex[1].Diffuse.w = rightAlpha;		// 右上
	m_Vertex[3].Diffuse.w = rightAlpha;		// 右下

	// 書き換えた頂点を GPU のバッファへ送り直す
	Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer, 0, nullptr, m_Vertex, 0, 0);
}

void Polygon2D::SetPixelShader(const char* FileName)
{
	if (m_PixelShader) m_PixelShader->Release();		// Init で作った unlitTexturePS を捨てる
	Renderer::CreatePixelShader(&m_PixelShader, FileName);
}

void Polygon2D::SetRect(float x, float y, float width, float height)
{
	m_Vertex[0].Position = XMFLOAT3(x, y, 0.0f);	// 左上
	m_Vertex[1].Position = XMFLOAT3(x + width, y, 0.0f);	// 右上
	m_Vertex[2].Position = XMFLOAT3(x, y + height, 0.0f);	// 左下
	m_Vertex[3].Position = XMFLOAT3(x + width, y + height, 0.0f);	// 右下

	// 書き換えた頂点を GPU のバッファへ送り直す
	Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer, 0, nullptr, m_Vertex, 0, 0);
}

void Polygon2D::SetUV(float u, float v, float width, float height)
{
	m_Vertex[0].TexCoord = XMFLOAT2(u, v);			// 左上
	m_Vertex[1].TexCoord = XMFLOAT2(u + width, v);			// 右上
	m_Vertex[2].TexCoord = XMFLOAT2(u, v + height);	// 左下
	m_Vertex[3].TexCoord = XMFLOAT2(u + width, v + height);	// 右下

	// 書き換えた頂点を GPU のバッファへ送り直す
	Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer, 0, nullptr, m_Vertex, 0, 0);
}
