#include "Auto.h"
#include "Windows.h"
#include "iostream"
#include <string.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <vmmdll.h>
#include <tchar.h>
#include <cstdio>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
VMMDLL_SCATTER_HANDLE hScatter = 0;
VMM_HANDLE handle;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

//######################变量定义
ULONG64 qProcessBase = NULL;
DWORD pid;
DWORD cRead;
PVMMDLL_MAP_MODULEENTRY 模块地址;
struct guge
{
    float x;
    float y;
    float z;
};
struct v3d
{
    float x;
    float y;
    float z;
};
struct v4d
{
    float x;
    float y;
    float z;
    float w;
};
int Width, Hight;
v3d 游戏坐标;
v4d 窗口转换坐标;
guge 骨骼坐标;
float 矩阵数组[4][4];
POINT 坐标;
float buffer[1355] ;
ULONG64 缓冲区, 缓冲区1, 缓冲区2, 缓冲区3;
float d3d矩阵z, d3d矩阵x, d3d矩阵y, d3d矩阵y2, 缩放比例;
int hp, team, myteam, myhp;
DWORD bytesRead = 0;
HANDLE  游戏进程句柄;
ULONG64 addresses_strcture[50];
ULONG64 addresses_guge[50];




// 绘制骨骼线条的函数
bool 骨骼绘制(int i, int j) 
{
    float 骨骼世界坐标起始x, 骨骼世界坐标起始y, 骨骼世界坐标结束x, 骨骼世界坐标结束y, 相机z;

    // 计算起始点
   
    // 从 buffer 中每隔 16 字节提取一个浮点数
    //memcpy(&骨骼坐标.x, buffer + i * 0x30, sizeof(float)); // 每隔 16 字节提取 4 字节
    //memcpy(&骨骼坐标.y, buffer + i * 0x30 + 0x10, sizeof(float)); // 每隔 16 字节提取 4 字节
    //memcpy(&骨骼坐标.z, buffer + i * 0x30 + 0x20, sizeof(float)); // 每隔 16 字节提取 4 字节
    骨骼坐标.x = buffer[i * 12];
    骨骼坐标.y = buffer[i * 12 + 4];
    骨骼坐标.z = buffer[i * 12 + 8];
    相机z = 骨骼坐标.x * 矩阵数组[2][0] + 骨骼坐标.y * 矩阵数组[2][1] + 骨骼坐标.z * 矩阵数组[2][2] + 矩阵数组[2][3];
    if (相机z < 0) {
        return false;
    }

    骨骼世界坐标起始x = 1280 +
        (矩阵数组[0][0] * 骨骼坐标.x + 矩阵数组[0][1] * 骨骼坐标.y + 矩阵数组[0][2] * 骨骼坐标.z + 矩阵数组[0][3]) /
        相机z * 1280;

    骨骼世界坐标起始y = 720 -
        (矩阵数组[1][0] * 骨骼坐标.x + 矩阵数组[1][1] * 骨骼坐标.y + 矩阵数组[1][2] * 骨骼坐标.z + 矩阵数组[1][3]) /
        相机z * 720;

    // 计算结束点
    骨骼坐标.x = buffer[j * 12];
    骨骼坐标.y = buffer[j * 12 + 4];
    骨骼坐标.z = buffer[j * 12 + 8];
    //memcpy(&骨骼坐标.x, buffer + j * 0x30, sizeof(float)); // 每隔 16 字节提取 4 字节
    //memcpy(&骨骼坐标.y, buffer + j * 0x30 + 0x10, sizeof(float)); // 每隔 16 字节提取 4 字节
    //memcpy(&骨骼坐标.z, buffer + j * 0x30 + 0x20, sizeof(float)); // 每隔 16 字节提取 4 字节

    相机z = 骨骼坐标.x * 矩阵数组[2][0] + 骨骼坐标.y * 矩阵数组[2][1] + 骨骼坐标.z * 矩阵数组[2][2] + 矩阵数组[2][3];
    if (相机z < 0) {
        return false;
    }

    骨骼世界坐标结束x = 1280 +
        (矩阵数组[0][0] * 骨骼坐标.x + 矩阵数组[0][1] * 骨骼坐标.y + 矩阵数组[0][2] * 骨骼坐标.z + 矩阵数组[0][3]) /
        相机z * 1280;

    骨骼世界坐标结束y = 720 -
        (矩阵数组[1][0] * 骨骼坐标.x + 矩阵数组[1][1] * 骨骼坐标.y + 矩阵数组[1][2] * 骨骼坐标.z + 矩阵数组[1][3]) /
        相机z * 720;

    // 绘制线条
    ImGui::GetForegroundDrawList()->AddLine(
        ImVec2(骨骼世界坐标起始x, 骨骼世界坐标起始y),
        ImVec2(骨骼世界坐标结束x, 骨骼世界坐标结束y),
        ImColor(255, 0, 0, 255)
    );

    return true;
}


