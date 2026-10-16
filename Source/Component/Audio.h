#pragma once

#include <xaudio2.h>
#include "Component.h"


class Audio : public Component
{
private:
	static IXAudio2*				m_Xaudio;
	static IXAudio2MasteringVoice*	m_MasteringVoice;
	static IXAudio2SubmixVoice* m_MusicSubmix;		
	static IXAudio2SubmixVoice* m_SeSubmix;			

	IXAudio2SourceVoice*	m_SourceVoice{};
	BYTE*					m_SoundData{};

	int						m_Length{};
	int						m_PlayLength{};

	int 						m_SampleRate{};
	UINT64 				m_BaseSamples{};
	int						m_StartSample{};


public:
	enum TYPE
	{
		TYPE_MUSIC,		// 曲・BGM
		TYPE_SE,		// タップ音・メニュー音
	};
	static void InitMaster();
	static void UninitMaster();
	static void ApplyVolume();

	using Component::Component;

	void Uninit() override;

	void Load(const char* FileName, TYPE type);
	void Play(bool Loop = false);
	void PlayFrom(double startSec);
	void Pause();
	void Resume();
	double GetPlaybackTime() const;
	double GetLength() const { return m_SampleRate ? (double)m_PlayLength / m_SampleRate : 0.0; }   // 曲の長さ（秒）

	void SetVolume(float volume) { m_SourceVoice->SetVolume(volume); }


};

