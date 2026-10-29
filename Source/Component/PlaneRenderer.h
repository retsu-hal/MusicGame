#pragma once
#include "Component.h"

struct ShaderSet;

class PlaneRenderer : public Component
{
private:
    const ShaderSet *m_Shader = nullptr;
    ID3D11ShaderResourceView *m_Texture = nullptr;
    ID3D11Buffer *m_VertexBuffer = nullptr;

    XMFLOAT4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};
    XMFLOAT4 m_UV = {0.0f, 0.0f, 1.0f, 1.0f};

public:
    using Component::Component;

    void Load(const wchar_t *textureFile);
    void Uninit() override;
    void Draw() override;

    void SetColor(const XMFLOAT4 &color) { m_Color = color; }
    void SetAlpha(float alpha) { m_Color.w = alpha; }
    void SetUV(float u, float v, float w, float h) { m_UV = {u, v, w, h}; }
};