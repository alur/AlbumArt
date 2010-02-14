#include "albumart.h"
#include "crc32.h"
#include "bangs.h"
#include "LiteStep.h"
#include "main.h"
#include "strsafe.h"
#include "constants.h"
#include "curl\curl.h"

// Constants
const char g_rcsRevision[]	= "1.7";
const char g_szAppName[]	= "AlbumArt";
const char g_szMsgHandler[]	= "LSAlbumArtManager";
const UINT g_lsMessages[]	= {LM_GETREVID, LM_REFRESH, 0};

// Global Variables
HWND g_hParent, g_hwndMessageHandler;
HINSTANCE g_hInstance;
char g_szPath[MAX_PATH];

// External variables
extern GroupMap g_Groups;

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID /* pvReserved */)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInst);

	return TRUE;
}

int initModuleEx(HWND hwndParent, HINSTANCE hDllInstance, LPCSTR szPath)
{
	g_hParent = hwndParent;
	g_hInstance = hDllInstance;
	StringCchCopy(g_szPath, sizeof(g_szPath), szPath);

	if (!CreateMessageHandler())
		return 1;

	CRC32::crcInit();
	curl_global_init(CURL_GLOBAL_WIN32);

	Bangs::_Register();

	// Get RC Settings
	AlbumArt::LoadSettings();

	// Initialize Variables
	AlbumArt::UpdateInformation(true);

	return 0;
}

void quitModule(HINSTANCE hDllInstance)
{
	KillTimer(g_hwndMessageHandler, UPDATETIMER_ID);

	// Kill any lingering threads
	for (GroupMap::iterator iter = g_Groups.begin(); iter != g_Groups.end(); iter++)
	{
		if (iter->second.bThreadIsRunning)
		{
			*(iter->second.threadbTerminate) = true;
		}
	}

	g_Groups.clear();

	Bangs::_Unregister();

	if (g_hwndMessageHandler)
	{
		SendMessage(GetLitestepWnd(), LM_UNREGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);
		DestroyWindow(g_hwndMessageHandler);
	}

	g_hwndMessageHandler = NULL;

	if (!UnregisterClass(g_szMsgHandler, hDllInstance))
		MessageBox(NULL, "UnregisterClass Failed!", "AlbumArt Error", MB_OK);
}

LRESULT WINAPI MessageHandlerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case LM_REFRESH:
		{
			AlbumArt::LoadSettings(true);
			return 0;
		}
	case LM_GETREVID:
		{
			StringCchPrintf((char*)lParam, 64, "%s: %s", g_szAppName, g_rcsRevision);
			size_t uLength;
			if (SUCCEEDED(StringCchLength((char*)lParam, 64, &uLength)))
				return uLength;
			lParam = NULL;
			return 0;
		}
	case WM_TIMER:
		{
			switch (wParam)
			{
			case UPDATETIMER_ID:
				{
					AlbumArt::UpdateInformation();
				}
			}
			return 0;
		}
	case WM_DOWNLOADFINISHED:
		{
			GroupData *gData = (GroupData*)wParam;
			if (gData != NULL)
			{
				LiteStep::ExecuteEvent(gData->szPrefix, "OnDownloadFinished");
				AlbumArt::SetPath(gData, (LPCSTR)lParam);
			}
			return 0;
		}
	case WM_DOWNLOADSTARTED:
		{
			GroupData *gData = (GroupData*)wParam;
			if (gData != NULL)
			{
				LiteStep::ExecuteEvent(gData->szPrefix, "OnDownloadStarted");
			}
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool CreateMessageHandler()
{
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = MessageHandlerProc;
	wc.hInstance = g_hInstance;
	wc.lpszClassName = g_szMsgHandler;
	wc.hIconSm = 0;
	wc.style = CS_NOCLOSE;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "RegisterClassEx Failed!", "AlbumArt Error", MB_OK);
		return false;
	}

	g_hwndMessageHandler = CreateWindowEx(WS_EX_TOOLWINDOW, g_szMsgHandler, 0, WS_POPUP, 0, 0, 0, 0, g_hParent, 0, g_hInstance, 0);

	if (!g_hwndMessageHandler)
	{
		MessageBox(NULL, "CreateWindowEx Failed!", "AlbumArt Error", MB_OK);
		return false;
	}
	
	SendMessage(GetLitestepWnd(), LM_REGISTERMESSAGE, (WPARAM)g_hwndMessageHandler, (LPARAM)g_lsMessages);

	return true;
}