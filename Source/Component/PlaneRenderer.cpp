#include "main.h"
#include "Renderer.h"
#include "Manager.h"
#include "GameObject.h"
#include "Camera.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "PlaneRenderer.h"

void PlaneRenderer::Load(const wchar_t *textureFile)
{
    m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");
    m_Texture = TextureManager::Load(textureFile);

    // 色と UV を毎フレーム書き換えるので、初期データなしの動的バッファを作る
    if (m_VertexBuffer == nullptr)
    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(VERTEX_3D) * 4;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_VertexBuffer);
    }
}

void PlaneRenderer::Uninit()
{
    if (m_VertexBuffer)
    {
        m_VertexBuffer->Release();
        m_VertexBuffer = nullptr;
    }
}

void PlaneRenderer::Draw()
{
    CAMERA *camera = Manager::GetGameObject<CAMERA>();
    if (camera == nullptr)
        return;

    ID3D11DeviceContext *context = Renderer::GetDeviceContext();

    // カメラの行列をセットし直す(同じレイヤーの 2D が 2D 用の行列にしていることがあるので)
    camera->Draw();

    // 頂点更新(色・UV)。形は原点にある 1×1 の板。並びは Polygon2D と同じ Z 字
    float u0 = m_UV.x, v0 = m_UV.y;
    float u1 = m_UV.x + m_UV.z, v1 = m_UV.y + m_UV.w;
    XMFLOAT3 normal(0.0f, 0.0f, -1.0f);

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    VERTEX_3D *vertex = (VERTEX_3D *)msr.pData;
    vertex[0] = {XMFLOAT3(-0.5f, 0.5f, 0.0f), normal, m_Color, XMFLOAT2(u0, v0)};  // 左上
    vertex[1] = {XMFLOAT3(0.5f, 0.5f, 0.0f), normal, m_Color, XMFLOAT2(u1, v0)};   // 右上
    vertex[2] = {XMFLOAT3(-0.5f, -0.5f, 0.0f), normal, m_Color, XMFLOAT2(u0, v1)}; // 左下
    vertex[3] = {XMFLOAT3(0.5f, -0.5f, 0.0f), normal, m_Color, XMFLOAT2(u1, v1)};  // 右下
    context->Unmap(m_VertexBuffer, 0);

    // ワールド行列(Scale × Rot × Trans。親がいれば親の行列も掛かる)
    Renderer::SetWorldMatrix(m_GameObject->GetWorldMatrix());

    // マテリアル(色は頂点カラーで指定済み)
    MATERIAL material{};
    material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    // 描画
    m_Shader->Set();
    context->PSSetShaderResources(0, 1, &m_Texture);
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Renderer::SetDepthEnable(false); // 前後は描く順番で決める(今のノーツと同じ)
    context->Draw(4, 0);
    Renderer::SetDepthEnable(true);
}