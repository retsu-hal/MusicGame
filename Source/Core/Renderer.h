#pragma once
#include "Vector3.h"


struct VERTEX_SKIN
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Diffuse;
	XMFLOAT2 TexCoord;
	UINT           BoneIndex[4];
	XMFLOAT4 BoneWeight;
};

struct VERTEX_3D
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Diffuse;
	XMFLOAT2 TexCoord;
};



struct MATERIAL
{
	XMFLOAT4	Ambient;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Specular;
	XMFLOAT4	Emission;
	float		Shininess;
	BOOL		TextureEnable;
	float		Dummy[2];
};



struct LIGHT
{
	BOOL		Enable;
	BOOL		Dummy[3];
	XMFLOAT4	Direction;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Ambient;
};



class Renderer
{
private:

	static D3D_FEATURE_LEVEL       m_FeatureLevel;

	static ID3D11Device*           m_Device;
	static ID3D11DeviceContext*    m_DeviceContext;
	static IDXGISwapChain*         m_SwapChain;
	static ID3D11RenderTargetView* m_RenderTargetView;		// ゲーム画面（SCREEN_WIDTH x SCREEN_HEIGHT 固定）
	static ID3D11DepthStencilView* m_DepthStencilView;

	// 16:9のゲーム画面を、ウィンドウの大きさに合わせて拡大して表示するためのもの
	static ID3D11RenderTargetView*   m_BackBufferView;		// ウィンドウに出す画面（ウィンドウと同じ大きさ）
	static ID3D11ShaderResourceView* m_ScreenView;			// ゲーム画面をテクスチャとして読むためのビュー
	static ID3D11Buffer*             m_ScreenVertexBuffer;	// ゲーム画面を貼る四角形
	static ID3D11SamplerState*       m_SamplerState;		// 通常のサンプラー
	static ID3D11SamplerState*       m_ScreenSampler;		// ゲーム画面を貼るとき用（端がにじまないようCLAMP）
	static D3D11_VIEWPORT            m_ScreenViewport;		// ウィンドウ内で16:9になる範囲（上下か左右が黒帯）

	static ID3D11Buffer*			m_WorldBuffer;
	static ID3D11Buffer*			m_ViewBuffer;
	static ID3D11Buffer*			m_ProjectionBuffer;
	static ID3D11Buffer*			m_MaterialBuffer;
	static ID3D11Buffer*			m_LightBuffer;
	static ID3D11Buffer*			m_CameraBuffer;



	static ID3D11DepthStencilState* m_DepthStateEnable;
	static ID3D11DepthStencilState* m_DepthStateDisable;

	static ID3D11BlendState*		m_BlendState;
	static ID3D11BlendState*		m_BlendStateAdd;
	static ID3D11BlendState*		m_BlendStateATC;

	static void CreateBackBuffer();
	static void UpdateScreenViewport(UINT Width, UINT Height);


public:
	static void Init();
	static void Uninit();
	static void Begin();
	static void End();

	static void Resize(UINT Width, UINT Height);			// ウィンドウの大きさが変わったとき（WM_SIZE）
	static XMFLOAT2 ClientToGame(int X, int Y);				// ウィンドウ上の座標 → ゲーム画面の座標

	static void SetDepthEnable(bool Enable);
	static void SetAddEnable(bool Enable);
	static void SetATCEnable(bool Enable);
	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(XMMATRIX WorldMatrix);
	static void SetViewMatrix(XMMATRIX ViewMatrix);
	static void SetProjectionMatrix(XMMATRIX ProjectionMatrix);
	static void SetMaterial(MATERIAL Material);
	static void SetLight(LIGHT Light);
	static void SetCameraPosition(Vector3 CameraPosition);
	static void ClearDepth();

	static ID3D11Device* GetDevice( void ){ return m_Device; }
	static ID3D11DeviceContext* GetDeviceContext( void ){ return m_DeviceContext; }



	static void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
	static void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);


};
