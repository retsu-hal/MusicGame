#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "NoteManager.h"
#include "Lane.h"
#include "Effect.h"
#include <algorithm>

static constexpr float SHOW_TIME = 0.5f;  // 判定文字を出しておく時間(秒)★
static constexpr float POP_TIME = 0.08f;  // 出た瞬間に大きくしている時間(秒)
static constexpr float FADE_TIME = 0.15f; // 消えるときに薄くしていく時間(秒)
static constexpr float RING_TIME = 0.35f; // 輪が広がって消えるまでの時間(秒)★

// 判定ごとの色(Judge の番号の順。0 = None は使わない)
// 本当は芯と光を別の色にする(ui-mockups/text/README.md)。今は頂点の色を掛けるだけ ★
static const XMFLOAT4 JUDGE_COLOR[6] = {
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),    // None
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),    // PERFECT 白(あとで虹色にする ★)
    XMFLOAT4(1.0f, 0.55f, 0.75f, 1.0f),  // GREAT マゼンタ寄り
    XMFLOAT4(0.75f, 0.95f, 0.45f, 1.0f), // GOOD 黄緑
    XMFLOAT4(0.55f, 0.70f, 1.0f, 1.0f),  // BAD 青
    XMFLOAT4(0.60f, 0.60f, 0.68f, 1.0f), // MISS 灰色
};

// 判定文字の画像の大きさ(ピクセル)。高さはどれも 152
static const float JUDGE_W[6] = {0.0f, 374.0f, 285.0f, 258.0f, 206.0f, 233.0f};
static constexpr float JUDGE_H = 152.0f;

void Effect::Init()
{
    m_Layer = 3; // ノーツより手前

    // 頂点:原点が真ん中の 1×1 の板。2D(y が下向き)なので、Polygon2D と同じ向き
    VERTEX_3D vertex[4];
    {
        vertex[0].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f); // 左上
        vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

        vertex[1].Position = XMFLOAT3(0.5f, -0.5f, 0.0f); // 右上
        vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

        vertex[2].Position = XMFLOAT3(-0.5f, 0.5f, 0.0f); // 左下
        vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

        vertex[3].Position = XMFLOAT3(0.5f, 0.5f, 0.0f); // 右下
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

    // シェーダー読み込み。頂点シェーダーは同じで、ピクセルシェーダーだけちがう
    m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");
    m_RingShader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\ringPS.cso");

    m_JudgeTex[(int)Judge::Perfect] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_perfect.png");
    m_JudgeTex[(int)Judge::Great] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_great.png");
    m_JudgeTex[(int)Judge::Good] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_good.png");
    m_JudgeTex[(int)Judge::Bad] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_bad.png");
    m_JudgeTex[(int)Judge::Miss] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_miss.png");

    m_FastTex = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_fast.png");
    m_SlowTex = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_slow.png");
}

void Effect::Uninit()
{
    m_vertexBuffer->Release();

    GameObject::Uninit();
}

void Effect::Show(Judge judge, int lane, int width, double diff)
{
    // 1. 判定文字(上書き)
    m_Judge = judge;
    m_JudgeTime = 0.0f;

    // FAST / SLOW は GREAT・GOOD・BAD のときだけ。diff がマイナス = 早い
    if (judge == Judge::Great || judge == Judge::Good || judge == Judge::Bad)
        m_Timing = (diff < 0.0) ? -1 : 1;
    else
        m_Timing = 0;

    // 2. 輪(MISS と、幅 0 のときは出さない)
    if (judge != Judge::Miss && width > 0)
        m_Rings.push_back({(float)lane, (float)width, 0.0f, judge});
}

void Effect::Update()
{
    const float dt = Manager::GetDeltaTime();

    m_JudgeTime += dt;
    for (HitRing &r : m_Rings)
        r.time += dt;

    // 消えた輪(RING_TIME を過ぎたもの)を取り除く
    m_Rings.erase(std::remove_if(m_Rings.begin(), m_Rings.end(),
                                 [](const HitRing &r)
                                 { return r.time >= RING_TIME; }),
                  m_Rings.end());
}

