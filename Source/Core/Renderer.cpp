
#include "main.h"
#include "Renderer.h"
#include "ShaderManager.h"

#include <io.h>
#include <algorithm>


D3D_FEATURE_LEVEL       Renderer::m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*           Renderer::m_Device{};
ID3D11DeviceContext*    Renderer::m_DeviceContext{};
IDXGISwapChain*         Renderer::m_SwapChain{};
ID3D11RenderTargetView* Renderer::m_RenderTargetView{};
ID3D11DepthStencilView* Renderer::m_DepthStencilView{};

ID3D11RenderTargetView*   Renderer::m_BackBufferView{};
ID3D11ShaderResourceView* Renderer::m_ScreenView{};
ID3D11Buffer*             Renderer::m_ScreenVertexBuffer{};
ID3D11SamplerState*       Renderer::m_SamplerState{};
ID3D11SamplerState*       Renderer::m_ScreenSampler{};
D3D11_VIEWPORT            Renderer::m_ScreenViewport{};

ID3D11Buffer*			Renderer::m_WorldBuffer{};
ID3D11Buffer*			Renderer::m_ViewBuffer{};
ID3D11Buffer*			Renderer::m_ProjectionBuffer{};
ID3D11Buffer*			Renderer::m_MaterialBuffer{};
ID3D11Buffer*			Renderer::m_LightBuffer{};
ID3D11Buffer*			Renderer::m_CameraBuffer{};



ID3D11DepthStencilState* Renderer::m_DepthStateEnable{};
ID3D11DepthStencilState* Renderer::m_DepthStateDisable{};


ID3D11BlendState*		Renderer::m_BlendState{};
ID3D11BlendState*		Renderer::m_BlendStateAdd{};
ID3D11BlendState*		Renderer::m_BlendStateATC{};




