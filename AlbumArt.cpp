#include "albumart.h"
#include "crc32.h"
#include "utilities.h"
#include "LiteStep.h"

// Global variables
GroupMap g_Groups;
char g_szDebugMessage[MAX_LINE_LENGTH];
char g_szOnTrackChange[MAX_LINE_LENGTH];
int g_iCurrentTrack;
extern HWND g_hParent, g_hwndMessageHandler;

/*/
TODO::

 - Add !AlbumArtDownloadCover [group] [file to save cover to, default=(group)AutoDownloadCoversTo]
 - Add (group)DownloadDimensions [min-width] [min-height] [max-width] [max-height]
 - Add (group)CoverWidth, (group)CoverHeight [EVARS]
 - Add bangs to configure all settings w/o recycling or refreshing

/*/

// Loads all settings from config
void AlbumArt::LoadSettings (bool bIsRefresh)
{
	// Clear all old group settings on a refresh
	if (bIsRefresh)
	{
		g_Groups.clear();
		KillTimer(g_hwndMessageHandler, UPDATETIMER_ID);
	}

	// Timer update frequency
	UINT uFrequency = GetRCInt("AlbumArtUpdateFrequency", 100);
	if (uFrequency > USER_TIMER_MAXIMUM)
		uFrequency = USER_TIMER_MAXIMUM;
	else if (uFrequency < USER_TIMER_MINIMUM)
		uFrequency = USER_TIMER_MINIMUM;

	// Create all groups
	char szLine[MAX_LINE_LENGTH], szToken[MAX_LINE_LENGTH];
	LPVOID f = LCOpen(NULL);
	while (LCReadNextConfig(f, "*AlbumArt", szLine, sizeof(szLine)))
	{
		LPCSTR pszNext = szLine;
		GetToken(pszNext, szToken, &pszNext, false);
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (strcmp(szToken, "") != 0)
			{
				CreateGroup(szToken);
			}
		}
	}
	LCClose(f);

	// Add a group named "AlbumArt" if no other groups are defined (backwards compability and simplicity)
	if (g_Groups.empty())
	{
		CreateGroup("AlbumArt");
	}

	// Make sure that the current track is checked
	g_iCurrentTrack = -1;

	// Start the timer
	SetTimer(g_hwndMessageHandler, UPDATETIMER_ID, uFrequency, NULL);
}

// Creates a group named szPrefix and initalizes all of it's settings
void AlbumArt::CreateGroup(LPCSTR pszPrefix)
{
	GroupData gData = {0};
	char szConfigName[MAX_PATH], szLine[MAX_LINE_LENGTH], szToken[MAX_LINE_LENGTH];
	LPVOID f;

	f = LCOpen(NULL);
	StringCchPrintf(szConfigName, sizeof(szConfigName), "*%sCoverName", pszPrefix);
	while (LCReadNextConfig(f, szConfigName, szLine, sizeof(szLine)))
	{
		LPCSTR pszNext = szLine;
		GetToken(pszNext, szToken, &pszNext, false);
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (strcmp(szToken, "") != 0)
			{
				gData.CoverNames.push_back(std::string(szToken));
			}
		}
	}
	LCClose(f);

	f = LCOpen(NULL);
	StringCchPrintf(szConfigName, sizeof(szConfigName), "*%sParseType", pszPrefix);
	while (LCReadNextConfig(f, szConfigName, szLine, sizeof(szLine)))
	{
		LPCSTR pszNext = szLine;
		GetToken(pszNext, szToken, &pszNext, false);
		while (GetToken(pszNext, szToken, &pszNext, false))
		{
			if (strcmp(szToken, "") != 0)
			{
				gData.ParseTypes.push_back(Utilities::StringToParseType(szToken));
			}
		}
	}
	LCClose(f);

	StringCchCopy(gData.szPrefix, sizeof(gData.szPrefix), pszPrefix);
	LiteStep::GetPrefixedRCLine(gData.szParsedPath, sizeof(gData.szParsedPath), pszPrefix, "ParsePath", "");
	LiteStep::GetPrefixedRCLine(gData.szNoArtPath, sizeof(gData.szNoArtPath), pszPrefix, "NoArtPath", "");
	LiteStep::GetPrefixedRCLine(gData.szOutOfBoundsPath, sizeof(gData.szOutOfBoundsPath), pszPrefix, "OutOfBoundsPath", "");
	LiteStep::GetPrefixedRCLine(gData.szDownloadPath, sizeof(gData.szDownloadPath), pszPrefix, "AutoDownloadCoversTo", "");
	gData.bNoCRC32 = LiteStep::GetPrefixedRCBool(pszPrefix, "NoCRC", false);
	gData.bSearchFolderBeforeTag = LiteStep::GetPrefixedRCBool(pszPrefix, "SearchFolderBeforeTag", false);
	gData.iTrackOffset = LiteStep::GetPrefixedRCInt(pszPrefix, "TrackOffset", 0);
	gData.bNoCRC32ForErrors = false;
	gData.bIsOutOfBounds = false;
	gData.bThreadIsRunning = false;
	gData.threadbTerminate = NULL;
	gData.crc = 0;

	// Handle relative paths
	char szImageFolder[MAX_PATH], szTemp[MAX_PATH];
	LiteStep::GetPrefixedRCLine(szImageFolder, sizeof(szImageFolder), "LS", "ImageFolder", "");
	if (strcmp(gData.szParsedPath, "") != 0)
	{
		if (gData.szParsedPath[1] != ':') // Relative path...
		{
			StringCchCopy(szTemp, sizeof(szTemp), szImageFolder);
			StringCchCat(szTemp, sizeof(szTemp), gData.szParsedPath);
			StringCchCopy(gData.szParsedPath, sizeof(gData.szParsedPath), szTemp);
		}
	}
	if (strcmp(gData.szNoArtPath, "") != 0)
	{
		if (gData.szNoArtPath[1] != ':') // Relative path...
		{
			StringCchCopy(szTemp, sizeof(szTemp), szImageFolder);
			StringCchCat(szTemp, sizeof(szTemp), gData.szNoArtPath);
			StringCchCopy(gData.szNoArtPath, sizeof(gData.szNoArtPath), szTemp);
		}
	}
	if (strcmp(gData.szOutOfBoundsPath, "") != 0)
	{
		if (gData.szOutOfBoundsPath[1] != ':') // Relative path...
		{
			StringCchCopy(szTemp, sizeof(szTemp), szImageFolder);
			StringCchCat(szTemp, sizeof(szTemp), gData.szOutOfBoundsPath);
			StringCchCopy(gData.szOutOfBoundsPath, sizeof(gData.szOutOfBoundsPath), szTemp);
		}
	}

	g_Groups.insert(GroupMap::value_type(std::string(pszPrefix), gData));
}

void AlbumArt::DestroyGroup(LPCSTR pszPrefix)
{
	GroupMap::iterator iter = g_Groups.find(pszPrefix);
	if (iter != g_Groups.end())
	{
		if (iter->second.bThreadIsRunning)
		{
			*(iter->second.threadbTerminate) = true;
		}
		g_Groups.erase(iter);
	}
}

GroupData* AlbumArt::GetGroupByPrefix(LPCSTR pszPrefix)
{
	GroupMap::iterator iter = g_Groups.find(pszPrefix);
	return (iter != g_Groups.end()) ? &(iter->second) : NULL;
}

void AlbumArt::UpdateInformation(bool bInitializing)
{
	HANDLE hWinamp;
	ULONG dWinamp;

	// Get Winamps HWND
	HWND hwndWA2 = FindWindow("Winamp v1.x", NULL);
	if (hwndWA2 == NULL) // Winamp isn't running
	{
		g_iCurrentTrack = -1;
		return HandleError("Error: Unable to find Winamps HWND, most likely Winamp isn't running!");
	}

	// Make sure the track has changed
	int Track = SendMessage(hwndWA2, WM_USER, 0, 125);
	if (!bInitializing)
	{
		if (Track == g_iCurrentTrack)
		{
			return; // The track hasn't changed
		}
	}
	g_iCurrentTrack = Track;
	LiteStep::SetEvar("AlbumArt", "CurrentTrack", "%i", Track + 1);

	if (!bInitializing)
		LiteStep::ExecuteEvent("AlbumArt", "OnTrackChange");

	// Open a handle to Winamp
	GetWindowThreadProcessId(hwndWA2, &dWinamp);
	hWinamp = OpenProcess(PROCESS_VM_READ, false, dWinamp);
	if (hWinamp == NULL) // Failed to open Winamp :/
		return HandleError("Error: OpenProcess Failed!");

	char szTrackPath[MAX_PATH], szCoverPath[MAX_PATH];
	void* winampTrack;
	int PlayListLength = SendMessage(hwndWA2, WM_USER, 0, 124);

	for (GroupMap::iterator iter = g_Groups.begin(); iter != g_Groups.end(); iter++)
	{

		// Assume that if the file doesn't exist we are out of playlist bounds.
		if (Track + iter->second.iTrackOffset < 0 || Track + iter->second.iTrackOffset >= PlayListLength)
		{
			LiteStep::SetEvar(iter->first.c_str(), "IsOutOfBounds", "true");
			if (!iter->second.bIsOutOfBounds)
				LiteStep::ExecuteEvent(iter->second.szPrefix, "OnOutOfBounds");
			iter->second.bIsOutOfBounds = true;
			SetPath(&(iter->second), iter->second.szOutOfBoundsPath);
		}
		else
		{
			// Get the path to the file
			winampTrack = (void*)SendMessage(hwndWA2, WM_USER, Track + iter->second.iTrackOffset, 211);
			ReadProcessMemory(hWinamp, winampTrack, &szTrackPath, sizeof(szTrackPath), NULL);

			if (szTrackPath[4] == ':') // foobar... or streaming
			{
				char szProtocol[MAX_PATH];
				StringCchCopy(szProtocol, sizeof(szProtocol), szTrackPath);
				szProtocol[7] = 0;
				if (_stricmp(szProtocol, "file://") == 0)
					StringCchCopy(szTrackPath, sizeof(szTrackPath), (LPCSTR)(szTrackPath + 7));
				else if (_stricmp(szProtocol, "http://") == 0)
					//TODO::handle streams...
					StringCchCopy(szTrackPath, sizeof(szTrackPath), (LPCSTR)(szTrackPath + 7));
				else
					StringCchCopy(szTrackPath, sizeof(szTrackPath), (LPCSTR)(szTrackPath + 7));
			}
			LiteStep::SetEvar(iter->first.c_str(), "IsOutOfBounds", "false");
			if (!iter->second.bIsOutOfBounds)
				LiteStep::ExecuteEvent(iter->second.szPrefix, "OnIntoBounds");
			iter->second.bIsOutOfBounds = false;
			GetCover(&(iter->second), szTrackPath, szCoverPath, sizeof(szCoverPath));
			SetPath(&(iter->second), szCoverPath);
		}
	}
	CloseHandle(hWinamp);

	// Everything is fine
	StringCchCopy(g_szDebugMessage, sizeof(g_szDebugMessage), "Everything seems to be working properly.");
}

void AlbumArt::GetCover(GroupData *gData, LPCSTR pszTrackPath, LPSTR szCoverPath, UINT cchCoverLength)
{
	if (gData->bSearchFolderBeforeTag)
	{
		if (GetCoverFromFolder(gData, pszTrackPath, szCoverPath, cchCoverLength))
			return;
		else if (GetCoverFromTag(gData, pszTrackPath, szCoverPath, cchCoverLength))
			return;
	}
	else
	{
		if (GetCoverFromTag(gData, pszTrackPath, szCoverPath, cchCoverLength))
			return;
		else if (GetCoverFromFolder(gData, pszTrackPath, szCoverPath, cchCoverLength))
			return;
	}

	// No albumart found
	if (_stricmp(gData->szDownloadPath, "") != 0)
		DownloadCover(gData, pszTrackPath);
	StringCchCopy(szCoverPath, cchCoverLength, gData->szNoArtPath);
	return;
}

bool AlbumArt::GetCoverFromTag(GroupData *gData, LPCSTR pszTrackPath, LPSTR szCoverPath, UINT cchCoverLength)
{
	// Check for cover art in the ID3 tag
	if (strcmp(gData->szParsedPath, "") != 0)
	{
		// Try to get the cover from the ID3 tag
		ID3_Tag myTag(pszTrackPath);
		ID3_Frame* myFrame;

		for (std::vector<UCHAR>::const_iterator iter = gData->ParseTypes.begin(); iter != gData->ParseTypes.end(); iter++)
		{
			myFrame = myTag.Find(ID3FID_PICTURE, ID3FN_PICTURETYPE, *iter);
			if (myFrame != NULL)
			{
				myFrame->Field(ID3FN_DATA).ToFile(gData->szParsedPath);
				StringCchCopy(szCoverPath, cchCoverLength, gData->szParsedPath);
				return true;
			}
		}

		// No ID3 cover found, try FLAC instead
		FLAC::Metadata::Picture flacPicture;
		// Luckily FLAC uses the same picture IDs as ID3
		for (std::vector<UCHAR>::const_iterator iter = gData->ParseTypes.begin(); iter != gData->ParseTypes.end(); iter++)
		{
			if (FLAC::Metadata::get_picture(pszTrackPath, flacPicture, (FLAC__StreamMetadata_Picture_Type)*iter, NULL, NULL, (unsigned)(-1), (unsigned)(-1), (unsigned)(-1), (unsigned)(-1)))
			{
				FILE *fOutPut;
				fopen_s(&fOutPut, gData->szParsedPath, "wb");
				fwrite(flacPicture.get_data(), flacPicture.get_data_length(), 1, fOutPut);
				fclose(fOutPut);
				StringCchCopy(szCoverPath, cchCoverLength, gData->szParsedPath);
				return true;
			}
		}
	}
	return false;
}

