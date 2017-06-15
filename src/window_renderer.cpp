// SSB, v0.01 WIP
// (Window Renderer)

#include "window_renderer.hpp"
#include "logger.hpp"
#include <imgui.h>
#include <d3dcompiler.h>

int WindowRenderer::Initialise(HWND* hW)
{
    m_hWnd = hW;

    //get window dimensions
    RECT rc;
    GetClientRect(*m_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    //Set up DX swap chain
    //--------------------------------------------------------------
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

    //set buffer dimensions and format
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;;

    //set refresh rate
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

    //sampling settings
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SampleDesc.Count = 1;

    //swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    //swapChainDesc.Flags = 2048;

    //output window handle
    swapChainDesc.OutputWindow = *m_hWnd;
    swapChainDesc.Windowed = true;

    //Create the D3D device
    HRESULT ret = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &m_pD3DDevice);
    if ( ret != S_OK) 
        return -1;

    CreateRenderTarget();

    return 0;
}

int WindowRenderer::Terminate()
{
    if (!m_pD3DDevice)
        return 0;
    
    CleanupRenderTarget();

    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = NULL; }
    if (m_pD3DDevice) { m_pD3DDevice->Release(); m_pD3DDevice = NULL; }
    
    return 0;
}

int WindowRenderer::Render(UINT width, UINT height, ImDrawData* draw_data)
{
    ImVec4 clear_col = ImColor( 64, 64, 64 );
    m_pD3DDevice->ClearRenderTargetView(m_pRenderTargetView, (float*)&clear_col);

    if (draw_data == nullptr)
    {
        m_pSwapChain->Present(0, 0);
        return 0;
    }

    ID3D10Device* ctx = m_pD3DDevice;

    // Create and grow vertex/index buffers if needed
    if (!m_pVB || m_VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
        m_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D10_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D10_BUFFER_DESC));
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.ByteWidth = m_VertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (ctx->CreateBuffer(&desc, NULL, &m_pVB) < 0)
            return -1;
    }

    if (!m_pIB || m_IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (m_pIB) { m_pIB->Release(); m_pIB = NULL; }
        m_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D10_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D10_BUFFER_DESC));
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.ByteWidth = m_IndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        if (ctx->CreateBuffer(&desc, NULL, &m_pIB) < 0)
            return -1;
    }

    // Copy and convert all vertices into a single contiguous buffer
    ImDrawVert* vtx_dst = NULL;
    ImDrawIdx* idx_dst = NULL;
    m_pVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&vtx_dst);
    m_pIB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&idx_dst);
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    m_pVB->Unmap();
    m_pIB->Unmap();

    // Setup orthographic projection matrix into our constant buffer
    {
        void* mapped_resource;
        if (m_pVertexConstantBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
            return -1;
        VertexConstantBuffer* constant_buffer = (VertexConstantBuffer*)mapped_resource;
        const float L = 0.0f;
        const float R = (float)width;
        const float B = (float)height;
        const float T = 0.0f;
        const float mvp[4][4] =
        {
            { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
        };
        memcpy(&constant_buffer->m_mvp, mvp, sizeof(mvp));
        m_pVertexConstantBuffer->Unmap();
    }

    // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
    struct BACKUP_DX10_STATE
    {
        UINT                        ScissorRectsCount, ViewportsCount;
        D3D10_RECT                  ScissorRects[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D10_VIEWPORT              Viewports[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D10RasterizerState*      RS;
        ID3D10BlendState*           BlendState;
        FLOAT                       BlendFactor[4];
        UINT                        SampleMask;
        UINT                        StencilRef;
        ID3D10DepthStencilState*    DepthStencilState;
        ID3D10ShaderResourceView*   PSShaderResource;
        ID3D10SamplerState*         PSSampler;
        ID3D10PixelShader*          PS;
        ID3D10VertexShader*         VS;
        D3D10_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
        ID3D10Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
        UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
        DXGI_FORMAT                 IndexBufferFormat;
        ID3D10InputLayout*          InputLayout;
    };
    BACKUP_DX10_STATE old;
    old.ScissorRectsCount = old.ViewportsCount = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
    ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
    ctx->RSGetState(&old.RS);
    ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
    ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
    ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
    ctx->PSGetSamplers(0, 1, &old.PSSampler);
    ctx->PSGetShader(&old.PS);
    ctx->VSGetShader(&old.VS);
    ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
    ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
    ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    ctx->IAGetInputLayout(&old.InputLayout);

    // Setup viewport
    D3D10_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D10_VIEWPORT));
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    ctx->RSSetViewports(1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    ctx->IASetInputLayout(m_pInputLayout);
    ctx->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
    ctx->IASetIndexBuffer(m_pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(m_pVertexShader);
    ctx->VSSetConstantBuffers(0, 1, &m_pVertexConstantBuffer);
    ctx->PSSetShader(m_pPixelShader);
    ctx->PSSetSamplers(0, 1, &m_pFontSampler);

    // Setup render state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendState(m_pBlendState, blend_factor, 0xffffffff);
    ctx->OMSetDepthStencilState(m_pDepthStencilState, 0);
    ctx->RSSetState(m_pRasterizerState);

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const D3D10_RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };
                ctx->PSSetShaderResources(0, 1, (ID3D10ShaderResourceView**)&pcmd->TextureId);
                ctx->RSSetScissorRects(1, &r);
                ctx->DrawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore modified DX state
    ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
    ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
    ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
    ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
    ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
    ctx->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
    ctx->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
    ctx->PSSetShader(old.PS); if (old.PS) old.PS->Release();
    ctx->VSSetShader(old.VS); if (old.VS) old.VS->Release();
    ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
    ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
    ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
    ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
    ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();

    m_pSwapChain->Present(0, 0);
    
    return 0;
}

int WindowRenderer::Resize(LPARAM lParam)
{
    if (m_pSwapChain)
    {
        CleanupRenderTarget();
        m_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
    }
    return 0;
}

int WindowRenderer::CreateVertexShader()
{
    static const char* vertexShader =
        "cbuffer vertexBuffer : register(b0) \
            {\
            float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
            float2 pos : POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
            PS_INPUT output;\
            output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
            output.col = input.col;\
            output.uv  = input.uv;\
            return output;\
            }";

    D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &m_pVertexShaderBlob, NULL);
    if (m_pVertexShaderBlob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        return -1;
    if (m_pD3DDevice->CreateVertexShader((DWORD*)m_pVertexShaderBlob->GetBufferPointer(), m_pVertexShaderBlob->GetBufferSize(), &m_pVertexShader) != S_OK)
        return -1;

    // Create the input layout
    D3D10_INPUT_ELEMENT_DESC local_layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (m_pD3DDevice->CreateInputLayout(local_layout, 3, m_pVertexShaderBlob->GetBufferPointer(), m_pVertexShaderBlob->GetBufferSize(), &m_pInputLayout) != S_OK)
        return -1;

    // Create the constant buffer
    {
        D3D10_BUFFER_DESC desc;
        desc.ByteWidth = sizeof(VertexConstantBuffer);
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        m_pD3DDevice->CreateBuffer(&desc, NULL, &m_pVertexConstantBuffer);
    }

    return 0;
}

int WindowRenderer::CreatePixelShader()
{
    static const char* pixelShader =
        "struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
            return out_col; \
            }";

    D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &m_pPixelShaderBlob, NULL);
    if (m_pPixelShaderBlob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        return -1;
    if (m_pD3DDevice->CreatePixelShader((DWORD*)m_pPixelShaderBlob->GetBufferPointer(), m_pPixelShaderBlob->GetBufferSize(), &m_pPixelShader) != S_OK)
        return -1;

    return 0;
}