void 地址载入()
{
    DWORD cRead;
    // 读取矩阵和其他数据
    VMMDLL_MemReadEx(handle, pid, (ULONG64)qProcessBase + 0x186DE00, (PBYTE)&缓冲区3, sizeof(DWORD64), &cRead, 0);  //自身结构体地址
    for (int i = 0; i < 50; i++)
    {
       
        //ReadProcessMemory();
        VMMDLL_MemReadEx(handle, pid, (ULONG64)qProcessBase + 0x187B0D8, (PBYTE)&缓冲区, sizeof(DWORD64), &cRead, 0);
        VMMDLL_MemReadEx(handle, pid, (ULONG64)缓冲区 + i * 0x8, (PBYTE)&缓冲区, sizeof(DWORD64), &cRead, 0); //遍历人结构体地址
        addresses_strcture[i] = 缓冲区;
        //std::cout << std::dec<< hp << std::endl;
        VMMDLL_MemReadEx(handle, pid, (ULONG64)缓冲区 + 0x328, (PBYTE)&缓冲区2, sizeof(DWORD64), &cRead, 0);
        VMMDLL_MemReadEx(handle, pid, (ULONG64)缓冲区2 + 0x248, (PBYTE)&缓冲区2, sizeof(DWORD64), &cRead, 0);
        VMMDLL_MemReadEx(handle, pid, (ULONG64)缓冲区2 + 0x20, (PBYTE)&缓冲区2, sizeof(DWORD64), &cRead, 0);
        VMMDLL_MemReadEx(handle, pid, (ULONG64)缓冲区2 + 0xd8, (PBYTE)&缓冲区2, sizeof(DWORD64), &cRead, 0);
        addresses_guge[i] = 缓冲区2 + 0xc;

    }
    
}
 
void 骨骼()
{
    VMMDLL_MemReadEx(handle, pid, (QWORD)qProcessBase + 0x1A8C3A8, (PBYTE)&矩阵数组, 0x40, &cRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
    //std::cout << 矩阵数组[0][0] << std::endl;
    VMMDLL_MemReadEx(handle, pid, (QWORD)缓冲区3 + 0xab4, (PBYTE)&myhp, 0x4, &cRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
    VMMDLL_MemReadEx(handle, pid, (QWORD)缓冲区3 + 0xe68, (PBYTE)&myteam, 0x4, &cRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
    for (int i = 0; i < 50 ;i++)
    {
        
             //绘制骨骼线
              //准备读取
            //VMMDLL_Scatter_PrepareEx((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)缓冲区3 + 0xab4, 0x4, (PBYTE)&myhp, &bytesRead);
            //VMMDLL_Scatter_PrepareEx((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)缓冲区3 + 0xe68, 0x4, (PBYTE)&myteam, &bytesRead);
            
            //VMMDLL_Scatter_PrepareEx((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)addresses_strcture[i] + 0xab4, 0x4, (PBYTE)&hp, &bytesRead);
            //VMMDLL_Scatter_PrepareEx((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)addresses_strcture[i] + 0xe68, 0x4, (PBYTE)&team, &bytesRead);
            //VMMDLL_Scatter_PrepareEx((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)addresses_guge[i], 0x152c, (PBYTE)&buffer, &bytesRead);
            //VMMDLL_Scatter_ExecuteRead((VMMDLL_SCATTER_HANDLE)hScatter);
            //std::cout << hp <<"---"<<team << std::endl;
        VMMDLL_MemReadEx(handle, pid, (QWORD)addresses_strcture[i] + 0xab4, (PBYTE)&hp, 0x4, &cRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING|VMMDLL_FLAG_ZEROPAD_ON_FAIL);
        VMMDLL_MemReadEx(handle, pid, (QWORD)addresses_strcture[i] + 0xe68, (PBYTE)&team, 0x4, &cRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
        VMMDLL_MemReadEx(handle, pid, (QWORD)addresses_guge[i], (PBYTE)&buffer, 0x152c, &cRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
        //std::cout << hp<<"---"<<team << std::endl;
        if(team!=myteam&&hp>0&&hp<101&&team<4&&team>1)
        {
            骨骼绘制(5, 6);
            骨骼绘制(4, 5);
            骨骼绘制(2, 4);
            骨骼绘制(2, 25);
            骨骼绘制(2, 22);
            骨骼绘制(25, 26);
            骨骼绘制(22, 23);
            骨骼绘制(26, 27);
            骨骼绘制(23, 24);
            骨骼绘制(5, 86);
            骨骼绘制(14, 86);
            骨骼绘制(14, 67);
            骨骼绘制(5, 61);
            骨骼绘制(9, 61);
            骨骼绘制(9, 42);
        }
               
                
    }
}



int main()
{
   

    const char* tmp_str[] = { "", "-device", "FPGA" };
     handle = VMMDLL_Initialize(3, tmp_str);
    std::cout << "handle is " << handle << qProcessBase << std::endl;
    // 读pid
    VMMDLL_PidGetFromName(handle, (LPSTR)"cs2.exe", &pid);
    std::cout << "pid is "<< pid << std::endl;

    //// 读基址
    PVMMDLL_MAP_MODULEENTRY ppModuleMapEntry;
    VMMDLL_Map_GetModuleFromNameU(handle, pid, (LPSTR)"client.dll", &ppModuleMapEntry, NULL);
    qProcessBase = ppModuleMapEntry->vaBase;
    std::cout << "Process base is " << std::hex << qProcessBase << std::endl;
    //
   
    /*hScatter = VMMDLL_Scatter_Initialize(handle, pid, 0);
    if (!hScatter) 
    {
        printf("Failed to initialize Scatter handle.\n");
        
    }
    std::cout << "hscatter读取成功" << std::endl;*/
    //VMMDLL_Scatter_CloseHandle(hScatter);

    Width = GetSystemMetrics(SM_CXSCREEN);//获取当前分辨率宽度`
    Hight = GetSystemMetrics(SM_CYSCREEN);//获取当前分辨率高度
    //std::cout << Width << "---" << Hight << std::endl;
    地址载入();
    /*hScatter = VMMDLL_Scatter_Initialize((VMM_HANDLE)handle, (DWORD)pid, 0);
    VMMDLL_Scatter_Prepare((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)qProcessBase + 0x1a8a238, 0x40);
    VMMDLL_Scatter_Prepare((VMMDLL_SCATTER_HANDLE)hScatter, (QWORD)addresses_guge[3], 0x152c);*/
    // HWND GameHwnd = FindWindowA("MainWindow",NULL);
     // Create application window
     //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT, wc.lpszClassName, NULL, WS_POPUP, 0, 0, Width, Hight, nullptr, nullptr, wc.hInstance, nullptr);
    // | WS_EX_LAYERED  | WS_EX_TRANSPARENT WS_EX_TOPMOST |WS_EX_LAYERED ,
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);




    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(ImColor(255, 255, 255)); //背景颜色
    SetLayeredWindowAttributes(hwnd, ImColor(255, 255, 255), 0, LWA_COLORKEY);

    // Main loop
    bool done = false;



    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame(); //开始帧

        //获取当前帧率
        float frameRate = ImGui::GetIO().Framerate;
        char frameRateStr[32];
        std::snprintf(frameRateStr, sizeof(frameRateStr), "FPS: %d", static_cast<int>(frameRate));
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 100), ImColor(0, 255, 0, 255), frameRateStr);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 120), ImColor(0, 255, 0, 255), u8"绘制开启");

        // 绘制代码区
      
        static bool windows_open = true;
        static bool test;
        static int a = 0;
        static float b = 0;
        static float c = 0;



        if (windows_open)
        {
           
            骨骼();
        }

        

        // Rendering
        ImGui::Render();

        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);


}