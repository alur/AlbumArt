#ifndef MAIN_H
#define MAIN_H

#include "albumart.h"
#include "crc32.h"
#include "bangs.h"
#include "LiteStep.h"

extern "C" __declspec( dllexport ) int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath);
extern "C" __declspec( dllexport ) void quitModule(HINSTANCE hDllInstance);

bool CreateMessageHandler();
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif