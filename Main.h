#ifndef MAIN_H
#define MAIN_H

extern "C" __declspec( dllexport ) int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath);
extern "C" __declspec( dllexport ) void quitModule(HINSTANCE hDllInstance);

bool CreateMessageHandler();
LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif