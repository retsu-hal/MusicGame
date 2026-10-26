#pragma once
#include "main.h"
#include "GameObject.h"
#include <algorithm>

namespace LaneDraw
{
	// 見た目の設定(奥行き)
	constexpr float CAMERA_DIST = 600.0f;   // カメラから判定ラインまでの距離
	constexpr float HORIZON_Y = 90.0f;    // 地平線(レンズの中心)の画面の高さ
	constexpr float Z_FAR = 2400.0f;  // 床の奥の端
	constexpr float Z_NEAR = -110.0f;  // 床の手前の端

	constexpr int   LANE_COUNT = 6;        // レーンの数(Shift A S D F Space)
	constexpr float UPPER_HEIGHT = 200.0f;   // 上段の高さ

	// レーンの並び方。レーン上の位置と奥行きを、3D の世界座標にする
	struct Layout
	{
		int   laneCount = LANE_COUNT;
		float laneWidth = 150.0f;   // レーン幅(判定ラインの位置でのピクセル)
		float judgeLineY = 870.0f;   // 判定ラインの画面の高さ(カメラの位置に使う)
		float height = 0.0f;     // この面の高さ。0 = 下段

		XMFLOAT3 World(float laneX, float z) const
		{
			return XMFLOAT3((laneX - laneCount * 0.5f) * laneWidth, height, z);
		}
	};

	// DX11 用の色(0〜255 と 0〜1 のアルファ)
	inline XMFLOAT4 Rgba(int r, int g, int b, float a)
	{
		return XMFLOAT4(r / 255.0f, g / 255.0f, b / 255.0f, std::clamp(a, 0.0f, 1.0f));
	}
}


class Chart;

// レーンの床(床・押したときの光・拍の線・区切り線・ネオン・2段をつなぐ線)
// 下段と上段の両方を描く。レイヤー1 = ノーツより奥
class Lane : public GameObject
{
public:
	static constexpr BYTE KEYS[LaneDraw::LANE_COUNT] = { VK_SHIFT, 'A', 'S', 'D', 'F', VK_SPACE };   // 各レーンのキー

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	LaneDraw::Layout          m_Layout;
	ID3D11ShaderResourceView* m_White = nullptr;           // 色を付けるための真っ白な画像
	float                     m_Glow[LaneDraw::LANE_COUNT] = {};

	void DrawFloor(const LaneDraw::Layout& layout, const Chart& chart, float now, float speed, float active);  // active:0〜1。1 = 今いる段	
	void DrawFrame(const LaneDraw::Layout& lower, const LaneDraw::Layout& upper);              // 2段をつなぐ縦の線
	void DrawNeon(const LaneDraw::Layout& layout);                                            // 1段ぶんの両端のネオン
};


// 判定ライン・キー枠。今いる段の高さに出る
// レイヤー3 = ノーツより手前
class JudgeLine : public GameObject
{
public:
	void Init() override;
	void Update() override;
	void Draw() override;

private:
	LaneDraw::Layout          m_Layout;
	ID3D11ShaderResourceView* m_White = nullptr;
	float                     m_Glow[LaneDraw::LANE_COUNT] = {};
};