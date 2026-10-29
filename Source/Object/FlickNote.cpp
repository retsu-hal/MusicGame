#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "Conductor.h"
#include "Settings.h"
#include "Chart.h"
#include "FlickNote.h"
#include "NoteManager.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include <cmath>
#include <algorithm>

// 3スライスの両端の幅(レーン単位)。表示 144px のうち、しっぽ 24px・矢印の先 96px を 1レーン = 120px で割ったもの
static constexpr float TAIL = 0.2f;
static constexpr float HEAD = 0.8f;
static constexpr float FLICK_MOVE = 1.0f; // 「振った」とみなす 50ms の移動量(レーン単位)★
static constexpr float FLICK_MARGIN = 0.25f;

// ===============================================================
// 初期化・読み込み
// ===============================================================
void FlickNote::Init()
{
    m_Layer = 2;

    // 頂点:原点が真ん中の 1×1 の板。UV はスライスごとに書きかえるので、メンバーに取っておく
    {
        m_Vertex[0].Position = XMFLOAT3(-0.5f, 0.5f, 0.0f); // 左上
        m_Vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_Vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        m_Vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

        m_Vertex[1].Position = XMFLOAT3(0.5f, 0.5f, 0.0f); // 右上
        m_Vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_Vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        m_Vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

        m_Vertex[2].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f); // 左下
        m_Vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_Vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        m_Vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

        m_Vertex[3].Position = XMFLOAT3(0.5f, -0.5f, 0.0f); // 右下
        m_Vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_Vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        m_Vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);
    }

    // 頂点バッファ生成(TapNote と同じ。sd.pSysMem だけ m_Vertex)
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VERTEX_3D) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = m_Vertex;

    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

    // シェーダー読み込み
    m_Shader = ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso");

    // テクスチャ読み込み
    m_RightTex = TextureManager::Load(L"asset\\texture\\flick_mouse_right.png");
    m_LeftTex = TextureManager::Load(L"asset\\texture\\flick_mouse_left.png");
}

void FlickNote::Uninit()
{
    m_vertexBuffer->Release();

    GameObject::Uninit();
}

void FlickNote::Update()
{
    if (!Conductor::IsPlaying())
        return; // ポーズ中・再開のカウント中は何もしない
    const double now = Conductor::GetSongTime();
    const float move = m_Manager->GetFlickMove();

    // 1. 振ったら、向き・位置・時刻が合うノーツを1個だけ取る
    if (std::fabs(move) >= FLICK_MOVE)
    {
        const int dir = (move > 0.0f) ? 1 : -1;           // 振った向き
        const float startX = m_Manager->GetFlickStartX(); // 振り始めの位置

        for (FlickData &n : m_Notes) // 時刻の順
        {
            if (n.judge != Judge::None)
                continue; // もう判定した

            double diff = now - n.time;
            if (diff > WINDOW_BAD)
                continue; // もう過ぎてる(このあと MISS になる)
            if (diff < -WINDOW_BAD)
                break; // ここから先は全部まだ遠い

            if (n.dir != dir)
                continue; // 向きが違う

            // 振り始めがノーツの上(左右に余裕つき)か
            float left = n.lane - FLICK_MARGIN;
            float right = n.lane + n.width + FLICK_MARGIN;
            if (startX < left || startX > right)
                continue;

            // タイミングはタップと同じ
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
            m_Manager->ClearFlick(); // 1回振ったら使い切る
            break;
        }
    }

    // 2. 振られないまま BAD の幅も過ぎたノーツは MISS
    for (FlickData &n : m_Notes)
    {
        if (n.judge == Judge::None && now - n.time > WINDOW_BAD)
            SetJudge(n, Judge::Miss, 0.0);
    }
}

// ===============================================================
// 描画
// ===============================================================
void FlickNote::Draw()
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
    DrawDir(1, now, speed);  // 右(ゴールド)
    DrawDir(-1, now, speed); // 左(コーラル)

    Renderer::SetDepthEnable(true);
}

