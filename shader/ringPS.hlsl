#include "common.hlsl"

static const float RING_RADIUS = 0.8;	// 輪の半径(板の端 = 1.0)★
static const float WIDTH_START = 0.20;	// 出た瞬間の輪の太さ ★
static const float WIDTH_END = 0.05;	// 消えるときの輪の太さ ★
static const float INNER_GLOW = 0.25;	// 内側のうっすらした光の強さ ★

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
	//---- 1. 板の真ん中からの距離(真ん中 0、端 1)
	float2 p = In.TexCoord * 2.0 - 1.0;
	float d = length(p);

	//---- 2. 広がり具合(C++ から。0 = 出た瞬間、1 = 消える)
	float t = Material.Emission.x;

	//---- 3. 輪:半径のところで 1、離れると 0。だんだん細くなる
	float width = lerp(WIDTH_START, WIDTH_END, t);
	float ring = exp(-pow((d - RING_RADIUS) / width, 2.0));

	//---- 4. 内側のうっすらした光(真ん中ほど明るい。広がると消える)
	float inner = saturate(1.0 - d / RING_RADIUS) * INNER_GLOW * (1.0 - t);

	//---- 5. 合成。色はマテリアルの色、アルファで形を作る(加算なのでアルファ = 明るさ)
	outDiffuse.rgb = In.Diffuse.rgb;
	outDiffuse.a = saturate(ring + inner) * In.Diffuse.a;
}