bool AlbumArt::GetCoverFromFolder(GroupData *gData, LPCSTR pszTrackPath, LPSTR szCoverPath, UINT cchCoverLength)
{
	// Get the folder path
	char szFolderPath[MAX_PATH] = {0};
	StringCchCopy(szFolderPath, sizeof(szFolderPath), pszTrackPath);
	for (int i = sizeof(szFolderPath); i > 0; i--)
	{
		if (szFolderPath[i] == '\\')
		{
			szFolderPath[++i] = 0;
			break;
		}
	}
	
	// Check each covername
	char szArtPath[MAX_PATH];
	WIN32_FIND_DATA FindFileData;

	for (std::vector<std::string>::const_iterator iter = gData->CoverNames.begin(); iter != gData->CoverNames.end(); iter++)
	{
		StringCchCopy(szArtPath, sizeof(szArtPath), szFolderPath);
		StringCchCat(szArtPath, sizeof(szArtPath), iter->c_str());

		if (FindFirstFile(szArtPath, &FindFileData) != INVALID_HANDLE_VALUE)
		{
			StringCchCopy(szCoverPath, cchCoverLength, szFolderPath);
			StringCchCat(szCoverPath, cchCoverLength, FindFileData.cFileName);
			return true;
		}
	}

	return false;
}

void AlbumArt::HandleError(LPCSTR pszFormat, ...)
{
	va_list argList;
	va_start(argList, pszFormat);
	StringCchVPrintf(g_szDebugMessage, sizeof(g_szDebugMessage), pszFormat, argList);
	va_end(argList);
	for (GroupMap::iterator iter = g_Groups.begin(); iter != g_Groups.end(); iter++)
	{
		SetPath(&(iter->second), iter->second.szNoArtPath, true);
	}
}

// Sets the path evar for a group
void AlbumArt::SetPath(GroupData *gData, LPCSTR pszPath, bool bError)
{
	// Prevent the rest of this code from executing frequently when
	// something isn't working properly and this fuction is called often.
	if (gData->bNoCRC32ForErrors && bError)
		return;

	// Set the evar before executing any events
	LiteStep::SetEvar(gData->szPrefix, "Path", pszPath);

	// Execute AlbumArtOnChange if appropriate
	if (!gData->bNoCRC32)
	{
		// Check if the crc has changed
		ULONG crc = CRC32::crc32(pszPath);
		if (crc != gData->crc)
		{
			gData->crc = crc;
			LiteStep::SetEvar(gData->szPrefix, "CRC", "%X", crc);
			LiteStep::ExecuteEvent(gData->szPrefix, "OnChange");
		}
	}
	else
	{
		LiteStep::SetEvar(gData->szPrefix, "CRC", "");
		LiteStep::ExecuteEvent(gData->szPrefix, "OnChange");
	}

	// Set or clear the error flag
	gData->bNoCRC32ForErrors = bError;
}

// Starts the dowloader thread for the gData group
void AlbumArt::DownloadCover(GroupData *gData, LPCSTR pszTrackPath)
{
	// Initialize a DownLoadCoverData to send to the downloader thread
	DownLoadCoverData CoverData = {0};
	CoverData.bTerminate = false;
	CoverData.bInitalized = false;
	CoverData.gData = gData;

	// If a previous thread is still running, set it's bTerminate flag to true
	if (gData->bThreadIsRunning)
		*gData->threadbTerminate = true;

	// Get the Album name from Winamp
	if (!Utilities::GetExtendedWinampFileInfo(pszTrackPath, "Album", CoverData.szAlbum, sizeof(CoverData.szAlbum)))
		return;

	// Let relative paths be relative to the folder
	if (gData->szDownloadPath[1] != ':') // If it's a relative path
	{
		// Get the path to the folder the song is in
		StringCchCopy(CoverData.szPath, sizeof(CoverData.szPath), pszTrackPath);
		for (int i = sizeof(CoverData.szPath); i > 0; i--)
		{
			if (CoverData.szPath[i] == '\\')
			{
				CoverData.szPath[++i] = 0;
				break;
			}
		}
		// Add the relative path to the end of the folder path
		StringCchCat(CoverData.szPath, sizeof(CoverData.szPath), gData->szDownloadPath);
	}
	else // Just use the absolute path
	{
		StringCchCopy(CoverData.szPath, sizeof(CoverData.szPath), gData->szDownloadPath);
	}

	// Start the downloader thread
	_beginthreadex(NULL, 0, AlbumArt_Download_Thread::DownloadCover_Thread, &CoverData, 0, &CoverData.threadID);

	// We need to give the thread time to copy over the DownLoadCoverData struct to its own memory
	while (!CoverData.bInitalized) // WaitForSingleObject is a pain...
	{
		Sleep(10);
	}

	return;
}

