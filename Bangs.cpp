#include "bangs.h"
#include "albumart.h"
#include "utilities.h"
#include "crc32.h"
#include "LiteStep.h"

extern char g_szDebugMessage[];
extern HWND g_hwndMessageHandler;
extern GroupMap g_Groups;

namespace Bangs
{
	BangItem BangMap[] =
	{
		{"Debug", Debug},
		{"SetUpdateFrequency", SetUpdateFrequency},
		{"Create", Create},
		{"Destroy", Destroy},
		{"SetNoCRC", SetNoCRC},
		{"SetNoArtPath", SetNoArtPath},
		{"SetParsePath", SetParsePath},
		{"SetTrackOffset", SetTrackOffset},
		{"SetOutOfBoundsPath", SetOutOfBoundsPath},
		{"SetSearchFolderBeforeTag", SetSearchFolderBeforeTag},
		{"SetAutoDownloadCoversTo", SetAutoDownloadCoversTo},
		{"AddParseType", AddParseType},
		{"DeleteParseType", DeleteParseType},
		{"AddCoverName", AddCoverName},
		{"DeleteCoverName", DeleteCoverName},
		{NULL, NULL}
	};
}

// Registers bangs
void Bangs::_Register()
{
	char szBangName[MAX_BANGCOMMAND];
	for (int i = 0; BangMap[i].pCommand != NULL; i++)
	{
		StringCchPrintf(szBangName, MAX_BANGCOMMAND, "!AlbumArt%s", BangMap[i].szName);
		AddBangCommand(szBangName, BangMap[i].pCommand);
	}
}

// Unregisters bangs
void Bangs::_Unregister()
{
	char szBangName[MAX_BANGCOMMAND];
	for (int i = 0; BangMap[i].pCommand != NULL; i++)
	{
		StringCchPrintf(szBangName, MAX_BANGCOMMAND, "!AlbumArt%s", BangMap[i].szName);
		RemoveBangCommand(szBangName);
	}
}

// Shows the debug message
void Bangs::Debug(HWND, LPCSTR)
{
	MessageBox(NULL, g_szDebugMessage, "AlbumArt Status", MB_OK);
}

// Modifies the update frequency
void Bangs::SetUpdateFrequency(HWND, LPCSTR pszArgs)
{
	int iFrequency = atoi(pszArgs);
	if (iFrequency > USER_TIMER_MAXIMUM)
		iFrequency = USER_TIMER_MAXIMUM;
	else if (iFrequency < USER_TIMER_MINIMUM)
		iFrequency = USER_TIMER_MINIMUM;
	KillTimer(g_hwndMessageHandler, UPDATETIMER_ID);
	SetTimer(g_hwndMessageHandler, UPDATETIMER_ID, iFrequency, NULL);
}

// Creates Group(s) in pszArgs
void Bangs::Create(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;
	
	while (GetToken(pszNext, szToken, &pszNext, false))
	{
		if (strcmp(szToken, "") != 0)
		{
			AlbumArt::CreateGroup(szToken);
		}
	}
}

// Destroys Group(s) in pszArgs
void Bangs::Destroy(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	while (GetToken(pszNext, szToken, &pszNext, false))
	{
		if (strcmp(szToken, "") != 0)
		{
			AlbumArt::DestroyGroup(szToken);
		}
	}
}

// Updates the NoCRC setting
void Bangs::SetNoCRC(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the "BOOL" (second token)
		GetToken(pszNext, szToken, &pszNext, false);
		
		if (_stricmp(szToken, "toggle") != 0)
		{
			gData->bNoCRC32 = Utilities::String2Bool(szToken);
		}
		else // Toggling
		{
			gData->bNoCRC32 = !gData->bNoCRC32;
		}

		// Update the CRC32 value if necesary
		if (gData->bNoCRC32)
		{
			char szPath[MAX_LINE_LENGTH];
			LiteStep::GetPrefixedRCLine(szPath, sizeof(szPath), gData->szPrefix, "Path", "");
			gData->crc = CRC32::crc32(szPath);
			LiteStep::SetEvar(gData->szPrefix, "CRC", "%X", gData->crc);
		}
		else
		{
			LiteStep::SetEvar(gData->szPrefix, "CRC", "");
		}
	}
}

// Update the NoArtPath
void Bangs::SetNoArtPath(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new path (second token)
		GetToken(pszNext, szToken, &pszNext, false);
		
		char szCoverPath[MAX_LINE_LENGTH], szCurrentNoArtPath[MAX_PATH];
		LiteStep::GetPrefixedRCLine(szCoverPath, sizeof(szCoverPath), gData->szPrefix, "Path", "");
		StringCchCopy(szCurrentNoArtPath, sizeof(szCurrentNoArtPath), gData->szNoArtPath);
		
		// Update the NoArtPath
		if (strcmp(szToken, "") != 0)
		{
			if (szToken[1] != ':') // Relative path...
			{
				char szImageFolder[MAX_PATH];
				LiteStep::GetPrefixedRCLine(szImageFolder, sizeof(szImageFolder), "LS", "ImageFolder", "");

				StringCchCopy(gData->szNoArtPath, sizeof(gData->szNoArtPath), szImageFolder);
				StringCchCat(gData->szNoArtPath, sizeof(gData->szNoArtPath), szToken);
			}
			else
			{
				StringCchCopy(gData->szNoArtPath, sizeof(gData->szNoArtPath), szToken);
			}
		}
		else
		{
			StringCchCopy(gData->szNoArtPath, sizeof(gData->szNoArtPath), szToken);
		}

		// Check if the current cover is using the NoArtPath, and if so, update the current path
		if (_stricmp(szCoverPath, szCurrentNoArtPath) == 0)
		{
			AlbumArt::SetPath(gData, gData->szNoArtPath);
		}
	}
}

// Update the ParsePath
void Bangs::SetParsePath(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new path (second token)
		GetToken(pszNext, szToken, &pszNext, false);
		
		// Update the ParsePath
		if (strcmp(szToken, "") != 0)
		{
			if (szToken[1] != ':') // Relative path...
			{
				char szImageFolder[MAX_PATH];
				LiteStep::GetPrefixedRCLine(szImageFolder, sizeof(szImageFolder), "LS", "ImageFolder", "");

				StringCchCopy(gData->szParsedPath, sizeof(gData->szParsedPath), szImageFolder);
				StringCchCat(gData->szParsedPath, sizeof(gData->szParsedPath), szToken);
			}
			else
			{
				StringCchCopy(gData->szParsedPath, sizeof(gData->szParsedPath), szToken);
			}
		}
		else
		{
			StringCchCopy(gData->szParsedPath, sizeof(gData->szParsedPath), szToken);
		}
	}
}

// Update the trackoffset
void Bangs::SetTrackOffset(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new offset (second token)
		GetToken(pszNext, szToken, &pszNext, false);

		// Update the GroupData
		gData->iTrackOffset = atoi(szToken);

		// Let UpdateInformation handle everything else
		AlbumArt::UpdateInformation(true);
	}
}

// Update the OutOfBounds path
void Bangs::SetOutOfBoundsPath(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new path (second token)
		GetToken(pszNext, szToken, &pszNext, false);
		
		// Update the OutOfBounds path
		if (strcmp(szToken, "") != 0)
		{
			if (szToken[1] != ':') // Relative path...
			{
				char szImageFolder[MAX_PATH];
				LiteStep::GetPrefixedRCLine(szImageFolder, sizeof(szImageFolder), "LS", "ImageFolder", "");

				StringCchCopy(gData->szOutOfBoundsPath, sizeof(gData->szOutOfBoundsPath), szImageFolder);
				StringCchCat(gData->szOutOfBoundsPath, sizeof(gData->szOutOfBoundsPath), szToken);
			}
			else
			{
				StringCchCopy(gData->szOutOfBoundsPath, sizeof(gData->szOutOfBoundsPath), szToken);
			}
		}
		else
		{
			StringCchCopy(gData->szOutOfBoundsPath, sizeof(gData->szOutOfBoundsPath), szToken);
		}

		// Check if we are currently using the OutOfBounds path
		if (gData->bIsOutOfBounds)
		{
			AlbumArt::SetPath(gData, gData->szOutOfBoundsPath);
		}
	}
}

