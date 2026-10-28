#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "QuadBatch.h"
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

    m_JudgeTex[(int)Judge::Perfect] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_perfect.png");
    m_JudgeTex[(int)Judge::Great] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_great.png");
    m_JudgeTex[(int)Judge::Good] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_good.png");
    m_JudgeTex[(int)Judge::Bad] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_bad.png");
    m_JudgeTex[(int)Judge::Miss] = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_miss.png");

    m_FastTex = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_fast.png");
    m_SlowTex = TextureManager::Load(L"asset\\texture\\ui\\text\\play\\judge_slow.png");
    m_RingTex = TextureManager::Load(L"asset\\texture\\ui\\assets\\ring_wave.png");
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

    // 今いる面の判定ラインの、画面での高さ(奥行き 0 なので世界の 1 = 画面の 1 ピクセル。0章)
    const Layout L;
    const float lineY = L.judgeLineY - UPPER_HEIGHT * noteManager->GetPlayLayerAnim();
    const float centerX = SCREEN_WIDTH * 0.5f;

    // 2D で描く
    Renderer::SetDepthEnable(false);
    Renderer::SetWorldViewProjection2D();

    // 1. 輪(加算)
    for (const HitRing &r : m_Rings)
    {
        float t = r.time / RING_TIME; // 0 → 1

        // 輪の中心:範囲の真ん中(レーン単位)を画面の x に
        float x = centerX + (r.lane + r.width * 0.5f - L.laneCount * 0.5f) * L.laneWidth;

        // 大きさ:レーン幅の 0.6 倍 → 1.3 倍に広がる。横は範囲の幅ぶん伸ばす
        float s = 0.6f + 0.7f * t;
        float hw = L.laneWidth * r.width * s * 0.5f; // 横の半分
        float hh = L.laneWidth * s * 0.5f;           // 縦の半分

        XMFLOAT4 color = JUDGE_COLOR[(int)r.judge];
        color.w = 1.0f - t; // 広がりながら消える

        QuadBatch::Quad(XMFLOAT3(x - hw, lineY - hh, 0.0f), XMFLOAT3(x + hw, lineY - hh, 0.0f),
                        XMFLOAT3(x + hw, lineY + hh, 0.0f), XMFLOAT3(x - hw, lineY + hh, 0.0f),
                        color, color);
    }
    QuadBatch::Flush(m_RingTex, true);

    // 2. 判定文字
    if (m_Judge != Judge::None && m_JudgeTime < SHOW_TIME)
    {
        // 出た瞬間は 1.3 倍、POP_TIME かけて 1 倍に戻す
        float pop = (m_JudgeTime < POP_TIME) ? 1.3f - 0.3f * (m_JudgeTime / POP_TIME) : 1.0f;

        // 最後の FADE_TIME で薄くする
        float fade = std::clamp((SHOW_TIME - m_JudgeTime) / FADE_TIME, 0.0f, 1.0f);

        XMFLOAT4 color = JUDGE_COLOR[(int)m_Judge];
        color.w = fade;

        float y = lineY - 100.0f; // 文字の中心(判定ラインの 100 ピクセル上)★
        float hw = JUDGE_W[(int)m_Judge] * pop * 0.5f;
        float hh = JUDGE_H * pop * 0.5f;
        QuadBatch::Quad(XMFLOAT3(centerX - hw, y - hh, 0.0f), XMFLOAT3(centerX + hw, y - hh, 0.0f),
                        XMFLOAT3(centerX + hw, y + hh, 0.0f), XMFLOAT3(centerX - hw, y + hh, 0.0f),
                        color, color);
        QuadBatch::Flush(m_JudgeTex[(int)m_Judge]);

        // 3. FAST / SLOW(判定文字の上)
        if (m_Timing != 0)
        {
            float w = (m_Timing < 0) ? 71.0f : 80.0f;                          // 画像の幅(高さは 41)
            float fy = y - JUDGE_H * 0.5f;                                     // 判定文字の画像の上の端
            XMFLOAT4 c = (m_Timing < 0) ? XMFLOAT4(0.435f, 0.890f, 1.0f, fade) // FAST 水色
                                        : XMFLOAT4(1.0f, 0.55f, 0.75f, fade);  // SLOW ピンク
            QuadBatch::Quad(XMFLOAT3(centerX - w * 0.5f, fy - 41.0f, 0.0f), XMFLOAT3(centerX + w * 0.5f, fy - 41.0f, 0.0f),
                            XMFLOAT3(centerX + w * 0.5f, fy, 0.0f), XMFLOAT3(centerX - w * 0.5f, fy, 0.0f),
                            c, c);
            QuadBatch::Flush((m_Timing < 0) ? m_FastTex : m_SlowTex);
        }
    }

    Renderer::SetDepthEnable(true);
}