#pragma once

#include "GameObject.h"

struct ShaderSet;

// 曲情報(右上)。ジャケット・曲名・難易度を枠の中に出す
class SongPanel : public GameObject
{
private:
	// 四角の番号(描く順。先に描くほど下になる)
	enum
	{
		QUAD_FRAME,        // 枠
		QUAD_JACKET,       // ジャケット
		QUAD_TITLE,        // 曲名
		QUAD_DIFF_LABEL,   // 「DIFFICULTY」
		QUAD_DIFF,         // 「EASY」「HARD」

		QUAD_NUM
	};

	ID3D11ShaderResourceView* m_Tex[QUAD_NUM] = {};   // 四角ごとの画像(TextureManager から借りる)
	const ShaderSet* m_Shader = nullptr;
	bool m_Visible = false;                           // 曲が読めたときだけ true
	bool m_ShowDiff = true;                           // ← 11/25 追加。false なら難易度の行を出さない(チュートリアル)

public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};