#include "main.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "QuadBatch.h"

// static メンバーの実体
ID3D11Buffer* QuadBatch::m_Buffer = nullptr;
std::vector<VERTEX_3D> QuadBatch::m_Vertices;
int                    QuadBatch::m_MaxVertices = 0;
const ShaderSet* QuadBatch::m_Shader = nullptr;


void QuadBatch::Init(int maxQuads)
{
	m_MaxVertices = maxQuads * 6;          // 四角形1枚 = 三角形2枚 = 頂点6個
	m_Vertices.reserve(m_MaxVertices);

	// 毎フレーム書き換えるので DYNAMIC + CPU から書ける
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * m_MaxVertices;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_Buffer);

	// テクスチャ × 頂点カラー のシェーダー(Polygon2D と同じもの)
	m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");
}

void QuadBatch::Uninit()
{
	if (m_Buffer) { m_Buffer->Release(); m_Buffer = nullptr; }
	m_Vertices.clear();
	m_Shader = nullptr;   // シェーダーは ShaderManager が解放する
}

void QuadBatch::Quad(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT3& p3,
					 const XMFLOAT4& topColor, const XMFLOAT4& bottomColor,
					 XMFLOAT2 uvMin, XMFLOAT2 uvMax)
{
	if ((int)m_Vertices.size() + 6 > m_MaxVertices) return;   // あふれた分は捨てる(Init の数を増やす)

	VERTEX_3D v[4]{};
	v[0].Position = p0; v[0].Diffuse = topColor;    v[0].TexCoord = XMFLOAT2(uvMin.x, uvMin.y);
	v[1].Position = p1; v[1].Diffuse = topColor;    v[1].TexCoord = XMFLOAT2(uvMax.x, uvMin.y);
	v[2].Position = p2; v[2].Diffuse = bottomColor; v[2].TexCoord = XMFLOAT2(uvMax.x, uvMax.y);
	v[3].Position = p3; v[3].Diffuse = bottomColor; v[3].TexCoord = XMFLOAT2(uvMin.x, uvMax.y);
	for (VERTEX_3D& vv : v) vv.Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

	// 三角形2枚:0-1-2 と 0-2-3(どっちも画面で時計回り)
	m_Vertices.push_back(v[0]); m_Vertices.push_back(v[1]); m_Vertices.push_back(v[2]);
	m_Vertices.push_back(v[0]); m_Vertices.push_back(v[2]); m_Vertices.push_back(v[3]);
}

void QuadBatch::Flush(ID3D11ShaderResourceView* texture, bool add)
{
	if (m_Vertices.empty()) return;
	ID3D11DeviceContext* context = Renderer::GetDeviceContext();

	// 1. ためた頂点を GPU に送る(WRITE_DISCARD = 前の中身は捨てていい)
	D3D11_MAPPED_SUBRESOURCE mapped{};
	context->Map(m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, m_Vertices.data(), sizeof(VERTEX_3D) * m_Vertices.size());
	context->Unmap(m_Buffer, 0);

	// 2. シェーダー・マテリアル・テクスチャ
	m_Shader->Set();

	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);   // 色は頂点カラーで付ける
	material.TextureEnable = true;
	Renderer::SetMaterial(material);

	context->PSSetShaderResources(0, 1, &texture);

	// 3. 頂点バッファと形
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &m_Buffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 4. 描く(加算のときだけ合成を切り替えて、終わったら戻す)
	Renderer::SetAddEnable(add);
	context->Draw((UINT)m_Vertices.size(), 0);
	Renderer::SetAddEnable(false);

	m_Vertices.clear();
}