void Renderer::Init()
{
	HRESULT hr = S_OK;


	

	// ウィンドウの中身の大きさ（スワップチェーンはこの大きさ、ゲーム画面は SCREEN_WIDTH x SCREEN_HEIGHT 固定）
	RECT clientRect;
	GetClientRect(GetWindow(), &clientRect);
	UINT clientWidth  = (UINT)std::max(clientRect.right - clientRect.left, 1L);
	UINT clientHeight = (UINT)std::max(clientRect.bottom - clientRect.top, 1L);

	// デバイス、スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// ボーダーレスのフルスクリーンでも遅延が少ない方式

	hr = D3D11CreateDeviceAndSwapChain( NULL,
										D3D_DRIVER_TYPE_HARDWARE,
										NULL,
										0,
										NULL,
										0,
										D3D11_SDK_VERSION,
										&swapChainDesc,
										&m_SwapChain,
										&m_Device,
										&m_FeatureLevel,
										&m_DeviceContext );






	// Alt+Enter はDXGIに任せず、自前のフルスクリーン切り替え（main.cpp）で処理する
	IDXGIFactory* factory{};
	if (SUCCEEDED(m_SwapChain->GetParent(__uuidof(IDXGIFactory), (void**)&factory)))
	{
		factory->MakeWindowAssociation(GetWindow(), DXGI_MWA_NO_ALT_ENTER);
		factory->Release();
	}

	// ウィンドウに出す画面のレンダーターゲットビュー作成
	CreateBackBuffer();
	UpdateScreenViewport(clientWidth, clientHeight);


	// ゲーム画面のレンダーターゲット作成（16:9固定。最後にウィンドウへ拡大して貼る）
	{
		D3D11_TEXTURE2D_DESC screenDesc{};
		screenDesc.Width = SCREEN_WIDTH;
		screenDesc.Height = SCREEN_HEIGHT;
		screenDesc.MipLevels = 1;
		screenDesc.ArraySize = 1;
		screenDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		screenDesc.SampleDesc.Count = 1;
		screenDesc.Usage = D3D11_USAGE_DEFAULT;
		screenDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		ID3D11Texture2D* screenTexture{};
		m_Device->CreateTexture2D(&screenDesc, NULL, &screenTexture);
		m_Device->CreateRenderTargetView(screenTexture, NULL, &m_RenderTargetView);
		m_Device->CreateShaderResourceView(screenTexture, NULL, &m_ScreenView);
		screenTexture->Release();
	}


	// デプスステンシルバッファ作成（ゲーム画面と同じ大きさ）
	ID3D11Texture2D* depthStencile{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = SCREEN_WIDTH;
	textureDesc.Height = SCREEN_HEIGHT;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D16_UNORM;
	textureDesc.SampleDesc = swapChainDesc.SampleDesc;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	m_Device->CreateTexture2D(&textureDesc, NULL, &depthStencile);

	// デプスステンシルビュー作成
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = textureDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	m_Device->CreateDepthStencilView(depthStencile, &depthStencilViewDesc, &m_DepthStencilView);
	depthStencile->Release();


	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);





	// ビューポート設定
	D3D11_VIEWPORT viewport;
	viewport.Width = (FLOAT)SCREEN_WIDTH;
	viewport.Height = (FLOAT)SCREEN_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_DeviceContext->RSSetViewports( 1, &viewport );



	// ラスタライザステート設定
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID; 
	rasterizerDesc.CullMode = D3D11_CULL_BACK; 
	rasterizerDesc.DepthClipEnable = TRUE; 
	rasterizerDesc.MultisampleEnable = FALSE; 

	ID3D11RasterizerState *rs;
	m_Device->CreateRasterizerState( &rasterizerDesc, &rs );

	m_DeviceContext->RSSetState( rs );




	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_Device->CreateBlendState( &blendDesc, &m_BlendState );

	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	m_Device->CreateBlendState(&blendDesc, &m_BlendStateAdd);

	blendDesc.AlphaToCoverageEnable = TRUE;
	m_Device->CreateBlendState( &blendDesc, &m_BlendStateATC );

	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff );





	// デプスステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDesc.StencilEnable = FALSE;

	m_Device->CreateDepthStencilState( &depthStencilDesc, &m_DepthStateEnable );//深度有効ステート

	//depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ZERO;
	m_Device->CreateDepthStencilState( &depthStencilDesc, &m_DepthStateDisable );//深度無効ステート

	m_DeviceContext->OMSetDepthStencilState( m_DepthStateEnable, NULL );




	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	m_Device->CreateSamplerState( &samplerDesc, &m_SamplerState );

	m_DeviceContext->PSSetSamplers( 0, 1, &m_SamplerState );

	// ゲーム画面をウィンドウに貼るとき用（線形補間・端はCLAMP）
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_Device->CreateSamplerState( &samplerDesc, &m_ScreenSampler );



	// ゲーム画面を貼る四角形（SetWorldViewProjection2D の座標で画面全体）
	{
		VERTEX_3D vertex[4]{};
		vertex[0].Position = XMFLOAT3(0.0f,         0.0f,          0.0f);
		vertex[1].Position = XMFLOAT3(SCREEN_WIDTH, 0.0f,          0.0f);
		vertex[2].Position = XMFLOAT3(0.0f,         SCREEN_HEIGHT, 0.0f);
		vertex[3].Position = XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f);
		vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);
		vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);
		vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);
		vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);
		for (VERTEX_3D& v : vertex) v.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(VERTEX_3D) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = vertex;
		m_Device->CreateBuffer(&bd, &sd, &m_ScreenVertexBuffer);
	}



	// 定数バッファ生成
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = sizeof(float);

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_WorldBuffer );
	m_DeviceContext->VSSetConstantBuffers( 0, 1, &m_WorldBuffer);

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_ViewBuffer );
	m_DeviceContext->VSSetConstantBuffers( 1, 1, &m_ViewBuffer );

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_ProjectionBuffer );
	m_DeviceContext->VSSetConstantBuffers( 2, 1, &m_ProjectionBuffer );


	bufferDesc.ByteWidth = sizeof(MATERIAL);

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_MaterialBuffer );
	m_DeviceContext->VSSetConstantBuffers( 3, 1, &m_MaterialBuffer );
	m_DeviceContext->PSSetConstantBuffers( 3, 1, &m_MaterialBuffer );


	bufferDesc.ByteWidth = sizeof(LIGHT);

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_LightBuffer );
	m_DeviceContext->VSSetConstantBuffers( 4, 1, &m_LightBuffer );
	m_DeviceContext->PSSetConstantBuffers( 4, 1, &m_LightBuffer );

	bufferDesc.ByteWidth = sizeof(XMFLOAT4);
	m_Device->CreateBuffer( &bufferDesc, NULL, &m_CameraBuffer );
	m_DeviceContext->VSSetConstantBuffers( 5, 1, &m_CameraBuffer );
	m_DeviceContext->PSSetConstantBuffers(5, 1, &m_CameraBuffer);




	// ライト初期化
	LIGHT light{};
	light.Enable = true;
	light.Direction = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	light.Diffuse = XMFLOAT4(1.5f, 1.5f, 1.5f, 1.0f);
	SetLight(light);



	// マテリアル初期化
	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);




}



