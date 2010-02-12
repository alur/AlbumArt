#ifndef CRC32_H
#define CRC32_H

#include <windows.h>
#include <fstream>

namespace CRC32
{
	void crcInit();
	ULONG crc32(LPCSTR pszFilePath);
	ULONG reflect(ULONG data, UCHAR nBits);
}

#endif