unsigned __stdcall AlbumArt_Download_Thread::DownloadCover_Thread(void *CoverData)
{
	// Copy over the DownLoadCoverData struct from DownloadCover
	DownLoadCoverData cData = {0};
	memcpy(&cData, CoverData, sizeof(cData));

	// Update the GroupData struct
	cData.gData->threadbTerminate = &(cData.bTerminate);
	cData.gData->bThreadIsRunning = true;

	// Signal DownloadCover that we're done accessing it's data structures, and it may finish
	((DownLoadCoverData*)CoverData)->bInitalized = true;

	// Build the URL to parse for covers
	char szAddres[MAX_LINE_LENGTH], szEncodedAlbum[MAX_LINE_LENGTH];
	Utilities::URLEncode(cData.szAlbum, szEncodedAlbum, sizeof(szEncodedAlbum));
	StringCchCopy(szAddres, sizeof(szAddres), "http://www.albumart.org/index.php?searchindex=Music&srchkey=");
	StringCchCat(szAddres, sizeof(szEncodedAlbum), szEncodedAlbum);

	// Retrive the contents of the above URL and store it in cData.szSearchPageContents
	CURL* curlHandle = curl_easy_init();
	curl_easy_setopt(curlHandle, CURLOPT_URL, szAddres);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, DownloadCover_Thread_Reader);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &cData);
	curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);

	// Terminate the thread if ordered to do so
	if (cData.bTerminate)
		return 0;
	
	// Search through the retrived data
	LPCSTR pszData = cData.szSearchPageContents.c_str();
	LPCSTR pszFind1 = strstr(pszData, "javascript:largeImagePopup('");
	if (pszFind1 != NULL)
	{
		LPCSTR pszFind2 = strchr(pszFind1 + 28, '\'');
		if (pszFind2 != NULL)
		{
			LPSTR pszURL = new char[pszFind2-pszFind1 - 27];
			StringCchCopy(pszURL, pszFind2-pszFind1 - 27, pszFind1 + 28);

			curlHandle = curl_easy_init();
			curl_easy_setopt(curlHandle, CURLOPT_URL, pszURL);
			SendMessage(g_hwndMessageHandler, WM_DOWNLOADSTARTED, (WPARAM)cData.gData, NULL);
			FILE * fOut;
			fopen_s(&fOut, cData.szPath, "wb");
			curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, fOut);
			curl_easy_perform(curlHandle);
			fclose(fOut);
			curl_easy_cleanup(curlHandle);

			SendMessage(g_hwndMessageHandler, WM_DOWNLOADFINISHED, (WPARAM)cData.gData, (LPARAM)cData.szPath);

			delete[] pszURL;

		}
	}

	if (!cData.bTerminate) // If no new thread has been started yet, uncheck the threadrunnig flag in the groupdata struct
		cData.gData->bThreadIsRunning = false;

	return 0;
}

// Recives the data from albumart.org and stores it in szSearchPageContents
size_t AlbumArt_Download_Thread::DownloadCover_Thread_Reader(void *ptr, size_t size, size_t nmemb, void *CoverData)
{
	// Pointer to the DownLoadCoverData struct in DownloadCover_Thread's memory
	DownLoadCoverData* cData = (DownLoadCoverData*)CoverData;

	// The amount of data retrived
	size_t DataSize = nmemb*size;

	// Create a new string to hold all the downloaded data
	LPSTR pszData = new char [DataSize+1];
	StringCchCopy(pszData, DataSize+1, (LPCSTR)ptr);

	// Concatenate the downloaded data to the previously downloaded data
	cData->szSearchPageContents += std::string(pszData);

	// Free memory
	delete[] pszData;

	// Returning anything other than nmemb*size will terminate the download
	return (cData->bTerminate) ? 0 : DataSize;
}