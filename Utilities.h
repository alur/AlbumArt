#ifndef UTILITIES_H
#define UTILITIES_H

#include <windows.h>

namespace Utilities
{
	typedef struct extendedFileInfoStruct {
		LPCSTR filename;
		LPCSTR metadata;
		LPSTR ret;
		size_t retlen;
	} extendedFileInfoStruct;

	bool GetExtendedWinampFileInfo(LPCSTR pszFile, LPCSTR pszField, LPSTR pszOut, UINT cchLength);
	void URLEncode(LPCSTR pszString, LPSTR pszEncoded, UINT cchLength);
	bool File_Exists (LPCSTR pszFilePath);
	UCHAR StringToParseType(LPCSTR szParse);
	bool String2Bool(LPCSTR pszBool);
}

#endif