void Effect::Draw()
{
    NoteManager *noteManager = Manager::GetGameObject<NoteManager>();
    if (!noteManager)
        return;

    // 今いる面の判定ラインの、画面での高さ(奥行き 0 なので世界の 1 = 画面の 1 ピクセル)
    const Layout L;
    const float lineY = L.judgeLineY - UPPER_HEIGHT * noteManager->GetPlayLayerAnim();
    const float centerX = SCREEN_WIDTH * 0.5f;

    // ----- 共通の設定(最初に 1 回) -----
    Renderer::SetDepthEnable(false);

    // マトリックス設定(2D)
    Renderer::SetWorldViewProjection2D();

    // 頂点バッファ設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    // プリミティブトポロジ設定
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
    RotMatrix = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f); // 2D なので回転なし(Polygon2D と同じ)
    MATERIAL material{};

    // ----- 1. 輪(加算)。ringPS で描くので画像はなし -----
    m_RingShader->Set();
    material.TextureEnable = false;
    Renderer::SetAddEnable(true);
    for (const HitRing &r : m_Rings)
    {
        float t = r.time / RING_TIME; // 0 → 1

        // 輪の中心:範囲の真ん中(レーン単位)を画面の x に
        float x = centerX + (r.lane + r.width * 0.5f - L.laneCount * 0.5f) * L.laneWidth;

        // 大きさ:レーン幅の 0.6 倍 → 1.3 倍に広がる。横は範囲の幅ぶん伸ばす
        float s = 0.6f + 0.7f * t;

        // マトリックス設定
        ScaleMatrix = XMMatrixScaling(L.laneWidth * r.width * s, L.laneWidth * s, 1.0f);
        TransMatrix = XMMatrixTranslation(x, lineY, 0.0f);
        WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
        Renderer::SetWorldMatrix(WorldMatrix);

        // マテリアル設定(広がりながら消える。広がり具合 t はシェーダーへ)
        material.Diffuse = JUDGE_COLOR[(int)r.judge];
        material.Diffuse.w = 1.0f - t;
        material.Emission = XMFLOAT4(t, 0.0f, 0.0f, 0.0f);
        Renderer::SetMaterial(material);

        // 描画
        Renderer::GetDeviceContext()->Draw(4, 0);
    }
    Renderer::SetAddEnable(false);

    // ここからは画像の板(unlitTexture)
    m_Shader->Set();
    material.TextureEnable = true;
    material.Emission = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

    // ----- 2. 判定文字 -----
    if (m_Judge != Judge::None && m_JudgeTime < SHOW_TIME)
    {
        // 出た瞬間は 1.3 倍、POP_TIME かけて 1 倍に戻す
        float pop = (m_JudgeTime < POP_TIME) ? 1.3f - 0.3f * (m_JudgeTime / POP_TIME) : 1.0f;

        // 最後の FADE_TIME で薄くする
        float fade = std::clamp((SHOW_TIME - m_JudgeTime) / FADE_TIME, 0.0f, 1.0f);

        float y = lineY - 100.0f; // 文字の中心(判定ラインの 100 ピクセル上)★

        // マトリックス設定
        ScaleMatrix = XMMatrixScaling(JUDGE_W[(int)m_Judge] * pop, JUDGE_H * pop, 1.0f);
        TransMatrix = XMMatrixTranslation(centerX, y, 0.0f);
        WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
        Renderer::SetWorldMatrix(WorldMatrix);

        // マテリアル設定
        material.Diffuse = JUDGE_COLOR[(int)m_Judge];
        material.Diffuse.w = fade;
        Renderer::SetMaterial(material);

        // テクスチャ設定・描画
        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_JudgeTex[(int)m_Judge]);
        Renderer::GetDeviceContext()->Draw(4, 0);

        // ----- 3. FAST / SLOW(判定文字の上) -----
        if (m_Timing != 0)
        {
            float w = (m_Timing < 0) ? 71.0f : 80.0f; // 画像の幅(高さは 41)
            float fy = y - JUDGE_H * 0.5f;            // 判定文字の画像の上の端

            // マトリックス設定(下の端が fy なので、真ん中は fy - 41 / 2)
            ScaleMatrix = XMMatrixScaling(w, 41.0f, 1.0f);
            TransMatrix = XMMatrixTranslation(centerX, fy - 41.0f * 0.5f, 0.0f);
            WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
            Renderer::SetWorldMatrix(WorldMatrix);

            // マテリアル設定
            material.Diffuse = (m_Timing < 0) ? XMFLOAT4(0.435f, 0.890f, 1.0f, fade) // FAST 水色
                                              : XMFLOAT4(1.0f, 0.55f, 0.75f, fade);  // SLOW ピンク
            Renderer::SetMaterial(material);

            // テクスチャ設定・描画
            ID3D11ShaderResourceView *texture = (m_Timing < 0) ? m_FastTex : m_SlowTex;
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
            Renderer::GetDeviceContext()->Draw(4, 0);
        }
    }

    Renderer::SetDepthEnable(true);
}