#include "main.h"
#include "Manager.h"
#include "Input.h"
#include "Polygon2D.h"
#include "Audio.h"
#include "Pause.h"

// メニューの帯(=クリックの当たり範囲)。タイトルと同じ大きさで、画面の真ん中に置く
#define CURSOR_W (660.0f)
#define CURSOR_H (72.0f)
#define CURSOR_X ((SCREEN_WIDTH - CURSOR_W) * 0.5f) // 630
#define CURSOR_Y (471.0f)                           // 1つ目の項目の帯の上端
#define MENU_PITCH (84.0f)                          // 項目の間隔(タイトルと同じ)
#define MENU_X (CURSOR_X + 40.0f)                   // 文字の左端(帯の左から 40。タイトルと同じずらし方)
#define MENU_Y (CURSOR_Y + 9.0f)                    // 文字の上端

#define COUNT_STEP (0.6f)   // カウント1つぶんの時間(秒)★
#define COUNT_SIZE (256.0f) // 数字の大きさ

static const XMFLOAT4 COLOR_SELECTED = {0.949f, 0.941f, 0.980f, 1.0f}; // #F2F0FA 選択中
static const XMFLOAT4 COLOR_NORMAL = {0.541f, 0.525f, 0.659f, 1.0f};   // #8A86A8 選択していない
static const XMFLOAT4 COLOR_ACCENT = {0.435f, 0.890f, 1.0f, 1.0f};     // #6FE3FF 強調(水色)
static const XMFLOAT4 COLOR_CURSOR = {0.486f, 0.424f, 1.0f, 0.25f};    // 帯のうす紫(半透明)

void PauseMenu::Init()
{
    // 効果音(表示しない GameObject に Audio を付ける。タイトルと同じやり方)
    GameObject *sound = Manager::AddGameObject<GameObject>();
    m_SelectSE = sound->AddComponent<Audio>();
    m_SelectSE->Load("asset\\audio\\se\\se_select.wav", Audio::TYPE_SE);
    m_DecideSE = sound->AddComponent<Audio>();
    m_DecideSE->Load("asset\\audio\\se\\se_click.wav", Audio::TYPE_SE);

    // 後ろを暗くする板
    m_Dim = Manager::AddGameObject<Polygon2D>();
    m_Dim->Init(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, L"asset\\texture\\ui\\assets\\white_square.png");
    m_Dim->SetColor(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

    // 「PAUSE」(284 × 96 を真ん中に)
    m_Pause = Manager::AddGameObject<Polygon2D>();
    m_Pause->Init((SCREEN_WIDTH - 284.0f) * 0.5f, 300.0f, 284.0f, 96.0f, L"asset\\texture\\ui\\text\\pause\\pause.png");
    m_Pause->SetColor(COLOR_SELECTED);

    // 選択中の帯と線(文字より先に足して、文字の下に描く)
    m_Cursor = Manager::AddGameObject<Polygon2D>();
    m_Cursor->Init(CURSOR_X, CURSOR_Y, CURSOR_W, CURSOR_H, L"asset\\texture\\ui\\assets\\white_square.png");
    m_Cursor->SetColor(COLOR_CURSOR);
    m_Cursor->SetGradientAlpha(1.0f, 0.0f); // 左はそのまま、右端で消える

    m_CursorLine = Manager::AddGameObject<Polygon2D>();
    m_CursorLine->Init(CURSOR_X + 12.0f, MENU_Y + 3.0f, 6.0f, 48.0f, L"asset\\texture\\ui\\assets\\white_square.png");
    m_CursorLine->SetColor(COLOR_ACCENT);

    // 項目の文字
    const WCHAR *textFile[MENU_MAX] = {
        L"asset\\texture\\ui\\text\\menu\\continue.png",
        L"asset\\texture\\ui\\text\\menu\\retry.png",
        L"asset\\texture\\ui\\text\\menu\\select.png"};
    const float textWidth[MENU_MAX] = {228.0f, 147.0f, 171.0f};

    for (int i = 0; i < MENU_MAX; i++)
    {
        m_MenuText[i] = Manager::AddGameObject<Polygon2D>();
        m_MenuText[i]->Init(MENU_X, MENU_Y + MENU_PITCH * i, textWidth[i], 60.0f, textFile[i]);
    }

    // キーガイド(画面の左下。タイトルと同じ位置)
    m_Guide[0] = Manager::AddGameObject<Polygon2D>();
    m_Guide[0]->Init(66.0f, 1005.0f, 150.0f, 78.0f, L"asset\\texture\\ui\\keyguide\\kg_click_decide.png");
    m_Guide[1] = Manager::AddGameObject<Polygon2D>();
    m_Guide[1]->Init(66.0f + 150.0f + 30.0f, 1005.0f, 159.0f, 78.0f, L"asset\\texture\\ui\\keyguide\\kg_esc_resume.png");

    // 3・2・1 の数字(Score・Combo と同じ数字のスプライト。4 × 4 マス)
    m_Count = Manager::AddGameObject<Polygon2D>();
    m_Count->Init((SCREEN_WIDTH - COUNT_SIZE) * 0.5f, (SCREEN_HEIGHT - COUNT_SIZE) * 0.5f,
                  COUNT_SIZE, COUNT_SIZE, L"asset\\texture\\number_atlas_512.png");

    // 全部レイヤー3(判定ライン・スコアと同じ)。GameScene で一番最後に足すので、一番上に描かれる
    for (Polygon2D *p : {m_Dim, m_Pause, m_Cursor, m_CursorLine,
                         m_MenuText[0], m_MenuText[1], m_MenuText[2],
                         m_Guide[0], m_Guide[1], m_Count})
        p->SetLayer(3);

    Close(); // 最初は隠しておく
}

void PauseMenu::Open()
{
    m_State = STATE_MENU;
    m_Select = MENU_CONTINUE; // 開いたときは CONTINUE を選んでおく

    m_Dim->SetAlpha(0.6f); // プレイ画面を暗く ★
    ShowMenu(true);
    m_Count->SetAlpha(0.0f);
    UpdateMenuLook();
}

void PauseMenu::Close()
{
    m_State = STATE_CLOSED;

    m_Dim->SetAlpha(0.0f);
    ShowMenu(false);
    m_Count->SetAlpha(0.0f);
}

// メニューの部品(カウントの数字と暗い板以外)をまとめて出す / 消す
void PauseMenu::ShowMenu(bool show)
{
    float a = show ? 1.0f : 0.0f;
    m_Pause->SetAlpha(a);
    m_Cursor->SetAlpha(show ? COLOR_CURSOR.w : 0.0f); // 帯だけ元が半透明なので、元の濃さに戻す
    m_CursorLine->SetAlpha(a);
    for (Polygon2D *text : m_MenuText)
        text->SetAlpha(a);
    for (Polygon2D *guide : m_Guide)
        guide->SetAlpha(a);
}

PauseMenu::RESULT PauseMenu::Update()
{
    // --- カウント中:3・2・1 を出して、終わったら スタート ---
    if (m_State == STATE_COUNT)
    {
        m_CountTime += Manager::GetDeltaTime();

        int n = 3 - (int)(m_CountTime / COUNT_STEP); // 3 → 2 → 1 → 0
        if (n <= 0)
        {
            Close();
            return RESULT_RESTART;
        }

        // 数字のマス(1行目の n マス目)を貼る
        m_Count->SetUV((n % 4) * 0.25f, (n / 4) * 0.25f, 0.25f, 0.25f);
        m_Count->SetAlpha(1.0f);

        // 暗い板を、カウントに合わせて少しずつ明るくする(プレイ画面が見えてくる)
        float t = m_CountTime / (COUNT_STEP * 3.0f); // 0 → 1
        m_Dim->SetAlpha(0.6f * (1.0f - t));
        return RESULT_NONE;
    }

    if (m_State != STATE_MENU)
        return RESULT_NONE; // 閉じているときは何もしない

    // --- メニュー中 ---

    // ホバーで選ぶ
    int hover = HitMenu(Input::GetMouseX(), Input::GetMouseY());
    if (hover >= 0 && hover != m_Select)
    {
        m_Select = hover;
        UpdateMenuLook();
        m_SelectSE->Play();
    }

    // 決まった項目(まだなら -1)
    int decide = -1;

    // 項目の上で左クリック
    if (Input::GetMouseTrigger(Input::MOUSE_LEFT))
        decide = HitMenu(Input::GetMouseX(), Input::GetMouseY());

    // Esc は CONTINUE と同じ
    if (Input::GetKeyTrigger(VK_ESCAPE))
        decide = MENU_CONTINUE;

    switch (decide)
    {
    case MENU_CONTINUE:
        m_DecideSE->Play();
        m_State = STATE_COUNT; // すぐには戻さず、カウントへ
        m_CountTime = 0.0f;
        ShowMenu(false);
        return RESULT_NONE;

    case MENU_RETRY:
        m_DecideSE->Play();
        Close();
        return RESULT_RETRY;

    case MENU_SELECT:
        m_DecideSE->Play();
        return RESULT_SELECT; // 開いたまま(フェードで切り替わるまで見せておく)

    default:
        return RESULT_NONE;
    }
}

void PauseMenu::UpdateMenuLook()
{
    for (int i = 0; i < MENU_MAX; i++)
        m_MenuText[i]->SetColor(i == m_Select ? COLOR_SELECTED : COLOR_NORMAL);

    // 帯と線を選んでいる行へ動かす(Init で置いた CONTINUE の行からのずらし量)
    Vector3 offset(0.0f, MENU_PITCH * m_Select, 0.0f);
    m_Cursor->SetPosition(offset);
    m_CursorLine->SetPosition(offset);
}

int PauseMenu::HitMenu(int x, int y)
{
    for (int i = 0; i < MENU_MAX; i++)
    {
        float left = CURSOR_X;
        float top = CURSOR_Y + MENU_PITCH * i; // i 行目の帯の上端
        float right = left + CURSOR_W;
        float bottom = top + CURSOR_H;

        if (x >= left && x < right && y >= top && y < bottom)
            return i;
    }
    return -1;
}