#include "main.h"
#include "Conductor.h"
#include "Audio.h"
#include <cmath>

// static メンバーは cpp で実体を作る（Manager.cpp と同じ）
Audio* Conductor::m_Audio = nullptr;
bool      Conductor::m_Playing = false;
double    Conductor::m_SongTime = 0.0;
double    Conductor::m_Bpm = 120.0;
double    Conductor::m_Offset = 0.0;
long long Conductor::m_Freq = 0;
long long Conductor::m_LastCounter = 0;
bool      Conductor::m_WaitingAudio = false;


void Conductor::Init()
{
	LARGE_INTEGER f, c;
	QueryPerformanceFrequency(&f);   // タイマーの精度（1秒で何カウント進むか）
	QueryPerformanceCounter(&c);     // 今のカウント
	m_Freq = f.QuadPart;
	m_LastCounter = c.QuadPart;
}


void Conductor::Start(Audio* audio, double startSec)
{
	m_Audio = audio;
	m_SongTime = startSec;
	m_Playing = true;

	if (startSec < 0.0)
	{
		// マイナスから始める＝カウントイン。0 秒になったら Update で曲を鳴らし始める
		m_Audio->Pause();
		m_WaitingAudio = true;
	}
	else
	{
		m_Audio->PlayFrom(startSec);
		m_WaitingAudio = false;
	}
}

void Conductor::Stop()
{
	if (m_Audio) m_Audio->Pause();
	m_Audio = nullptr;
	m_Playing = false;
	m_WaitingAudio = false;
}

void Conductor::Pause()
{
	if (!m_Audio) return;
	m_Audio->Pause();
	m_Playing = false;
}

void Conductor::ReStart()
{
	if (!m_Audio) return;
	if (!m_WaitingAudio) m_Audio->Resume();   // カウントイン中はまだ曲を鳴らさない
	m_Playing = true;
}

double Conductor::GetRawAudioTime()
{
	return m_Audio ? m_Audio->GetPlaybackTime() : 0.0;
}

double Conductor::CounterToSongTime(long long counter)
{
	if (!m_Playing) return GetSongTime();

	// 最後に Update した瞬間（m_LastCounter）から、何秒ずれているか
	// キーを押したのが Update より前ならマイナスになる
	double sec = (counter - m_LastCounter) / (double)m_Freq;

	return m_SongTime + sec - m_Offset;
}


void Conductor::Update()
{
	// --- 1. 前のフレームから実際に何秒たったか ---
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	double realDt = (c.QuadPart - m_LastCounter) / (double)m_Freq;
	m_LastCounter = c.QuadPart;
	// ↑ 止まっている間もカウンタを更新しておく。
	//   こうしないと Resume した瞬間に「停止中の時間」が一気に足されてしまう

	if (!m_Audio || !m_Playing) return;

	// --- カウントイン中：自分の時計だけで進め、0 秒になったら曲を鳴らす ---
	if (m_WaitingAudio)
	{
		m_SongTime += realDt;
		if (m_SongTime >= 0.0)
		{
			m_Audio->PlayFrom(m_SongTime);
			m_WaitingAudio = false;
		}
		return;
	}

	// --- 2. 自分の時計で進めたらどうなるか（予測） ---
	double predicted = m_SongTime + realDt;

	// --- 3. 正解（音声の時刻）と比べる ---
	double audioTime = m_Audio->GetPlaybackTime();
	double diff = audioTime - predicted;   // ＋なら自分が遅れている、－なら進みすぎ

	if (std::fabs(diff) > 0.05)
	{
		// 50ms 以上ずれた（処理落ち・シーク直後など）→ 音声に一気に合わせる
		m_SongTime = audioTime;
	}
	else
	{
		// 小さなずれは 10% ずつ寄せる
		double step = realDt + diff * 0.1;
		if (step < 0.0) step = 0.0;   // 時間を巻き戻さない（ノーツが震える原因になる）
		m_SongTime += step;
	}
}