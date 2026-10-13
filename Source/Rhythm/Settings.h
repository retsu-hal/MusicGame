#pragma once

// プレイヤーの設定（settings.json に保存して、次に起動したときも使う）
class Settings
{
public:
	static inline float noteSpeed = 800.0f;   // ハイスピード（1秒で進む奥行き）
	static inline float offsetMs  = 0.0f;     // 判定・表示のずれ補正（ミリ秒）

	static void Load();
	static void Save();
};
