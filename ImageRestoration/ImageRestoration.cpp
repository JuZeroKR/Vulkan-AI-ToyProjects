#include "framework.h"
#include "ImageRestoration.h"
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include "VulkanEngine.h"
#include "VulkanEngine/Context.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // 0. Open debug console window
    AllocConsole();
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    std::cout << "[ImageRestoration] Console initialized." << std::endl;

    // 1. Initialize Windows Window
    const wchar_t CLASS_NAME[] = L"ImageRestorationWindowClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"ImageRestoration Vulkan Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd)
    {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);

    // 2. Create Vulkan Context and call createInstance
    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface"
    };

    std::unique_ptr<caaVk::Context> context;
    try {
        context = std::make_unique<caaVk::Context>(instanceExtensions, /*useSwapchain=*/true);
        std::cout << "[ImageRestoration] Vulkan instance created successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[ImageRestoration] Error: " << e.what() << std::endl;
        OutputDebugStringA(e.what());
        return -1;
    }

    // 3. Main Message Loop
    MSG msg = { };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // TODO: Vulkan Render Code Here
        }
    }

    // 4. Cleanup
    context.reset();
    std::cout << "[ImageRestoration] Shutdown complete." << std::endl;

    return (int) msg.wParam;
}