// Set the SearchFolderBeforeTag setting
void Bangs::SetSearchFolderBeforeTag(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the "BOOL" (second token)
		GetToken(pszNext, szToken, &pszNext, false);
		
		if (_stricmp(szToken, "toggle") != 0)
		{
			gData->bSearchFolderBeforeTag = Utilities::String2Bool(szToken);
		}
		else // Toggling
		{
			gData->bSearchFolderBeforeTag = !gData->bSearchFolderBeforeTag;
		}
	}
}

// Upadtes the AutoDownloadCoversTo path
void Bangs::SetAutoDownloadCoversTo(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new path (second token)
		GetToken(pszNext, szToken, &pszNext, false);
		
		char szCoverPath[MAX_LINE_LENGTH];
		LiteStep::GetPrefixedRCLine(szCoverPath, sizeof(szCoverPath), gData->szPrefix, "Path", "");
		
		// Update the AutoDownloadCoversTo path
		StringCchCopy(gData->szDownloadPath, sizeof(gData->szDownloadPath), szToken);

		// Check if the current cover is using the NoArtPath, and if so, try to download a new cover
		if (_stricmp(szCoverPath, gData->szNoArtPath) == 0)
		{
			if (_stricmp(szToken, "") != 0)
			{
				if (!gData->bIsOutOfBounds)
				{
					HWND hwndWA2 = FindWindow("Winamp v1.x", NULL);
					if (hwndWA2 != NULL) // Winamp isn't running
					{
						int Track = SendMessage(hwndWA2, WM_USER, 0, 125);
						HANDLE hWinamp;
						ULONG dWinamp;

						GetWindowThreadProcessId(hwndWA2, &dWinamp);
						hWinamp = OpenProcess(PROCESS_VM_READ, false, dWinamp);
						if (hWinamp != NULL)
						{
							char szTrackPath[MAX_PATH];
							void* winampTrack = (void*)SendMessage(hwndWA2, WM_USER, Track + gData->iTrackOffset, 211);
							ReadProcessMemory(hWinamp, winampTrack, &szTrackPath, sizeof(szTrackPath), NULL);
							if (szTrackPath[4] == ':')
								AlbumArt::DownloadCover(gData, szTrackPath + 7);
							else
								AlbumArt::DownloadCover(gData, szTrackPath);
							CloseHandle(hWinamp);
						}
					}
				}
			}
		}
	}
}

// Add some parsetypes
void Bangs::AddParseType(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new parsetypes (remaining tokens)
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (strcmp(szToken, "") != 0)
			{
				gData->ParseTypes.push_back(Utilities::StringToParseType(szToken));
			}
		}
	}
}

// Delete some parsetypes
void Bangs::DeleteParseType(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the parsetypes to delete (remaining tokens)
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (_stricmp(szToken, ".all") == 0)
			{
				gData->ParseTypes.clear();
				break;
			}
			else if (strcmp(szToken, "") != 0)
			{
				UCHAR uType = Utilities::StringToParseType(szToken);
				for (std::vector<UCHAR>::const_iterator iter = gData->ParseTypes.begin(); iter != gData->ParseTypes.end(); iter++)
				{
					if (*iter == uType)
					{
						gData->ParseTypes.erase(iter);
					}
				}
			}
		}
	}
}

// Add some covernames
void Bangs::AddCoverName(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the new covernames (remaining tokens)
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (strcmp(szToken, "") != 0)
			{
				gData->CoverNames.push_back(std::string(szToken));
			}
		}
	}
}

// Delete some covernames
void Bangs::DeleteCoverName(HWND, LPCSTR pszArgs)
{
	char szToken[MAX_LINE_LENGTH];
	LPCSTR pszNext = pszArgs;

	// Get the group name (first token)
	GetToken(pszNext, szToken, &pszNext, false);
	GroupData* gData = AlbumArt::GetGroupByPrefix(szToken);

	if (gData != NULL)
	{
		// Get the covernames to delete (remaining tokens)
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (_stricmp(szToken, ".all") == 0)
			{
				gData->CoverNames.clear();
				break;
			}
			else if (strcmp(szToken, "") != 0)
			{
				for (std::vector<std::string>::const_iterator iter = gData->CoverNames.begin(); iter != gData->CoverNames.end(); iter++)
				{
					if (strcmp(iter->c_str(), szToken) == 0)
					{
						gData->CoverNames.erase(iter);
					}
				}
			}
		}
	}
}

