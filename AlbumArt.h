#ifndef ALBUMART_H
#define ALBUMART_H

#include <windows.h>
#include <vector>
#include <map>
#include <string>
#include "lsapi.h"

// Case insensitive string comparison
struct stringicmp {
    bool operator()(const std::string& s1, const std::string& s2) const {
        return (lstrcmpi(s1.c_str(), s2.c_str()) < 0);
    }
};

// (group) Variables
typedef struct GroupData {
	char szPrefix[MAX_LINE_LENGTH];
	// Paths
	char szNoArtPath[MAX_PATH];
	char szParsedPath[MAX_PATH];
	char szDownloadPath[MAX_PATH];
	char szOutOfBoundsPath[MAX_PATH];
	// Other user set variables
	std::vector<std::string> CoverNames;
	std::vector<UCHAR> ParseTypes;
	int iTrackOffset;
	bool bNoCRC32;
	bool bSearchFolderBeforeTag;
	// Module maintained variables
	bool bNoCRC32ForErrors; // 
	bool bIsOutOfBounds; // 
	ULONG crc; // The crc32 of the current cover art
	bool bThreadIsRunning; // Whether or not this group has a downloader thread running
	bool *threadbTerminate; // Pointer to the bTerminate variable in the currently running downloader thread (if there is one)
} GroupData;
typedef std::map<std::string, GroupData, stringicmp> GroupMap;

// Data passed between the downloader thread and the main thread
typedef struct DownLoadCoverData {
	char szAlbum[MAX_LINE_LENGTH]; // Name of the album to locate covers for
	char szPath[MAX_PATH]; // Where to save the cover when one is found
	std::string szSearchPageContents; // Holds the data returned from albumart.org
	GroupData *gData; // Pointer to a GroupData 
	UINT uMinWidth;
	UINT uMinHeight;
	UINT uMaxHeight;
	UINT uMaxWidth;
	unsigned threadID;
	bool bTerminate;
	bool bInitalized;
} DownLoadCoverData;

namespace AlbumArt
{
	void UpdateInformation(bool bInitializing = false);
	void LoadSettings (bool bIsRefresh = false);
	void GetCover(GroupData *gData, LPCSTR pszTrackPath, LPSTR szCoverPath, UINT cchCoverLength);
	bool GetCoverFromTag(GroupData *gData, LPCSTR pszTrackPath, LPSTR szCoverPath, UINT cchCoverLength);
	bool GetCoverFromFolder(GroupData *gData, LPCSTR pszTrackPath, LPSTR szCoverPath, UINT cchCoverLength);
	void SetPath(GroupData *gData, LPCSTR pszPath, bool bError = false);
	void CreateGroup(LPCSTR pszPrefix);
	void DestroyGroup(LPCSTR pszPrefix);
	void HandleError(LPCSTR pszFormat, ...);
	void DownloadCover(GroupData *gData, LPCSTR pszTrackPath);
	GroupData* GetGroupByPrefix(LPCSTR pszPrefix);
}

namespace AlbumArt_Download_Thread
{
	unsigned __stdcall DownloadCover_Thread(void *CoverData);
	size_t DownloadCover_Thread_Reader(void *ptr, size_t size, size_t nmemb, void *CoverData);
}

#endif