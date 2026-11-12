#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Camera.h"
#include "GameObject.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "Audio.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "Conductor.h"
#include "Settings.h"
#include "NoteManager.h"
#include <cmath>

// staticメンバー変数はcppで定義する必要がある
std::list<GameObject *> Manager::m_GameObjects;
Scene *Manager::m_Scene = nullptr;
Scene *Manager::m_NextScene = nullptr;
float Manager::m_ChangeSceneTime = 5.0f;

float Manager::m_DeltaTime = 1.0f / 60.0f;

int Manager::m_FpsFrames = 0;
DWORD Manager::m_FpsLastTime = 0;
int Manager::m_Fps = 0;

bool Manager::m_ShowDebug = false;
bool Manager::m_DebugChangedMouse = false;

void Manager::Init()
{
	Input::Init();
	Renderer::Init();
	Audio::InitMaster();
	Conductor::Init();
	Settings::Load(); // 前回の設定（ハイスピード・オフセット）
	Audio::ApplyVolume();
	Conductor::SetOffset(Settings::offsetMs / 1000.0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// 日本語フォント（Windows に最初から入っているメイリオ）
	ImGuiIO &io = ImGui::GetIO();
	ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", 18.0f);
	IM_ASSERT(font != nullptr); // 読み込めなかったら止める

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

	for (GameObject *obj : m_GameObjects)
		obj->Uninit();
	for (GameObject *obj : m_GameObjects)
		delete obj;
	m_GameObjects.clear();

	if (m_Scene != nullptr)
	{
		m_Scene->Uninit();
		delete m_Scene;
	}

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

	float dt = GetDeltaTime();
	Input::Update();
	Conductor::Update();

	// F1：デバッグ画面の表示 ⇔ 非表示
	if (Input::GetKeyTrigger(VK_F1))
	{
		m_ShowDebug = !m_ShowDebug;

		if (m_ShowDebug)
		{
			// 相対モード（プレイ中）ならカーソルを出す
			m_DebugChangedMouse = (Input::GetMouseMode() == Input::MOUSE_MODE_RELATIVE);
			if (m_DebugChangedMouse)
				Input::SetMouseMode(Input::MOUSE_MODE_ABSOLUTE);
		}
		else if (m_DebugChangedMouse)
		{
			// 自分で変えたときだけ元に戻す
			Input::SetMouseMode(Input::MOUSE_MODE_RELATIVE);
			m_DebugChangedMouse = false;
		}
	}
	DrawDebugWindow();	// m_Scene->Update より前：変えた値がこのフレームの処理にすぐ効く

	if (m_Scene != nullptr)
		m_Scene->Update();

	for (GameObject *gameObject : m_GameObjects)
		gameObject->Update();

	// ゲームオブジェクトの削除　【ラムダ式】
	m_GameObjects.remove_if([](GameObject *object)
							{ return object->Destroy(); });

	// シーンの切り替え
	if (m_NextScene != nullptr)
	{
		m_ChangeSceneTime -= dt;

		if (m_ChangeSceneTime <= 0.0f)
		{
			// 現在のシーンを破棄
			if (m_Scene != nullptr)
			{
				m_Scene->Uninit();
				delete m_Scene;
			}

			// ゲームオブジェクトの削除
			for (GameObject *gameObject : m_GameObjects)
			{
				gameObject->Uninit();
				delete gameObject;
			}
			m_GameObjects.clear();

			Profiler::Clear();
			m_DebugChangedMouse = false;	// 新しいシーンが自分でモードを決めるので、戻す責任は捨てる
			m_Scene = m_NextScene;
			{
				ScopedSceneTimer sceneTimer; // シーン全体の実ロード時間(実経過)
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

	// Z値計算
	CAMERA *camera = GetGameObject<CAMERA>();

	if (camera)
	{
		Vector3 forward = camera->GetForward();
		Vector3 position = camera->GetPosition();

		// Z値計算
		for (GameObject *gameObject : m_GameObjects)
		{
			if (gameObject != nullptr)
			{
				gameObject->CalcCameraZ(position, forward);
			}
		}

		// Z値でソート
		m_GameObjects.sort([](GameObject *a, GameObject *b)
						   { return a->GetCameraZ() > b->GetCameraZ(); });
	}

	// レイヤー順で描画
	for (int layer = 0; layer < 4; layer++)
	{
		for (GameObject *gameObject : m_GameObjects)
		{
			if (gameObject != nullptr)
			{
				if (gameObject->GetLayer() == layer)
					gameObject->Draw();
			}
		}
	}

	// デバッグ表示（ImGui::Render より前に線をためる）

	DrawFps();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	Renderer::End();
}

GameObject *Manager::FindGameObjectByID(unsigned int id)
{
	if (id == 0)
		return nullptr;
	for (GameObject *object : m_GameObjects)
	{
		if (object->GetID() == id && !object->IsDestroyed())
			return object;
	}
	return nullptr;
}

//==============================================================================
// FPS を数えて、SHOW FPS が ON なら右上に描く
//==============================================================================
void Manager::DrawFps()
{
	// 数える(OFF のときも数えておくと、ON にした瞬間から正しい値が出る)
	m_FpsFrames++;

	DWORD now = timeGetTime();
	DWORD elapsed = now - m_FpsLastTime; // 数え始めてから何ミリ秒たったか

	if (elapsed >= 1000) // 1秒たったら
	{
		m_Fps = (int)std::lround(m_FpsFrames * 1000.0 / elapsed); // 1秒あたりに直す
		m_FpsFrames = 0;										  // 次の1秒を数え直す
		m_FpsLastTime = now;
	}

	if (!Settings::showFps)
	{
		return;
	}

	// 右上に置く。プレイ画面だけ、右上の「SCORE」と重ならないように左にずらす
	float right = SCREEN_WIDTH - 30.0f;
	if (dynamic_cast<GameScene *>(m_Scene) != nullptr)
	{
		right = SCREEN_WIDTH - 110.0f;
	}

	char text[16];
	snprintf(text, sizeof(text), "%d FPS", m_Fps);
}


//==============================================================================
// F1 のデバッグ画面：設定の全項目・カメラ・楽曲のスキップ
//==============================================================================
void Manager::DrawDebugWindow()
{
	if (!m_ShowDebug) return;

	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("デバッグ (F1)");

	//---------------- 設定 ----------------
	if (ImGui::CollapsingHeader("設定", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// ハイスピード：中身は 800 とかだけど、設定画面に合わせて 8.0 で見せる
		float hiSpeed = Settings::noteSpeed / 100.0f;
		if (ImGui::SliderFloat("ハイスピード", &hiSpeed, 2.0f, 20.0f, "%.1f"))
			Settings::noteSpeed = hiSpeed * 100.0f;

		// 判定オフセットは Conductor に渡さないと効かない
		if (ImGui::SliderFloat("判定オフセット(ms)", &Settings::offsetMs, -200.0f, 200.0f, "%.0f"))
			Conductor::SetOffset(Settings::offsetMs / 1000.0);

		ImGui::SliderFloat("表示オフセット(ms)", &Settings::visualOffsetMs, -200.0f, 200.0f, "%.0f");

		// マウス感度：中身 0.025 → 表示 2.50
		float sens = Settings::mouseSensitivity * 100.0f;
		if (ImGui::SliderFloat("マウス感度", &sens, 0.10f, 5.00f, "%.2f"))
			Settings::mouseSensitivity = sens / 100.0f;

		ImGui::Checkbox("レイヤーガイド", &Settings::layerGuide);
		ImGui::Checkbox("FAST / SLOW", &Settings::fastSlow);

		// フルスクリーンは値を書き換えるだけじゃ切り替わらないので関数を呼ぶ
		bool full = IsFullscreen();
		if (ImGui::Checkbox("フルスクリーン", &full))
			ToggleFullscreen();

		ImGui::Checkbox("FPS 表示", &Settings::showFps);

		// 音量：どれか変わったら XAudio2 に反映
		bool volChanged = false;
		volChanged |= ImGui::SliderInt("全体の音量", &Settings::masterVolume, 0, 100);
		volChanged |= ImGui::SliderInt("曲の音量", &Settings::musicVolume, 0, 100);
		volChanged |= ImGui::SliderInt("SE の音量", &Settings::seVolume, 0, 100);
		if (volChanged)
			Audio::ApplyVolume();

		if (ImGui::Button("settings.json に保存"))
			Settings::Save();
	}

	//---------------- カメラ（プレイ画面だけ） ----------------
	CAMERA* camera = GetGameObject<CAMERA>();
	if (camera && ImGui::CollapsingHeader("カメラ", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Vector3 pos = camera->GetPosition();
		Vector3 dir = camera->GetTarget() - pos;     // 位置 → 注視点
		float dist = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (dist < 0.001f) dist = 0.001f;            // 0 で割らない

		// 向きから角度を出す（+ピッチ = 上向き、+ヨー = 右向き）
		float pitch = XMConvertToDegrees(asinf(dir.y / dist));
		float yaw = XMConvertToDegrees(atan2f(dir.x, dir.z));

		// 位置：動かしても向きは変えない（注視点も一緒に動かす）
		if (ImGui::DragFloat3("位置", &pos.x, 1.0f))
		{
			camera->SetPosition(pos);
			camera->SetTarget(pos + dir);
		}

		// 角度：距離はそのまま、向きだけ変えて注視点を置き直す
		bool angleChanged = false;
		angleChanged |= ImGui::SliderFloat("ピッチ(上下)", &pitch, -89.0f, 89.0f, "%.1f°");
		angleChanged |= ImGui::SliderFloat("ヨー(左右)", &yaw, -180.0f, 180.0f, "%.1f°");
		if (angleChanged)
		{
			float p = XMConvertToRadians(pitch);
			float y = XMConvertToRadians(yaw);
			Vector3 newDir(cosf(p) * sinf(y), sinf(p), cosf(p) * cosf(y));
			camera->SetTarget(pos + newDir * dist);
		}

		Vector3 target = camera->GetTarget();
		ImGui::Text("注視点  %.1f, %.1f, %.1f", target.x, target.y, target.z);
		ImGui::Text("注視点までの距離  %.1f", dist);
	}

	//---------------- 楽曲（プレイ画面だけ） ----------------
	NoteManager* notes = GetGameObject<NoteManager>();
	if (notes && ImGui::CollapsingHeader("楽曲", ImGuiTreeNodeFlags_DefaultOpen))
	{
		double now = Conductor::GetSongTime();
		double length = notes->GetLength();
		ImGui::Text("再生位置  %.1f / %.1f 秒", now, length);

		// ポーズ中に飛ぶと、ポーズメニューが出たまま曲が鳴り出すので止めておく
		if (!Conductor::IsPlaying())
		{
			ImGui::TextDisabled("ポーズ中はスキップできない");
		}
		else
		{
			if (ImGui::Button("-10秒")) notes->SeekTo(now - 10.0);
			ImGui::SameLine();
			if (ImGui::Button("+10秒")) notes->SeekTo(now + 10.0);

			// スライダーで好きな位置へ。ドラッグ中に毎フレーム飛ぶと音がブツブツになるので、離した瞬間だけ飛ぶ
			static float seekPos = 0.0f;
			static bool  dragging = false;
			if (!dragging) seekPos = (float)now;	// 触ってない間は今の位置に追従
			ImGui::SliderFloat("位置へ移動", &seekPos, 0.0f, (float)length, "%.1f 秒");
			dragging = ImGui::IsItemActive();		// スライダーを触ってるか（次のフレーム用）
			if (ImGui::IsItemDeactivatedAfterEdit())
				notes->SeekTo(seekPos);
		}
	}

	ImGui::End();
}
