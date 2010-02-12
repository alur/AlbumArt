#ifndef LITESTEP_H
#define LITESTEP_H

#include <windows.h>
#include "lsapi.h"
#include <strsafe.h>
#include <shlwapi.h>

namespace LiteStep
{
	void GetPrefixedRCLine(LPSTR szDest, UINT cbBuffer, LPCSTR pszPrefix, LPCSTR pszOption, LPCSTR pszDefault);
	bool GetPrefixedRCBool(LPCSTR pszPrefix, LPCSTR pszOption, bool bDefault);
	void SetEvar(LPCSTR pszPrefix, LPCSTR pszEvar, LPCSTR pszFormat, ...);
	void ExecuteEvent(LPCSTR pszPrefix, LPCSTR pszEvent);
	int GetPrefixedRCInt(LPCSTR pszPrefix, LPCSTR pszOption, int iDefault);
}

#endif