void Renderer::Uninit()
{

	m_WorldBuffer->Release();
	m_ViewBuffer->Release();
	m_ProjectionBuffer->Release();
	m_LightBuffer->Release();
	m_MaterialBuffer->Release();


	m_DeviceContext->ClearState();
	m_ScreenVertexBuffer->Release();
	m_ScreenSampler->Release();
	m_SamplerState->Release();
	m_ScreenView->Release();
	m_BackBufferView->Release();
	m_RenderTargetView->Release();
	m_SwapChain->Release();
	m_DeviceContext->Release();
	m_Device->Release();

}




void Renderer::Begin()
{
	// ゲーム画面（SCREEN_WIDTH x SCREEN_HEIGHT）に描く
	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

	D3D11_VIEWPORT viewport{};
	viewport.Width = (FLOAT)SCREEN_WIDTH;
	viewport.Height = (FLOAT)SCREEN_HEIGHT;
	viewport.MaxDepth = 1.0f;
	m_DeviceContext->RSSetViewports(1, &viewport);

	float clearColor[4] = { 0.0f, 0.5f, 1.0f, 0.1f };
	m_DeviceContext->ClearRenderTargetView( m_RenderTargetView, clearColor );
	m_DeviceContext->ClearDepthStencilView( m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}



void Renderer::End()
{
	// ゲーム画面を、16:9のままウィンドウいっぱいに拡大して貼る（余った所は黒帯）
	m_DeviceContext->OMSetRenderTargets(1, &m_BackBufferView, NULL);

	float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_DeviceContext->ClearRenderTargetView(m_BackBufferView, black);
	m_DeviceContext->RSSetViewports(1, &m_ScreenViewport);

	m_DeviceContext->OMSetBlendState(NULL, NULL, 0xffffffff);	// 半透明にせず、そのまま貼る

	ShaderManager::Load("shader\\unlitTextureVS.cso", "shader\\unlitTexturePS.cso")->Set();
	SetWorldViewProjection2D();

	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.TextureEnable = TRUE;
	SetMaterial(material);

	m_DeviceContext->PSSetShaderResources(0, 1, &m_ScreenView);
	m_DeviceContext->PSSetSamplers(0, 1, &m_ScreenSampler);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	m_DeviceContext->IASetVertexBuffers(0, 1, &m_ScreenVertexBuffer, &stride, &offset);
	m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_DeviceContext->Draw(4, 0);

	// 次のフレームでゲーム画面をレンダーターゲットに戻せるよう、テクスチャとしての設定を外す
	ID3D11ShaderResourceView* nullView = NULL;
	m_DeviceContext->PSSetShaderResources(0, 1, &nullView);
	m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerState);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

	m_SwapChain->Present( 1, 0 );
}



void Renderer::Resize(UINT Width, UINT Height)
{
	// 初期化前や最小化中は何もしない
	if (m_SwapChain == NULL || Width == 0 || Height == 0) return;

	// スワップチェーンの画面を使っている参照を外してから作り直す
	m_DeviceContext->OMSetRenderTargets(0, NULL, NULL);
	m_BackBufferView->Release();
	m_BackBufferView = NULL;

	m_SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);

	CreateBackBuffer();
	UpdateScreenViewport(Width, Height);
}



XMFLOAT2 Renderer::ClientToGame(int X, int Y)
{
	if (m_ScreenViewport.Width <= 0.0f || m_ScreenViewport.Height <= 0.0f)
		return XMFLOAT2((float)X, (float)Y);

	return XMFLOAT2(
		(X - m_ScreenViewport.TopLeftX) * SCREEN_WIDTH  / m_ScreenViewport.Width,
		(Y - m_ScreenViewport.TopLeftY) * SCREEN_HEIGHT / m_ScreenViewport.Height);
}



