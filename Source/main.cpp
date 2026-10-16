

#include "main.h"
#include "Manager.h"
#include "Renderer.h"
#include "Settings.h"
#include <thread>
#include <algorithm>
#include <windowsx.h>




const char* CLASS_NAME = "AppClass";
const char* WINDOW_NAME = "Re:sonare";


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HWND g_Window;

bool            g_Fullscreen = false;
WINDOWPLACEMENT g_WindowPlacement = { sizeof(WINDOWPLACEMENT) };	// フルスクリーンから戻すときの位置と大きさ

HWND GetWindow()
{
	return g_Window;
}

bool IsFullscreen()
{
	return g_Fullscreen;
}

void SetFullscreen(bool Fullscreen)
{
	if (Fullscreen == g_Fullscreen) return;
	g_Fullscreen = Fullscreen;

	if (Fullscreen)
	{
		// 枠なしのウィンドウを、今いるモニターいっぱいに広げる
		GetWindowPlacement(g_Window, &g_WindowPlacement);

		MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
		GetMonitorInfo(MonitorFromWindow(g_Window, MONITOR_DEFAULTTONEAREST), &monitorInfo);
		const RECT& rc = monitorInfo.rcMonitor;

		SetWindowLongPtr(g_Window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(g_Window, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
	else
	{
		// 元のウィンドウに戻す
		SetWindowLongPtr(g_Window, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPlacement(g_Window, &g_WindowPlacement);
		SetWindowPos(g_Window, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

void ToggleFullscreen()
{
	SetFullscreen(!g_Fullscreen);

	Settings::fullscreen = g_Fullscreen;	// 次に起動したときも同じ表示にする
	Settings::Save();
}

// マウスの座標（ウィンドウの中身の座標）を、ゲーム画面（SCREEN_WIDTH x SCREEN_HEIGHT）の座標に直す
static LPARAM ToGameMousePosition(UINT uMsg, LPARAM lParam)
{
	bool hasClientPosition =
		(uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && uMsg != WM_MOUSEWHEEL && uMsg != WM_MOUSEHWHEEL)
		|| uMsg == WM_MOUSEHOVER;
	if (!hasClientPosition) return lParam;

	XMFLOAT2 pos = Renderer::ClientToGame(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	return MAKELPARAM((short)pos.x, (short)pos.y);
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{


	WNDCLASSEX wcex;
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = nullptr;

		RegisterClassEx(&wcex);


		// ウィンドウモードの大きさ：作業領域に収まる16:9（最大 SCREEN_WIDTH x SCREEN_HEIGHT）
		RECT workArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
		float scale = std::min({
			(workArea.right - workArea.left) * 0.9f / SCREEN_WIDTH,
			(workArea.bottom - workArea.top) * 0.9f / SCREEN_HEIGHT,
			1.0f });

		RECT rc = { 0, 0, (LONG)(SCREEN_WIDTH * scale), (LONG)(SCREEN_HEIGHT * scale) };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		g_Window = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	}

	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	Manager::Init();



	ShowWindow(g_Window, nCmdShow);
	UpdateWindow(g_Window);

	// 前回の設定（初回はフルスクリーン）で表示する
	SetFullscreen(Settings::fullscreen);




	DWORD dwExecLastTime;
	DWORD dwCurrentTime;
	timeBeginPeriod(1);
	dwExecLastTime = timeGetTime();
	dwCurrentTime = 0;



	MSG msg;
	while(1)
	{
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
        }
		else
		{
			dwCurrentTime = timeGetTime();

			if((dwCurrentTime - dwExecLastTime) >= (1000 / 60))
			{
				dwExecLastTime = dwCurrentTime;

				Manager::Update();
				Manager::Draw();
			}
		}
	}

	timeEndPeriod(1);

	UnregisterClass(CLASS_NAME, wcex.hInstance);

	Manager::Uninit();

	CoUninitialize();

	return (int)msg.wParam;
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// ImGui と Input には、ゲーム画面の座標でマウス位置を渡す
	LPARAM gameLParam = ToGameMousePosition(uMsg, lParam);

#if  _DEBUG
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, gameLParam))
		return true;

#endif //  _DEBUG

	
	switch(uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_ACTIVATEAPP:
        break;

	case WM_SIZE:
		Renderer::Resize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SIZING:
	{
		// 枠をドラッグしても、中身が16:9のままになるようにする
		RECT* rc = (RECT*)lParam;
		RECT frame = { 0, 0, 0, 0 };
		AdjustWindowRect(&frame, WS_OVERLAPPEDWINDOW, FALSE);
		LONG frameW = frame.right - frame.left;
		LONG frameH = frame.bottom - frame.top;

		LONG clientW = (rc->right - rc->left) - frameW;
		LONG clientH = (rc->bottom - rc->top) - frameH;
		if (wParam == WMSZ_TOP || wParam == WMSZ_BOTTOM)
			clientW = clientH * SCREEN_WIDTH / SCREEN_HEIGHT;	// 上下の辺：高さに合わせて幅を決める
		else
			clientH = clientW * SCREEN_HEIGHT / SCREEN_WIDTH;	// それ以外：幅に合わせて高さを決める

		if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
			rc->top = rc->bottom - (clientH + frameH);
		else
			rc->bottom = rc->top + (clientH + frameH);

		if (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT)
			rc->left = rc->right - (clientW + frameW);
		else
			rc->right = rc->left + (clientW + frameW);
		return TRUE;
	}

    case WM_SYSKEYDOWN:
		// Alt+Enter：フルスクリーン切り替え（押しっぱなしの連続入力は無視）
		if (wParam == VK_RETURN && (lParam & (1 << 29)) && !(lParam & (1 << 30)))
		{
			ToggleFullscreen();
			return 0;
		}
		break;

	case WM_SYSCHAR:
		if (wParam == VK_RETURN) return 0;	// Alt+Enter の「ポン」という警告音を鳴らさない
		break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        break;

    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
		Input::ProcessMessage(uMsg, wParam, gameLParam);
        break;

	case WM_KEYDOWN:
		Input::OnKeyDown(wParam,lParam);
		switch (wParam)
		{
		case VK_ESCAPE:
			break;

		case VK_F11:	// フルスクリーン切り替え（押しっぱなしの連続入力は無視）
			if (!(lParam & (1 << 30))) ToggleFullscreen();
			break;
		}
		break;
	
	case WM_CLOSE:	
		if (
			MessageBoxW(hWnd, L"本当に終了しますか？", L"確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK
			)
		{
			DestroyWindow(hWnd);
		}
		else
		{
			return 0;
		}
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

