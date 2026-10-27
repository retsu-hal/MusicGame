#pragma once
#include "main.h"
#include "GameObject.h"

namespace LaneDraw
{
	// 見た目の設定(奥行き)
	constexpr float CAMERA_DIST = 600.0f;   // カメラから判定ラインまでの距離
	constexpr float HORIZON_Y = 90.0f;    // 地平線(レンズの中心)の画面の高さ
	constexpr float Z_FAR = 2400.0f;  // 床の奥の端
	constexpr float Z_NEAR = -110.0f;  // 床の手前の端

	constexpr int   LANE_COUNT = 5;        // レーンの数(Shift A S D F Space)
	constexpr float UPPER_HEIGHT = 200.0f;   // 上段の高さ

	// レーンの並び方。レーン上の位置と奥行きを、3D の世界座標にする
	struct Layout
	{
		int   laneCount = LANE_COUNT;
		float laneWidth = 150.0f;   // レーン幅(判定ラインの位置でのピクセル)
		float judgeLineY = 850.0f;   // 判定ラインの画面の高さ(カメラの位置に使う)
		float height = 0.0f;     // この面の高さ。0 = 下段

		XMFLOAT3 World(float laneX, float z) const
		{
			return XMFLOAT3((laneX - laneCount * 0.5f) * laneWidth, height, z);
		}
	};

	// 3D で描く準備と後片付け。3D で描くもの(Lane・JudgeLine・ノーツ)は、Draw の頭と最後で呼ぶ
	// 中身は Lane.cpp に1つだけ
	void Begin3D();
	void End3D();
}


class Chart;

// レーンの床(床・押したときの光・拍の線・区切り線・ネオン)
// 今いる面(上下する)と、上段の枠(固定)を描く。レイヤー1 = ノーツより奥
class Lane : public GameObject
{
public:
	static constexpr BYTE KEYS[LaneDraw::LANE_COUNT] = {  'A', 'S', 'D', 'F', VK_SPACE };   // 各レーンのキー

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	LaneDraw::Layout          m_Layout;
	ID3D11ShaderResourceView* m_White = nullptr;   // 色を付けるための真っ白な画像
	ID3D11ShaderResourceView* m_LaneTex = nullptr;   // 床の画像(lane.png)
	float                     m_Glow[LaneDraw::LANE_COUNT] = {};

	// 1面ぶん。active:1 = 今いる面(濃い床・光・区切り線)、0 = 上段の枠(薄い床だけ)
	void DrawFloor(const LaneDraw::Layout& layout, const Chart& chart, float now, float speed, float active);
	void DrawNeon(const LaneDraw::Layout& layout);   // 1面ぶんの両端のネオン
};


// 判定ライン・キー枠。判定ラインは上段(固定)と今いる面、キー枠は今いる面
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
	ID3D11ShaderResourceView* m_Cursor = nullptr;
	float                     m_Glow[LaneDraw::LANE_COUNT] = {};
};