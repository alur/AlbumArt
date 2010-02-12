#ifndef BANGS_H
#define BANGS_H

#include <windows.h>
#include "lsapi.h"
#include "constants.h"
#include <stdlib.h>

typedef struct BangItem {
	char szName[54];
	BANGCOMMANDPROC pCommand;
} BangItem;

namespace Bangs
{
	void _Register();
	void _Unregister();

	void Debug(HWND hCaller, LPCSTR pszArgs);
	void SetUpdateFrequency(HWND hCaller, LPCSTR pszArgs);
	void Create(HWND hCaller, LPCSTR pszArgs);
	void Destroy(HWND hCaller, LPCSTR pszArgs);

	void SetNoCRC(HWND hCaller, LPCSTR pszArgs);
	void SetNoArtPath(HWND hCaller, LPCSTR pszArgs);
	void SetParsePath(HWND hCaller, LPCSTR pszArgs);
	void SetTrackOffset(HWND hCaller, LPCSTR pszArgs);
	void SetOutOfBoundsPath(HWND hCaller, LPCSTR pszArgs);
	void SetSearchFolderBeforeTag(HWND hCaller, LPCSTR pszArgs);
	void SetAutoDownloadCoversTo(HWND hCaller, LPCSTR pszArgs);

	void AddParseType(HWND hCaller, LPCSTR pszArgs);
	void DeleteParseType(HWND hCaller, LPCSTR pszArgs);
	void AddCoverName(HWND hCaller, LPCSTR pszArgs);
	void DeleteCoverName(HWND hCaller, LPCSTR pszArgs);
}
#endif