ID3D10ShaderResourceView* WindowRenderer::CreateTexture(unsigned char* pixels, int width, int height)
{
    ID3D10ShaderResourceView* pFontTextureView;
    // Create DX10 texture
    {
        D3D10_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D10_USAGE_DEFAULT;
        desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D10Texture2D *pTexture = NULL;
        D3D10_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        m_pD3DDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
        ZeroMemory(&srv_desc, sizeof(srv_desc));
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = desc.MipLevels;
        srv_desc.Texture2D.MostDetailedMip = 0;
        m_pD3DDevice->CreateShaderResourceView(pTexture, &srv_desc, &pFontTextureView);
        pTexture->Release();
    }

    return pFontTextureView;
}

void WindowRenderer::DestroyTexture(ID3D10ShaderResourceView* pResource)
{
    if (pResource) { pResource->Release(); }
}

int WindowRenderer::CreateRenderTarget()
{
    //Create Render Target
    {
        DXGI_SWAP_CHAIN_DESC sd;
        m_pSwapChain->GetDesc(&sd);

        // Create the render target
        ID3D10Texture2D* pBackBuffer;
        D3D10_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
        render_target_view_desc.Format = sd.BufferDesc.Format;
        render_target_view_desc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
        HRESULT res = m_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
        m_pD3DDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &m_pRenderTargetView);
        m_pD3DDevice->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);
        pBackBuffer->Release();
    }

    //Create Shaders
    CreateVertexShader();
    CreatePixelShader();

    // Create the blending setup
    {
        D3D10_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.BlendEnable[0] = true;
        desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
        desc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
        desc.BlendOp = D3D10_BLEND_OP_ADD;
        desc.SrcBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
        desc.DestBlendAlpha = D3D10_BLEND_ZERO;
        desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
        desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
        m_pD3DDevice->CreateBlendState(&desc, &m_pBlendState);
    }

    // Create the rasterizer state
    {
        D3D10_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D10_FILL_SOLID;
        desc.CullMode = D3D10_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
        m_pD3DDevice->CreateRasterizerState(&desc, &m_pRasterizerState);
    }

    // Create depth-stencil State
    {
        D3D10_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D10_COMPARISON_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        m_pD3DDevice->CreateDepthStencilState(&desc, &m_pDepthStencilState);
    }

    // Create texture sampler
    {
        D3D10_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        m_pD3DDevice->CreateSamplerState(&desc, &m_pFontSampler);
    }
    return 0;
}

int WindowRenderer::CleanupRenderTarget()
{
    if (m_pIB) { m_pIB->Release(); m_pIB = nullptr; }
    if (m_pVB) { m_pVB->Release(); m_pVB = nullptr; }

    if (m_pFontSampler) { m_pFontSampler->Release(); m_pFontSampler = nullptr; }

    if (m_pDepthStencilState) { m_pDepthStencilState->Release(); m_pDepthStencilState = nullptr; }

    if (m_pRasterizerState) { m_pRasterizerState->Release(); m_pRasterizerState = nullptr; }

    if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }

    if (m_pVertexConstantBuffer) { m_pVertexConstantBuffer->Release(); m_pVertexConstantBuffer = nullptr; }
    if (m_pPixelShaderBlob) { m_pPixelShaderBlob->Release(); m_pPixelShaderBlob = nullptr; }
    if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = nullptr; }

    if (m_pInputLayout) { m_pInputLayout->Release(); m_pInputLayout = nullptr; }
    if (m_pVertexShaderBlob) { m_pVertexShaderBlob->Release(); m_pVertexShaderBlob = nullptr; }
    if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = nullptr; }

    if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = nullptr; }
    return 0;
}
