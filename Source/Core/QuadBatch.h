#pragma once
#include "main.h"
#include "Renderer.h"
#include <vector>

struct ShaderSet;

// 四角形をためて、まとめて描く(全員で1個を使い回す)
class QuadBatch
{
private:
	static ID3D11Buffer* m_Buffer;
	static std::vector<VERTEX_3D> m_Vertices;
	static int                    m_MaxVertices;
	static const ShaderSet* m_Shader;

public:
	static void Init(int maxQuads);
	static void Uninit();

	// 角は「画面で見て 左上 → 右上 → 右下 → 左下」の順。
	// 色は上の辺と下の辺で変えられる(床なら 上 = 奥、下 = 手前)
	static void Quad(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT3& p3,
					const XMFLOAT4& topColor, const XMFLOAT4& bottomColor,
					XMFLOAT2 uvMin = XMFLOAT2(0.0f, 0.0f), XMFLOAT2 uvMax = XMFLOAT2(1.0f, 1.0f));

	// ためた四角形を描いて空にする(自分の Draw の最後で必ず呼ぶ)
	static void Flush(ID3D11ShaderResourceView* texture, bool add = false);
};