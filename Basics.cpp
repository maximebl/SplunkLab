#include "Headers/Basics.h"

std::wstring GetSolutionDirectory()
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(SOLUTION_DIR);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT NonQueuedMessage, WPARAM Wparam, LPARAM lparam)
{
    LONG_PTR* ProcData = (LONG_PTR*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (ProcData == NULL)
    {
        return DefWindowProc(hwnd, NonQueuedMessage, Wparam, lparam);
    }

    switch (NonQueuedMessage)
    {
    case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }

    case WM_SIZE:
        {
            // Implement resizing later
            break;
        }

    default:
        {
            return DefWindowProc(hwnd, NonQueuedMessage, Wparam, lparam);
        }
    }

    return 0;
}

void CreateWin32Window(WindowInfo* WndInfo, HINSTANCE hInstance, int nCmdShow)
{
    // Fill in window class struct
    WNDCLASSEX WndClass;
    WndClass.cbSize = sizeof(WNDCLASSEX);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = WindowProc;
    WndClass.cbClsExtra = 1000;
    WndClass.cbWndExtra = 1000;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = NULL;
    WndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = WndInfo->WindowName;
    WndClass.hIconSm = NULL;

    // Register window class struct
    RegisterClassEx(&WndClass);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Create the window
    WndInfo->Hwnd = CreateWindowEx(0, WndClass.lpszClassName,
                                   WndInfo->WindowName, WS_OVERLAPPEDWINDOW, WndInfo->X,
                                   WndInfo->Y, WndInfo->Width, WndInfo->Height, NULL, NULL,
                                   hInstance, NULL);

    assert(WndInfo->Hwnd);

    // Show window
    ShowWindow(WndInfo->Hwnd, nCmdShow);
}

void DestroyWindow(struct WindowInfo* WndInfo, HINSTANCE hInstance)
{
    DestroyWindow(WndInfo->Hwnd);
    UnregisterClass(WndInfo->WindowName, hInstance);
}

MSG WindowMessageLoop()
{
    MSG QueuedMessage;
    // Run message loop
    if (PeekMessage(&QueuedMessage, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&QueuedMessage);
        DispatchMessage(&QueuedMessage);
    }

    // return QueuedMessage.message;
    return QueuedMessage;
}
