#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "GameObject.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "Audio.h"
#include "ModelRenderer.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "Collider.h"
#include "Conductor.h"
#include "Settings.h"
#include "QuadBatch.h"
#include <cmath>		

//staticメンバー変数はcppで定義する必要がある
std::list<GameObject* > Manager::m_GameObjects;
Scene* Manager::m_Scene=nullptr;
Scene* Manager::m_NextScene=nullptr;
float Manager::m_ChangeSceneTime = 5.0f;

float Manager::m_DeltaTime = 1.0f / 60.0f;

int Manager::m_FpsFrames = 0;
DWORD Manager::m_FpsLastTime = 0;
int Manager::m_Fps = 0;

void Manager::Init()
{
	Input::Init();
	Renderer::Init();
	QuadBatch::Init(4096);
	Audio::InitMaster();
	Conductor::Init();
	Settings::Load();                                   // 前回の設定（ハイスピード・オフセット）
	Audio::ApplyVolume();
	Conductor::SetOffset(Settings::offsetMs / 1000.0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// 日本語フォント（Windows に最初から入っているメイリオ）
	ImGuiIO& io = ImGui::GetIO();
	ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", 18.0f);
	IM_ASSERT(font != nullptr);   // 読み込めなかったら止める

	ImGui_ImplWin32_Init(GetWindow());
	ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetDeviceContext());

	m_FpsLastTime = timeGetTime();

	ChangeScene<TitleScene>();
}


void Manager::Uninit()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	
	for (GameObject* obj : m_GameObjects) obj->Uninit();
	for (GameObject* obj : m_GameObjects) delete obj;
	m_GameObjects.clear();
	
	if(m_Scene!=nullptr)
	{
		m_Scene->Uninit();
		delete m_Scene;
	}

	ModelRenderer::UnloadAll();	
	QuadBatch::Uninit();
	ShaderManager::Unload();
	TextureManager::Unload();

	Renderer::Uninit();
	Input::Uninit();
	Audio::UninitMaster();
}

void Manager::Update()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	// ImGui もゲーム画面（SCREEN_WIDTH x SCREEN_HEIGHT）の座標で描く。ウィンドウの大きさは Renderer::End で合わせる
	ImGui::GetIO().DisplaySize = ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	ImGui::NewFrame();

	float  dt =GetDeltaTime();
	Input::Update();
	Conductor::Update();



	if (m_Scene != nullptr)	m_Scene->Update();

	for (GameObject* gameObject : m_GameObjects) gameObject->Update();

	//当たり判定（各オブジェクトのUpdateで移動し終わってから、まとめて押し出す）
	Collider::Check();

	//ゲームオブジェクトの削除　【ラムダ式】
	m_GameObjects.remove_if([](GameObject* object)
		{
			return object->Destroy();
		});

	//シーンの切り替え
	if (m_NextScene != nullptr)
	{
		m_ChangeSceneTime -= dt;

		if(m_ChangeSceneTime <= 0.0f)
		{
			//現在のシーンを破棄
			if (m_Scene != nullptr)
			{
				m_Scene->Uninit();
				delete m_Scene;
			}

			//ゲームオブジェクトの削除
			for (GameObject* gameObject : m_GameObjects)
			{
				gameObject->Uninit();
				delete gameObject;
			}
			m_GameObjects.clear();

			Profiler::Clear();
			m_Scene = m_NextScene;
			{
				ScopedSceneTimer sceneTimer;	// シーン全体の実ロード時間(実経過)
				m_Scene->Init();
			}
			Profiler::Dump();
			m_NextScene = nullptr;
		}
	}
};

void Manager::Draw()
{
	Renderer::Begin();


	

	//Z値計算
	CAMERA* camera = GetGameObject<CAMERA>();

	if (camera)
	{
		Vector3 forward = camera->GetForward();
		Vector3 position = camera->GetPosition();

		//Z値計算
		for (GameObject* gameObject : m_GameObjects)
		{
			if (gameObject != nullptr)
			{
				gameObject->CalcCameraZ(position, forward);
			}
		}

		//Z値でソート
		m_GameObjects.sort([](GameObject* a, GameObject* b)
			{
				return a->GetCameraZ() > b->GetCameraZ();
			});
	}


	//レイヤー順で描画
	for (int layer = 0; layer < 4; layer++)
	{
		for (GameObject* gameObject : m_GameObjects)
		{
			if (gameObject != nullptr)
			{
				if(gameObject->GetLayer()==layer)
				gameObject->Draw();
			}
		}
	}
	
	//デバッグ表示（ImGui::Render より前に線をためる）

	DrawFps();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


	Renderer::End();
}

GameObject* Manager::FindGameObjectByID(unsigned int id)
{
	if (id == 0) return nullptr;
	for (GameObject* object : m_GameObjects)
	{
		if (object->GetID() == id && !object->IsDestroyed()) return object;
	}
	return nullptr;
}

//==============================================================================
//FPS を数えて、SHOW FPS が ON なら右上に描く
//==============================================================================
void Manager::DrawFps()
{
	//数える(OFF のときも数えておくと、ON にした瞬間から正しい値が出る)
	m_FpsFrames++;

	DWORD now = timeGetTime();
	DWORD elapsed = now - m_FpsLastTime;		// 数え始めてから何ミリ秒たったか

	if (elapsed >= 1000)						// 1秒たったら
	{
		m_Fps = (int)std::lround(m_FpsFrames * 1000.0 / elapsed);	// 1秒あたりに直す
		m_FpsFrames = 0;						// 次の1秒を数え直す
		m_FpsLastTime = now;
	}

	if (!Settings::showFps)
	{
		return;
	}

	//右上に置く。プレイ画面だけ、右上の「SCORE」と重ならないように左にずらす
	float right = SCREEN_WIDTH - 30.0f;
	if (dynamic_cast<GameScene*>(m_Scene) != nullptr)
	{
		right = SCREEN_WIDTH - 110.0f;
	}

	char text[16];
	snprintf(text, sizeof(text), "%d FPS", m_Fps);
}