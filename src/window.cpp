// SSB, v0.01 WIP
// (Window)

#include "window.hpp"
#include "logger.hpp"
#include "resource.h"
#include <imgui.h>

int Window::Initialise(const char* title, int width, int height)
{
    // Set timers
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&m_context.m_ticksPerSecond))
        return false;
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&m_context.m_time))
        return false;

    m_pTitle = strdup(title);

    m_context.m_width = width;
    m_context.m_height = height;

    m_context.m_wc.cbSize = sizeof(WNDCLASSEX);
    m_context.m_wc.style = CS_CLASSDC; //CS_HREDRAW | CS_VREDRAW;
    m_context.m_wc.lpfnWndProc = (WNDPROC)Window::StaticWndProc;
    m_context.m_wc.cbClsExtra = 0;
    m_context.m_wc.cbWndExtra = 0;
    m_context.m_wc.hInstance = GetModuleHandle(NULL);
    m_context.m_wc.hIcon = 0;
    m_context.m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    m_context.m_wc.hbrBackground = NULL; // (HBRUSH)(COLOR_WINDOW + 1);
    m_context.m_wc.lpszMenuName = NULL;
    m_context.m_wc.lpszClassName = TEXT(title);
    m_context.m_wc.hIconSm = 0;
    m_context.m_wc.hIcon = LoadIcon(m_context.m_wc.hInstance, MAKEINTRESOURCE(IDI_SSB));
    RegisterClassEx(&m_context.m_wc);

    //Resize the window
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    //create the window from the class above
    //disable resizing and correct for extra width and height
    m_context.m_hWnd = CreateWindow(title, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, m_context.m_wc.hInstance, this);

    //window handle not created
    if (!m_context.m_hWnd) 
        return -1;

    //if window creation was successful
    ShowWindow(m_context.m_hWnd, SW_SHOW);
    UpdateWindow(m_context.m_hWnd);

    m_renderer.Initialise(&m_context.m_hWnd);

    m_context.m_pImGui = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_context.m_pImGui);
    
    ImGuiIO& io = ImGui::GetIO();
    
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';
    
    // Build
    unsigned char* pixels;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);
    
    // Store our identifier
    m_pFontTexture = m_renderer.CreateTexture(pixels, texWidth, texHeight);
    io.Fonts->TexID = (void *)m_pFontTexture;
    
    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    return 0;
}

int Window::Terminate()
{
    ImGui::SetCurrentContext(m_context.m_pImGui);
    ImGui::GetIO().Fonts->TexID = nullptr;
    m_renderer.DestroyTexture(m_pFontTexture);
    m_pFontTexture = nullptr;
    m_renderer.Terminate();
    ImGui::Shutdown();
    ImGui::DestroyContext(m_context.m_pImGui);

    
    DestroyIcon(m_context.m_wc.hIcon);
    UnregisterClass(TEXT(m_pTitle), m_context.m_wc.hInstance);
    if (m_context.m_hWnd) DestroyWindow(m_context.m_hWnd);
    if (m_pTitle != nullptr) free(m_pTitle);

    return 0;
}

int Window::Update()
{
    if (m_close)
        return 0;

    m_render = true;

    if (m_msg.message == WM_QUIT)
    {
        m_render = false;
        m_close = true;
        return -1;
    }

    if (PeekMessage(&m_msg, NULL, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&m_msg);
        DispatchMessage(&m_msg);
        m_render = false;
        return 0;
    }

    ImGui::SetCurrentContext(m_context.m_pImGui);
    
    ImGuiIO& io = ImGui::GetIO();
    
    RECT rect;
    GetClientRect(m_context.m_hWnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    
    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
    io.DeltaTime = (float)(current_time - m_context.m_time) / m_context.m_ticksPerSecond;
    m_context.m_time = current_time;
    
    ImGui::NewFrame();
    
    // Draw

    ImGui::Begin("Test", &m_open, ImGuiWindowFlags_NoSavedSettings);
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
    ImGui::End();

    return 0;
}

int Window::Render()
{
    if (!m_render)
        return 0;

    ImGui::SetCurrentContext(m_context.m_pImGui);

    ImGui::Render();
    ImDrawData* pData = ImGui::GetDrawData();
    if (pData)
        m_renderer.Render(m_context.m_width, m_context.m_height, pData);

    return 0;
}


LRESULT CALLBACK Window::StaticWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window *pThis; // our "this" pointer will go here

    if (uMsg == WM_NCCREATE)
    {
        // Recover the "this" pointer which was passed as a parameter
        // to CreateWindow(Ex).
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Window*>(lpcs->lpCreateParams);
        // Put the value in a safe place for future use
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        // Recover the "this" pointer from where our WM_NCCREATE handler
        // stashed it.
        pThis = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        // Now that we have recovered our "this" pointer, let the
        // member function finish the job.
        return pThis->WndProc(hwnd, uMsg, wParam, lParam);
    }

    // We don't know what our "this" pointer is, so just do the default
    // thing. Hopefully, we didn't need to customize the behavior yet.
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (InputHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        {
            m_renderer.Resize(lParam);
            RECT rect;
            GetClientRect(m_context.m_hWnd, &rect);
            m_context.m_width = rect.right - rect.left;
            m_context.m_height = rect.bottom - rect.top;
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT Window::InputHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGui::SetCurrentContext(m_context.m_pImGui);
    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
    case WM_LBUTTONDOWN:
        io.MouseDown[0] = true;
        return true;
    case WM_LBUTTONUP:
        io.MouseDown[0] = false;
        return true;
    case WM_RBUTTONDOWN:
        io.MouseDown[1] = true;
        return true;
    case WM_RBUTTONUP:
        io.MouseDown[1] = false;
        return true;
    case WM_MBUTTONDOWN:
        io.MouseDown[2] = true;
        return true;
    case WM_MBUTTONUP:
        io.MouseDown[2] = false;
        return true;
    case WM_MOUSEWHEEL:
        io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
        return true;
    case WM_MOUSEMOVE:
        io.MousePos.x = (signed short)(lParam);
        io.MousePos.y = (signed short)(lParam >> 16);
        return true;
    case WM_KEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = 1;
        return true;
    case WM_KEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = 0;
        return true;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned short)wParam);
        return true;
    }
    return 0;
}
