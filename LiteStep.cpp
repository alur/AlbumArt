#include "LiteStep.h"

extern HWND g_hParent;

// Sets a LiteStep evar
void LiteStep::SetEvar(LPCSTR pszPrefix, LPCSTR pszEvar, LPCSTR pszFormat, ...)
{
	char szNewEvar[MAX_PATH], szValue[MAX_LINE_LENGTH];
	va_list argList;

	// Join the Prefix with the Evar's name
	StringCchCopy(szNewEvar, sizeof(szNewEvar), pszPrefix);
	StringCchCat(szNewEvar, sizeof(szNewEvar), pszEvar);

	// Create the Evar's value from the Format and the argList
	va_start(argList, pszFormat);
	StringCchVPrintfA(szValue, sizeof(szValue), pszFormat, argList);
	va_end(argList);
	
	// Without this values containing spaces will act weird
	PathQuoteSpaces(szValue);

	// Set the variable
	LSSetVariable(szNewEvar, szValue);
}

// Reads 
void LiteStep::ExecuteEvent(LPCSTR pszPrefix, LPCSTR pszEvent)
{
	char szExecute[MAX_LINE_LENGTH];
	LiteStep::GetPrefixedRCLine(szExecute, sizeof(szExecute), pszPrefix, pszEvent, "");
	LSExecute(g_hParent, szExecute, SW_SHOWNORMAL);
}

// Gets a prefixed LiteStep Line
void LiteStep::GetPrefixedRCLine(LPSTR szDest, UINT cbBuffer, LPCSTR pszPrefix, LPCSTR pszOption, LPCSTR pszDefault)
{
	char szOptionName[MAX_LINE_LENGTH];
	StringCchPrintf(szOptionName, cbBuffer, "%s%s", pszPrefix, pszOption);
	GetRCLine(szOptionName, szDest, cbBuffer, pszDefault);
	PathUnquoteSpaces(szDest);
}

// Gets a prefixed LiteStep bool
bool LiteStep::GetPrefixedRCBool(LPCSTR pszPrefix, LPCSTR pszOption, bool bDefault)
{
	char szOptionName[MAX_LINE_LENGTH];
	StringCchPrintf(szOptionName, MAX_LINE_LENGTH, "%s%s", pszPrefix, pszOption);
	return GetRCBoolDef(szOptionName, bDefault) != FALSE;
}

// Gets a prefixed LiteStep integer
int LiteStep::GetPrefixedRCInt(LPCSTR pszPrefix, LPCSTR pszOption, int iDefault)
{
	char szOptionName[MAX_LINE_LENGTH];
	StringCchPrintf(szOptionName, MAX_LINE_LENGTH, "%s%s", pszPrefix, pszOption);
	return GetRCInt(szOptionName, iDefault);
}