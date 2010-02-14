#include <fstream>
#include "crc32.h"

// Global Variables
ULONG g_crcTable[256]; // For faster CRC computations, must be initalized by crcInit()

// 
ULONG CRC32::reflect(ULONG data, UCHAR nBits)
{
	ULONG reflection = 0x00000000;
	for (UCHAR bit = 0; bit < nBits; ++bit)
	{
		if (data & 0x01)
		{
			reflection |= (1 << ((nBits - 1) - bit));
		}
		data = (data >> 1);
	}
	return (reflection);
}

// Compute the remainder of each possible dividend.
void CRC32::crcInit()
{
	for (int dividend = 0; dividend < 256; dividend++)
	{
		ULONG remainder = dividend << (8 * sizeof(ULONG) - 8);
		for (UCHAR bit = 8; bit > 0; bit--)
		{		
			remainder = (remainder & (1 << (8 * sizeof(ULONG) - 1))) ? (remainder << 1) ^ 0x04C11DB7 : (remainder << 1);
		}
		g_crcTable[dividend] = remainder;
	}
}

// Calculates the CRC32 of a file
ULONG CRC32::crc32(LPCSTR pszFilePath)
{
    ULONG remainder = 0xFFFFFFFF;
    UCHAR data;
	std::ifstream file;

	file.open(pszFilePath, std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open())
		return 0;

	std::ifstream::pos_type size = file.tellg();
	LPSTR szContents = new char [size];
	file.seekg (0, std::ios::beg);
    file.read (szContents, size);
    file.close();

    for (int byte = 0; byte < size; ++byte)
    {
        data = (UCHAR)reflect((szContents[byte]) ^ (remainder >> (8 * sizeof(ULONG) - 8)), 8);
  		remainder = g_crcTable[data] ^ (remainder << 8);
    }

	delete[] szContents;

    return reflect(remainder ^ 0xFFFFFFFF, 8 * sizeof(ULONG));
}