void Renderer::CreateBackBuffer()
{
	ID3D11Texture2D* backBuffer{};
	m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	m_Device->CreateRenderTargetView(backBuffer, NULL, &m_BackBufferView);
	backBuffer->Release();
}



void Renderer::UpdateScreenViewport(UINT Width, UINT Height)
{
	// ウィンドウに収まる一番大きい16:9の範囲を、真ん中に置く
	float scale = std::min((float)Width / SCREEN_WIDTH, (float)Height / SCREEN_HEIGHT);
	float w = floorf(SCREEN_WIDTH * scale);
	float h = floorf(SCREEN_HEIGHT * scale);

	m_ScreenViewport.TopLeftX = floorf((Width - w) * 0.5f);
	m_ScreenViewport.TopLeftY = floorf((Height - h) * 0.5f);
	m_ScreenViewport.Width = w;
	m_ScreenViewport.Height = h;
	m_ScreenViewport.MinDepth = 0.0f;
	m_ScreenViewport.MaxDepth = 1.0f;
}




void Renderer::SetDepthEnable( bool Enable )
{
	if( Enable )
		m_DeviceContext->OMSetDepthStencilState( m_DepthStateEnable, NULL );
	else
		m_DeviceContext->OMSetDepthStencilState( m_DepthStateDisable, NULL );

}

void Renderer::SetAddEnable(bool Enable)
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateAdd, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

}

void Renderer::SetATCEnable( bool Enable )
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateATC, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

}

void Renderer::SetWorldViewProjection2D()
{
	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());

	XMMATRIX projection;
	projection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(projection);
}


void Renderer::SetWorldMatrix(XMMATRIX WorldMatrix)
{
	XMFLOAT4X4 worldf;
	XMStoreFloat4x4(&worldf, XMMatrixTranspose(WorldMatrix));
	m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, NULL, &worldf, 0, 0);
}

void Renderer::SetViewMatrix(XMMATRIX ViewMatrix)
{
	XMFLOAT4X4 viewf;
	XMStoreFloat4x4(&viewf, XMMatrixTranspose(ViewMatrix));
	m_DeviceContext->UpdateSubresource(m_ViewBuffer, 0, NULL, &viewf, 0, 0);
}

void Renderer::SetProjectionMatrix(XMMATRIX ProjectionMatrix)
{
	XMFLOAT4X4 projectionf;
	XMStoreFloat4x4(&projectionf, XMMatrixTranspose(ProjectionMatrix));
	m_DeviceContext->UpdateSubresource(m_ProjectionBuffer, 0, NULL, &projectionf, 0, 0);

}



void Renderer::SetMaterial( MATERIAL Material )
{
	m_DeviceContext->UpdateSubresource( m_MaterialBuffer, 0, NULL, &Material, 0, 0 );
}

void Renderer::SetLight( LIGHT Light )
{
	m_DeviceContext->UpdateSubresource(m_LightBuffer, 0, NULL, &Light, 0, 0);
}
void Renderer::SetCameraPosition(Vector3 CameraPosition)
{
	XMFLOAT4 CameraPos;
	CameraPos.x = CameraPosition.x;
	CameraPos.y = CameraPosition.y;
	CameraPos.z = CameraPosition.z;
	CameraPos.w = 1.0f;

	m_DeviceContext->UpdateSubresource(m_CameraBuffer, 0, NULL, &CameraPos, 0, 0);
}


void Renderer::CreateVertexShader( ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName )
{

	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	if (!file) { OutputDebugStringA("シェーダーが見つからない: "); OutputDebugStringA(FileName); OutputDebugStringA("\n"); }
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreateVertexShader(buffer, fsize, NULL, VertexShader);


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	m_Device->CreateInputLayout(layout,
		numElements,
		buffer,
		fsize,
		VertexLayout);

	delete[] buffer;
}



void Renderer::CreatePixelShader( ID3D11PixelShader** PixelShader, const char* FileName )
{
	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreatePixelShader(buffer, fsize, NULL, PixelShader);

	delete[] buffer;
}


