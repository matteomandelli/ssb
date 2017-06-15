// SSB, v0.01 WIP
// (Window Renderer)

#pragma once

#include <d3d10.h>

struct ImDrawData;

class WindowRenderer
{
public:
    struct VertexConstantBuffer
    {
        float           m_mvp[4][4];
    };

public:
    WindowRenderer() {}
    ~WindowRenderer() {}

    int                         Initialise(HWND* hW);
    int                         Terminate();
    
    int                         Render(UINT width, UINT height, ImDrawData* draw_data);

    int                         Resize(LPARAM lParam);

    ID3D10ShaderResourceView*   CreateTexture(unsigned char* pixels, int width, int height);
    void                        DestroyTexture(ID3D10ShaderResourceView* pResource);

private:
    int                         CreateRenderTarget();
    int                         CleanupRenderTarget();

    int                         CreateVertexShader();
    int                         CreatePixelShader();
    
    HWND*                       m_hWnd;

    //device vars
    ID3D10Device*               m_pD3DDevice = nullptr;
    IDXGISwapChain*             m_pSwapChain = nullptr;
    ID3D10RenderTargetView*     m_pRenderTargetView = nullptr;
    
    ID3D10Blob *                m_pVertexShaderBlob = nullptr;
    ID3D10VertexShader*         m_pVertexShader = nullptr;
    ID3D10InputLayout*          m_pInputLayout = nullptr;
    ID3D10Buffer*               m_pVertexConstantBuffer = nullptr;

    ID3D10Blob *                m_pPixelShaderBlob = nullptr;
    ID3D10PixelShader*          m_pPixelShader = nullptr;
    
    ID3D10BlendState*           m_pBlendState = nullptr;

    ID3D10RasterizerState*      m_pRasterizerState = nullptr;

    ID3D10DepthStencilState*    m_pDepthStencilState = nullptr;

    ID3D10SamplerState*         m_pFontSampler = nullptr;

    ID3D10Buffer*               m_pVB = nullptr;
    ID3D10Buffer*               m_pIB = nullptr;
    int                         m_VertexBufferSize = 5000;
    int                         m_IndexBufferSize = 10000;
};
