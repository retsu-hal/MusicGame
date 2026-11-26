// laneNeonPS.hlsl  レーン両端のネオン。床と同じ板 1 枚に貼る
#include "common.hlsl"

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    float LANE_W = Material.Emission.w;
    float3 COLOR_L = Material.Ambient.rgb;
    float3 COLOR_R = Material.Specular.rgb;
    float FALLOFF = Material.Emission.x;
    float INTENSITY = Material.Emission.y;
    float CORE = Material.Emission.z;
    
    // UV の x(0〜1)を、左端からのピクセル距離にする
    float x = In.TexCoord.x * LANE_W;
    float dL = x; // 左端までの距離
    float dR = LANE_W - x; // 右端までの距離

    // 端ほど 1、離れると exp で急に 0 へ
    float glowL = exp(-dL / FALLOFF) * INTENSITY;
    float glowR = exp(-dR / FALLOFF) * INTENSITY;

    // 白い芯。CORE より内側だけ 1(境目は 0.75px ぼかす)
    float coreL = smoothstep(CORE + 0.75, CORE - 0.75, dL);
    float coreR = smoothstep(CORE + 0.75, CORE - 0.75, dR);

    float3 col = COLOR_L * glowL + COLOR_R * glowR + 0.8 * (coreL + coreR);

    // 加算で重ねるので、アルファは 1 でいい(黒 = 何も足さない)
    outDiffuse = float4(col, 1.0);
}