void FlickNote::DrawDir(int dir, float now, float speed)
{
    // 3スライスの区切り。右向きは「しっぽ・伸ばす・矢印」、左向きは「矢印・伸ばす・しっぽ」
    const float leftPart = (dir > 0) ? TAIL : HEAD;  // 左の区間の幅(レーン単位)
    const float rightPart = (dir > 0) ? HEAD : TAIL; // 右の区間の幅

    // 画像の横の区切り(u)。0 / 区切り1 / 区切り2 / 1
    const float u1 = (dir > 0) ? 1.0f / 6.0f : 2.0f / 3.0f;
    const float u2 = (dir > 0) ? 1.0f / 3.0f : 5.0f / 6.0f;
    const float u[4] = {0.0f, u1, u2, 1.0f};

    // 厚み:表示 64px を 1レーン = 120px に合わせる(ホイールフリックと同じ厚みになる)
    const float depth = m_Layout.laneWidth * (64.0f / 120.0f);

    // テクスチャ設定(この向きの画像。DrawDir の中では変わらないので最初に 1 回)
    ID3D11ShaderResourceView *texture = (dir > 0) ? m_RightTex : m_LeftTex;
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);

    for (const FlickData &n : m_Notes)
    {
        if (n.dir != dir)
            continue;
        if (n.judge != Judge::None)
            continue;

        float z = ((float)n.time - now) * speed;
        if (z > Z_FAR || z < -60.0f)
            continue; // 遠すぎ・通り過ぎ

        float y = (n.layer == 1) ? UPPER_HEIGHT : 0.0f;

        // 横の区切り(x、レーン単位)。左端・区切り1・区切り2・右端
        float x0 = (float)n.lane;
        float x3 = x0 + n.width;
        float x[4] = {x0, x0 + leftPart, x3 - rightPart, x3};

        // マテリアル設定。色は画像に入っているので白、奥からふわっと(3 枚とも同じ)
        float fade = std::clamp((Z_FAR - z) / (Z_FAR * 0.3f), 0.0f, 1.0f);
        MATERIAL material{};
        material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, fade);
        material.TextureEnable = true;
        Renderer::SetMaterial(material);

        // 3枚並べる。i 番目の板は x[i]〜x[i+1] に、画像の u[i]〜u[i+1] を貼る
        for (int i = 0; i < 3; i++)
        {
            if (x[i + 1] <= x[i])
                continue; // 幅 0(1レーンのときの真ん中)は描かない

            // UV 設定(Polygon2D の SetUV と同じ。縦は画像全部 0〜1)
            m_Vertex[0].TexCoord = XMFLOAT2(u[i], 0.0f);     // 左上
            m_Vertex[1].TexCoord = XMFLOAT2(u[i + 1], 0.0f); // 右上
            m_Vertex[2].TexCoord = XMFLOAT2(u[i], 1.0f);     // 左下
            m_Vertex[3].TexCoord = XMFLOAT2(u[i + 1], 1.0f); // 右下
            Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer, 0, nullptr, m_Vertex, 0, 0);

            // スライスの真ん中(レーン全体の真ん中が x = 0)と、幅(ピクセル)
            float centerX = ((x[i] + x[i + 1]) * 0.5f - m_Layout.laneCount * 0.5f) * m_Layout.laneWidth;
            float width = (x[i + 1] - x[i]) * m_Layout.laneWidth;

            // マトリックス設定(床に寝かせる)
            XMMATRIX WorldMatrix, ScaleMatrix, RotMatrix, TransMatrix;
            ScaleMatrix = XMMatrixScaling(width, depth, 1.0f);
            RotMatrix = XMMatrixRotationRollPitchYaw(XM_PIDIV2, 0.0f, 0.0f);
            TransMatrix = XMMatrixTranslation(centerX, y, z);
            WorldMatrix = ScaleMatrix * RotMatrix * TransMatrix;
            Renderer::SetWorldMatrix(WorldMatrix);

            // 描画
            Renderer::GetDeviceContext()->Draw(4, 0);
        }
    }
}

void FlickNote::Load(const Chart &chart)
{
    m_Notes.clear();
    for (const Note &n : chart.notes)
    {
        if (n.type != 3)
            continue; // 左右フリックだけ拾う
        if (n.dir == 0)
            continue; // 向きがないものは振りようがないので飛ばす

        FlickData data;
        data.time = chart.TickToSec(n.tick);
        data.lane = n.lane;
        data.width = std::max(1, n.width); // 0 や マイナスと書かれていても 1 レーンにする
        data.layer = n.layer;
        data.dir = (n.dir > 0) ? 1 : -1;
        m_Notes.push_back(data);
    }
}

void FlickNote::Reset(double startSec)
{
    for (FlickData &n : m_Notes)
        n.judge = (n.time < startSec) ? Judge::Skip : Judge::None;
}

void FlickNote::SetJudge(FlickData &note, Judge judge, double diff)
{
    note.judge = judge;
    m_Manager->OnJudge(judge, note.lane, diff, note.width);
}