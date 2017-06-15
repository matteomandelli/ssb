// SSB, v0.01 WIP
// (Window)

#pragma once

#include "window_renderer.hpp"

struct ImGuiContext;

struct WindowContext
{
    WNDCLASSEX                  m_wc;
    HWND                        m_hWnd = 0;
    UINT                        m_width = 100;
    UINT                        m_height = 100;
    INT64                       m_time = 0;
    INT64                       m_ticksPerSecond = 0;
    ImGuiContext*               m_pImGui;
};

class Window
{
public:
    Window() {}
    ~Window() {}

    // non copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    int                         Initialise(const char* title, int width, int height);
    int                         Update();
    int                         Render();
    int                         Terminate();

    bool                        IsClosed() { return m_close; }

private:
    static LRESULT CALLBACK     StaticWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                     WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT                     InputHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam);

    WindowContext               m_context;
    WindowRenderer              m_renderer;
    ID3D10ShaderResourceView*   m_pFontTexture = nullptr;
    MSG                         m_msg;
    bool                        m_open = true;
    bool                        m_render = false;
    bool                        m_close = false;
};
