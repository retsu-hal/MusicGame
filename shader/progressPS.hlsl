#include "common.hlsl"

static const float4 FILL_COLOR = float4(0.40, 0.86, 1.00, 1.0); // 進んだところの色(水色) ★
static const float4 TRACK_COLOR = float4(0.16, 0.16, 0.28, 0.8); // まだのところの色 ★
static const float BAR_WIDTH_PX = 504.0; // バーの幅(ピクセル)。C++ の BAR_W と合わせる ★
static const float GLOW_PX = 6.0; // 先端の光の広がり(ピクセル) 

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
	//---- 1. 横の位置(左 0、右 1)と、進み具合(C++ から。0〜1)
    float x = In.TexCoord.x;
    float p = Material.Emission.x;

	//---- 2. 塗るかどうか。x が p より左なら 1、右なら 0
	//        境目だけ 1 ピクセルぶんぼかす(ギザギザ・カクつき防止)
    float aa = 1.0 / BAR_WIDTH_PX;
    float fill = 1.0 - smoothstep(p - aa, p + aa, x);

    float4 col = lerp(TRACK_COLOR, FILL_COLOR, fill);

	//---- 3. 先端の光:先端に近いほど明るい。始まる前(p = 0)は出さない
    float distPx = abs(x - p) * BAR_WIDTH_PX;
    float glow = exp(-distPx / GLOW_PX) * step(0.001, p);
    col.rgb += FILL_COLOR.rgb * glow * 0.8;
    col.a = max(col.a, glow);

	//---- 4. マテリアルの色をかける(SetColor / SetAlpha が効くように)
    outDiffuse = col * In.Diffuse;
}