#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <locale>
#include <codecvt>

struct WindowInfo
{
    const char* WindowName;
    INT X;
    INT Y;
    INT Width;
    INT Height;
    HWND Hwnd;
};

std::wstring GetSolutionDirectory();
void CreateWin32Window(WindowInfo* WndInfo, HINSTANCE hInstance, int nCmdShow);
void DestroyWindow(WindowInfo* WndInfo, HINSTANCE hInstance);
MSG WindowMessageLoop();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT NonQueuedMessage, WPARAM wparam, LPARAM lparam);

inline UINT32 AlignTo(UINT32 num, UINT32 alignment)
{
    assert(alignment > 0);
    return ((num + alignment - 1) / alignment) * alignment;
}

inline UINT64 AlignTo(UINT64 num, UINT64 alignment)
{
    assert(alignment > 0);
    return ((num + alignment - 1) / alignment) * alignment;
}

