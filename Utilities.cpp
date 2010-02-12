#include "utilities.h"

// Checks if a file exists
bool Utilities::File_Exists (LPCSTR pszFilePath)
{
	return GetFileAttributes(pszFilePath) != INVALID_FILE_ATTRIBUTES;
}

void Utilities::URLEncode(LPCSTR pszString, LPSTR pszEncoded, UINT cchLength)
{
	for (UINT i = 0, iPos = 0; iPos < cchLength; i++)
	{
		if (pszString[i] == 0)
		{
			pszEncoded[iPos] = 0;
			break;
		}
		else if ((48 <= pszString[i] && pszString[i] <= 57) || (65 <= pszString[i] && pszString[i] <= 90) ||
			(97 <= pszString[i] && pszString[i] <= 122) ||
			pszString[i] == '~' || pszString[i] == '!' || pszString[i] == '*' || pszString[i] == '(' ||
			pszString[i] == ')' || pszString[i] == '\'')
		{
			pszEncoded[iPos++] = pszString[i];
		}
		else
		{
			// Work out ASCII values for the 2 hex digits
			char digit1 = (pszString[i] & 0xF0) >> 4, digit2 = (pszString[i] & 0x0F);
			digit1 += (digit1 < 10) ? 48 : 87;
			digit2 += (digit2 < 10) ? 48 : 87;

			pszEncoded[iPos++] = '%';
			pszEncoded[iPos++] = digit1;
			pszEncoded[iPos++] = digit2;
		}
	}
}

bool Utilities::GetExtendedWinampFileInfo(LPCSTR pszFile, LPCSTR pszField, LPSTR pszOut, UINT cchLength)
{
	HWND hwndWA2 = FindWindow("Winamp v1.x", NULL);
	if (hwndWA2 == NULL)
		return false;

	ULONG dWinamp;
	GetWindowThreadProcessId(hwndWA2, &dWinamp);

	HANDLE hWinamp;
	hWinamp = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, dWinamp);

	extendedFileInfoStruct exFIS = {0};

	// Allocate some memory in winamps address space
	char *remBuf = (char *)VirtualAllocEx(hWinamp, NULL, cchLength, MEM_COMMIT, PAGE_READWRITE);
	char *remMetaDataBuf = (char *)VirtualAllocEx(hWinamp, NULL, 64, MEM_COMMIT, PAGE_READWRITE);
	char *remStruct = (char *)VirtualAllocEx(hWinamp, NULL, sizeof(exFIS), MEM_COMMIT, PAGE_READWRITE);
	char *remFileBuf = (char *)VirtualAllocEx(hWinamp, NULL, 512, MEM_COMMIT, PAGE_READWRITE);
	
	// Fill in the data for the extendedfileinfostruct
	exFIS.filename = remFileBuf;
	exFIS.metadata = remMetaDataBuf;
	exFIS.ret = remBuf;
	exFIS.retlen = cchLength;

	// Copy over the extendedfileinfostruct and it's data to winamps address space
	WriteProcessMemory(hWinamp, remFileBuf, pszFile, 512, NULL);
	WriteProcessMemory(hWinamp, remMetaDataBuf, pszField, 64, NULL);
	WriteProcessMemory(hWinamp, remStruct, &exFIS, sizeof(exFIS), NULL);

	// Have winamp read the extendedfileinfostruct and put the desired metadata in remBuf
	if (!SendMessage(hwndWA2, WM_USER, (WPARAM)remStruct, 296))
		return false;

	// Copy remBuf to local memory
	ReadProcessMemory(hWinamp, remBuf, pszOut, cchLength, NULL);

	// Free up memory allocated in winamps address space
	VirtualFreeEx(hWinamp, remBuf, cchLength, MEM_DECOMMIT);
	VirtualFreeEx(hWinamp, remBuf, 0, MEM_RELEASE);
	VirtualFreeEx(hWinamp, remMetaDataBuf, 64, MEM_DECOMMIT);
	VirtualFreeEx(hWinamp, remMetaDataBuf, 0, MEM_RELEASE);
	VirtualFreeEx(hWinamp, remStruct, sizeof(exFIS), MEM_DECOMMIT);
	VirtualFreeEx(hWinamp, remStruct, 0, MEM_RELEASE);
	VirtualFreeEx(hWinamp, remFileBuf, 512, MEM_DECOMMIT);
	VirtualFreeEx(hWinamp, remFileBuf, 0, MEM_RELEASE);

	return true;
}

UCHAR Utilities::StringToParseType(LPCSTR szParse)
{
	if (_stricmp(szParse, "Other") == 0)
		return ID3PT_OTHER;
	else if (_stricmp(szParse, "PNG32Icon") == 0)
		return ID3PT_PNG32ICON;
	else if (_stricmp(szParse, "OtherIcon") == 0)
		return ID3PT_OTHERICON;
	else if (_stricmp(szParse, "CoverFront") == 0)
		return ID3PT_COVERFRONT;
	else if (_stricmp(szParse, "CoverBack") == 0)
		return ID3PT_COVERBACK;
	else if (_stricmp(szParse, "LeafletPage") == 0)
		return ID3PT_LEAFLETPAGE;
	else if (_stricmp(szParse, "Media") == 0)
		return ID3PT_MEDIA;
	else if (_stricmp(szParse, "LeadArtist") == 0)
		return ID3PT_LEADARTIST;
	else if (_stricmp(szParse, "Artist") == 0)
		return ID3PT_ARTIST;
	else if (_stricmp(szParse, "Conductor") == 0)
		return ID3PT_CONDUCTOR;
	else if (_stricmp(szParse, "Band") == 0)
		return ID3PT_BAND;
	else if (_stricmp(szParse, "Composer") == 0)
		return ID3PT_COMPOSER;
	else if (_stricmp(szParse, "Lyricist") == 0)
		return ID3PT_LYRICIST;
	else if (_stricmp(szParse, "RecordingLocation") == 0)
		return ID3PT_REC_LOCATION;
	else if (_stricmp(szParse, "Recording") == 0)
		return ID3PT_RECORDING;
	else if (_stricmp(szParse, "Performance") == 0)
		return ID3PT_PERFORMANCE;
	else if (_stricmp(szParse, "Video") == 0)
		return ID3PT_VIDEO;
	else if (_stricmp(szParse, "Fish") == 0)
		return ID3PT_FISH;
	else if (_stricmp(szParse, "Illustration") == 0)
		return ID3PT_ILLUSTRATION;
	else if (_stricmp(szParse, "ArtistLogo") == 0)
		return ID3PT_ARTISTLOGO;
	else if (_stricmp(szParse, "PublisherLogo") == 0)
		return ID3PT_PUBLISHERLOGO;

	return ID3PT_COVERFRONT;
}

bool Utilities::String2Bool(LPCSTR pszBool)
{
	return (lstrcmpi(pszBool, "off") && lstrcmpi(pszBool, "false") && lstrcmpi(pszBool, "no") && lstrcmpi(pszBool, "0"));
}