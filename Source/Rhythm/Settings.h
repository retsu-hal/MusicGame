#pragma once

// プレイヤーの設定（settings.json に保存して、次に起動したときも使う）
class Settings
{
public:
	static inline float noteSpeed = 800.0f;   // ハイスピード（1秒で進む奥行き）
	static inline float offsetMs = 0.0f;     // 判定のずれ補正（ミリ秒）  コメントだけ「判定・表示」→「判定」
	static inline bool  fullscreen = true;    // フルスクリーンで表示するか（F11 / Alt+Enter で切り替え）
	static inline float mouseSensitivity = 0.01f;   // アークのカーソルの感度(マウス 1 ピクセルで動くレーン数)

	static inline float visualOffsetMs = 0.0f;   // ノーツを描く位置のずれ補正（ミリ秒）。まだゲームでは使っていない
	static inline bool  layerGuide = true;   //  反対の段にノーツが来たら矢印で知らせる。まだ使っていない
	static inline bool  fastSlow = true;   //  PERFECT 以外のときに FAST / SLOW を出す。まだ使っていない
	static inline bool  showFps = false;   // 右上に FPS を出すか

	static inline int   masterVolume = 80;    // 全体の音量（0〜100）
	static inline int   musicVolume = 90;    //  曲・BGM の音量（0〜100）
	static inline int   seVolume = 70;    // タップ音・メニュー音の音量（0〜100）

	static void Load();
	static void Save();
};
