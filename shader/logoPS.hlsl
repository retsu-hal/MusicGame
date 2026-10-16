#include "common.hlsl"


Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

static const float3 GLOW_COLOR = float3(0.435, 0.890, 1.0); // #6FE3FF 水色
static const float SWEEP_TIME = 3.0; // 光が1回通り抜けて、次が来るまでの秒数
static const float SWEEP_WIDTH = 0.05; // 光の帯の太さ(UV の幅)

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float4 tex = g_Texture.Sample(g_SamplerState, In.TexCoord);

	//---- 1. 文字の色(「:」だけ水色)
    float3 color = In.Diffuse.rgb;

	//---- 2. 文字の周りの光(周りのアルファの平均)
    float2 texel;
    g_Texture.GetDimensions(texel.x, texel.y);
    texel = 1.0 / texel; // 1ピクセルが UV でいくつか

    float around = 0.0;
    for (int i = 0; i < 8; i++)
    {
        float angle = i * (6.2831853 / 8.0);
        float2 dir = float2(cos(angle), sin(angle));
        around += g_Texture.Sample(g_SamplerState, In.TexCoord + dir * texel * 6.0).a;
        around += g_Texture.Sample(g_SamplerState, In.TexCoord + dir * texel * 12.0).a;
    }
    around /= 16.0;

	//---- 3. 光の帯(時間で左から右へ動く)
    float time = Material.Emission.x; // C++ から渡した経過時間
    float pos = frac(time / SWEEP_TIME) * 1.6 - 0.3; // -0.3 → 1.3 へ動く
    float u = In.TexCoord.x + In.TexCoord.y * 0.15; // 少し斜めにする
    float sweep = exp(-pow((u - pos) / SWEEP_WIDTH, 2.0)); // 帯の中心で 1、離れると 0

	//---- 4. 合成
    float glow = sweep * saturate(around * 2.2); // 帯の中で、文字の近くだけ光る
    float3 letter = lerp(color, GLOW_COLOR, sweep * 0.5); // 帯が通るとき文字自体も少し水色に

    outDiffuse.rgb = lerp(GLOW_COLOR, letter, tex.a); // 文字の上は文字の色、外は光の色
    outDiffuse.a = saturate(tex.a + glow) * In.Diffuse.a; // 文字か光のある所だけ見える。フェードも効く
}