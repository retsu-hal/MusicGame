#pragma once
#include "main.h"
#include "GameObject.h"

class Chart;
struct ShaderSet;
// 見た目の設定(奥行き)
constexpr float CAMERA_DIST = 600.0f;  // カメラから判定ラインまでの距離
constexpr float HORIZON_Y = 90.0f;	   // 地平線(レンズの中心)の画面の高さ
constexpr float Z_FAR = 2400.0f;	   // 床の奥の端
constexpr float Z_NEAR = -110.0f;	   // 床の手前の端
constexpr int LANE_COUNT = 5;		   // レーンの数
constexpr float UPPER_HEIGHT = 200.0f; // 上段の高さ

// レーンの並び方。レーン上の位置と奥行きを、3D の世界座標にする
struct Layout
{
	static inline float laneWidth = 250.0f;  // レーン幅(判定ラインの位置でのピクセル)
	float judgeLineY = 850.0f; // 判定ラインの画面の高さ(カメラの位置に使う)
	float height = 0.0f;	   // この面の高さ。0 = 下段

	XMFLOAT3 World(float laneX, float z) const
	{
		return XMFLOAT3((laneX - LANE_COUNT * 0.5f) * laneWidth, height, z);
	}
};

// 両端のネオンの見た目
struct NeonParam
{
	XMFLOAT3 left = XMFLOAT3(0.31f, 0.35f, 1.00f);
	XMFLOAT3 right = XMFLOAT3(1.00f, 0.27f, 0.75f);
	float falloff = 35.30f;
	float intensity = 1.67f;
	float core = 3.50f;
};

class Lane : public GameObject
{
private:
	Layout m_bottomrow;
	Layout m_upperrow ;
	ID3D11ShaderResourceView *m_WhiteTex = nullptr;
	ID3D11ShaderResourceView *m_LaneTex = nullptr;
	ID3D11ShaderResourceView* m_KeyTex[LANE_COUNT] = {};
	const ShaderSet *m_Shader = nullptr;
	const ShaderSet* m_NeonShader = nullptr;
	float m_Glow[LANE_COUNT] = {};
	NeonParam m_Neon;

	void DrawNeon(const Layout &layout); // 1面ぶんの両端のネオン
	void DrawDepthLine(const Layout &L, float laneX, float width, const XMFLOAT4 &color);

public:
	static constexpr BYTE KEYS[LANE_COUNT] = {'A', 'S', 'D', 'F', VK_SPACE}; // 各レーンのキー

	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;

	NeonParam& GetNeon() { return m_Neon; } // デバッグ画面から書き換える用
};
