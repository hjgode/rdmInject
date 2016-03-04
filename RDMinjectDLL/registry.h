//registry.h

#pragma once

#ifndef _REG_LIBRARAY_
#define _REG_LIBRARAY_

//Create, write and read registry keys and values
#include <stdio.h>

//global
static HKEY g_hkey=NULL;
static TCHAR g_subkey[MAX_PATH+1]=L"init";

//functions
int OpenKey();
int OpenKey(TCHAR *subkey);
int OpenCreateKey(TCHAR *subkey);
int CloseKey();
int CreateSubKey(TCHAR *subkey);
int RegReadDword(TCHAR *valuename, DWORD *value);
int RegReadStr(TCHAR *valuename, TCHAR *value);

int RegWriteDword(TCHAR *valuename, DWORD *value);
int RegWriteStr(TCHAR *valuename, TCHAR *str);
int RegWriteByte(TCHAR *valuename, byte value);

int RegDelValue(TCHAR *valuename);
int RegDelKey(TCHAR *keyname);

DWORD RegDeleteKeyAll(HKEY hStartKey , LPTSTR pKeyName );
DWORD RegDeleteValAll(HKEY hStartKey, LPTSTR pKeyName );

void ShowError(LONG er);

int IsIntermec(void);
int ReadBuildNumber(TCHAR *szBuildNumber);
int ReadPlatformName(TCHAR *szPlatformName);

//===============================================================
int ReadBuildNumber(TCHAR *szBuildNumber)
{
	HKEY oldKey=g_hkey;
	wsprintf(szBuildNumber, L"unknown");
	int ec;
	ec = OpenKey(L"Platform");
	if (ec == ERROR_SUCCESS)
	{
		ec = RegReadStr(L"Software Build Number", szBuildNumber);
		g_hkey=oldKey;
		return ec;
	}
	else
	{
		g_hkey=oldKey;
		return ec;
	}
}

int ReadPlatformName(TCHAR *szPlatformName)
{
	HKEY oldKey=g_hkey;
	wsprintf(szPlatformName, L"unknown");
	int ec;
	ec = OpenKey(L"Platform");
	if (ec == ERROR_SUCCESS)
	{
		ec = RegReadStr(L"Name", szPlatformName);
		g_hkey=oldKey;
		return ec;
	}
	else
	{
		g_hkey=oldKey;
		return ec;
	}
}

int RegWriteDword(TCHAR *valuename, DWORD *value)
{
	LONG rc=0;
	if (g_hkey==NULL)
		rc = OpenKey();
	rc = RegSetValueEx(	g_hkey, 
						valuename, 
						NULL,
						REG_DWORD, 
						(LPBYTE) value,
						sizeof(DWORD)); 
 
	return rc;
}

int RegWriteByte(TCHAR *valuename, byte value)
{
	LONG rc=0;
	byte b = value;
	if (g_hkey==NULL)
		rc = OpenKey();
	rc = RegSetValueEx(	g_hkey, 
						valuename, 
						NULL,
						REG_BINARY, 
						&b,
						sizeof(byte)); 
 
	return rc;
}

int RegWriteStr(TCHAR *valuename, TCHAR *str)
{
	LONG rc=0;
	if (g_hkey==NULL)
		rc = OpenKey();
	TCHAR txt[MAX_PATH+1];
	wcscpy(txt, str);
	rc = RegSetValueEx(	g_hkey, 
						valuename, 
						NULL,
						REG_SZ, 
						(LPBYTE)txt,
						(wcslen(txt) + 1) * sizeof(txt[0]));
 	return rc;
}

int RegReadByte(TCHAR *valuename, byte *value)
{
	static byte dwResult;
	LONG rc;
	DWORD dwType=REG_BINARY;
	DWORD dwSize=sizeof(byte);
	if (g_hkey==NULL)
		rc = OpenKey();
	if (g_hkey != NULL)
	{
		rc = RegQueryValueEx(g_hkey, valuename, NULL, &dwType, &dwResult, &dwSize);
		if (rc == ERROR_SUCCESS)
		{
			CloseKey();
			*value = dwResult;
			return rc;
		}
	}
	CloseKey();
	return rc;
}


//RegReadDword
int RegReadDword(TCHAR *valuename, DWORD *value)
{
	static DWORD dwResult;
	//DWORD *pdwResult = &dwResult;

	LONG rc;
	DWORD dwType=REG_DWORD;
	DWORD dwSize=sizeof(DWORD);
	if (g_hkey==NULL)
		rc = OpenKey();
	if (g_hkey != NULL)
	{
		rc = RegQueryValueEx(g_hkey, valuename, NULL, &dwType, (LPBYTE) dwResult, &dwSize);
		if (rc == ERROR_SUCCESS)
		{
			CloseKey();
			*value = dwResult;
			return rc;
		}
	}
	CloseKey();
	return rc;
}

//RegReadStr
int RegReadStr(TCHAR *valuename, TCHAR *value)
{
	static TCHAR szStr[MAX_PATH+1];
	LONG rc;
	DWORD dwType=REG_SZ;
	DWORD dwSize=0;
	if (g_hkey == NULL)
	{
		if (OpenKey()==0) //use default g_hkey
		{
			dwSize = sizeof(szStr) * sizeof(TCHAR);
			rc = RegQueryValueEx(g_hkey, valuename, NULL, &dwType, (LPBYTE)szStr, &dwSize);
			if (rc == ERROR_SUCCESS)
			{
				CloseKey();
				wcscpy(value, szStr);
				return 0;
			}
		}
	}
	else
	{
		//use already opened g_hkey
		dwSize = sizeof(szStr) * sizeof(TCHAR);
		rc = RegQueryValueEx(g_hkey, valuename, NULL, &dwType, (LPBYTE)szStr, &dwSize);
		if (rc == ERROR_SUCCESS)
		{
			CloseKey();
			wcscpy(value, szStr);
			return 0;
		}
	}

	wcscpy(value, L"");
	CloseKey();
	return -1;
}

//OpenKey to iHook2
int OpenKey()
{
	//open key to gain access to subkeys
	LONG rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        g_subkey, 
        0,
        0, 
        &g_hkey);
	if (rc == ERROR_SUCCESS)
		return 0;
	else
	{
		g_hkey=NULL;
		return rc;
	}
}

int OpenCreateKey(TCHAR *subkey)
{
	DWORD dwDisp;
	LONG rc;
	if (wcslen(subkey)==0)
		wcscpy(subkey, g_subkey);
	//create the key if it does not exist
	rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
						subkey, 
						0, 
						NULL, 
						0, 
						0, 
						NULL,
						&g_hkey,
						&dwDisp);
	return rc;
}

//OpenKey with a specified SubKey
int OpenKey(TCHAR *subkey)
{
	//open key to gain access to subkeys
	LONG rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        subkey, 
        0,
        0, 
        &g_hkey);
	if (rc == ERROR_SUCCESS)
	{
		wsprintf(g_subkey, L"%s", subkey);
		return 0;
	}
	else
	{
		g_hkey=NULL;
		return rc;
	}
}

int RegDelValue(TCHAR *valuename)
{
	if (g_hkey==NULL)
	{
		if (OpenKey()!=0)
			return -1; //could not open default key
	}
	if ( ERROR_SUCCESS == RegDeleteValue(g_hkey, valuename) )
		return 0;	//success
	else
	{
/*
#ifdef DEBUG
		ShowError(GetLastError());
#endif
*/
		return -2; //error deleting key
	}
}

DWORD RegDeleteValAll(HKEY hStartKey, LPTSTR pKeyName )
{
   DWORD   dwRtn, dwValueLength;
   LPTSTR  pSubKey = NULL;
   TCHAR   szValue[MAX_PATH]; // (256) this should be dynamic.
   HKEY    hKey;
 
   // do not allow NULL or empty key name
   if ( pKeyName &&  lstrlen(pKeyName))
   {
      if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
         0, 0, &hKey )) == ERROR_SUCCESS)
      {
         while (dwRtn == ERROR_SUCCESS )
         {
			 //enum all values and delete them
            dwValueLength = MAX_PATH;
            dwRtn=RegEnumValue(
                           hKey,
                           0,				// always index zero
                           szValue,			// lpValueName, pointer to name (out)
                           &dwValueLength,	// lpcchValueName, len of szValue (in|out)
                           NULL,			// lpReserved
                           NULL,			// lpType (not of interest here)
                           NULL,			// lpData (not of interest here)
                           NULL				// lpcbData (not of interest here)
                         );
 
            if(dwRtn == ERROR_NO_MORE_ITEMS)
            {
				DEBUGMSG(1, (L"#No more values...\n"));
                break;
            }
            else if(dwRtn == ERROR_SUCCESS)
			{
				DEBUGMSG(1, (szValue));
				DEBUGMSG(1, (L"...Deleting value\n"));
               dwRtn=RegDeleteValue(hKey, szValue);
			}
         }
         RegCloseKey(hKey);
         // Do not save return code because error
         // has already occurred
      }
   }
   else
      dwRtn = ERROR_BADKEY;
 
   return dwRtn;
}

//delete all EMPTY keys (the ones with no more subkeys)
DWORD RegDeleteKeyAll(HKEY hStartKey , LPTSTR pKeyName )
{
   DWORD   dwRtn, dwSubKeyLength;
   LPTSTR  pSubKey = NULL;
   TCHAR   szSubKey[MAX_PATH]; // (256) this should be dynamic.
   HKEY    hKey;
 
   // do not allow NULL or empty key name
   if ( pKeyName &&  lstrlen(pKeyName))
   {
      if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
         0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
      {
         while (dwRtn == ERROR_SUCCESS )
         {
            dwSubKeyLength = MAX_PATH;
            dwRtn=RegEnumKeyEx(
                           hKey,
                           0,       // always index zero
                           szSubKey,
                           &dwSubKeyLength,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );
 
            if(dwRtn == ERROR_NO_MORE_ITEMS)
            {
				//remove all values
				dwRtn = RegDeleteValAll(hStartKey, pKeyName);
				if ( (dwRtn == ERROR_SUCCESS) || (dwRtn == ERROR_NO_MORE_ITEMS))
					dwRtn = RegDeleteKey(hStartKey, pKeyName);
               break;
            }
            else if(dwRtn == ERROR_SUCCESS)
               dwRtn=RegDeleteKeyAll(hKey, szSubKey); //dive into subkey and delete there
         }
         RegCloseKey(hKey);
         // Do not save return code because error
         // has already occurred
      }
   }
   else
      dwRtn = ERROR_BADKEY;
 
   return dwRtn;
}

int RegDelKey(TCHAR *keyname)
{
	DWORD rc;
	if (g_hkey==NULL)
	{
		if (OpenKey()!=0)
			return -1; //could not open default key
	}
	//enum all sub values and delete them
	rc = RegDeleteKeyAll(g_hkey, keyname);
	//rc = RegDeleteKey(g_hkey, keyname);
	if (ERROR_SUCCESS == rc)
		return 0;	//success
	else
	{
#ifdef DEBUG
		ShowError(rc);
#endif
		return rc; //error deleting key
	}
}

//close the global g_hkey
int CloseKey()
{
	if (g_hkey == NULL)
		return 0;
	LONG rc = RegCloseKey(g_hkey);
	g_hkey=NULL;
	return rc;
}

//will open or create a subkey and changes global g_hkey
int CreateSubKey(TCHAR *subkey)
{
	DWORD dwDisp;
	//L"\\Software\\Intermec\\iHook2"
	LONG rc = RegCreateKeyEx (HKEY_LOCAL_MACHINE, 
 					 subkey, 
					 0,
                     TEXT (""), 
					 0, 
					 0, 
					 NULL, 
                     &g_hkey, 
					 &dwDisp);
	return rc;
}

void ShowError(LONG er)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		er,
		0, // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	TCHAR temp[MAX_PATH];
	wsprintf(temp, (LPTSTR)lpMsgBuf);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
#ifdef DEBUG
	DEBUGMSG(1, ((LPCTSTR)lpMsgBuf));
#else
	// Display the string.
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, L"Error", MB_OK | MB_ICONINFORMATION );
#endif
	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//////////////////////////////////////////////////////////////////////////////////
// IsIntermec will test a reg key and return 0, if it contains Intermec
//////////////////////////////////////////////////////////////////////////////////
int IsIntermec(void)
{
	FILE *stream;
	//testing for intermec
	stream = fopen ("\\Windows\\itc50.dll", "r");
	if(stream != NULL){
		fclose(stream);
		return 0;
	}
	//testing for HHP (Dolphin 70e etc.)
	stream = fopen ("\\windows\\HHPScanDriver.dll", "r");
	if(stream != NULL){
		fclose(stream);
		return 0;
	}
	OpenKey(L"Platform");
	if (g_hkey != NULL)
	{
		TCHAR val[MAX_PATH+1];
		if ( RegReadStr(L"Name", val) == 0) //no error?
		{
			if ( wcsstr(val, L"Intermec") != NULL )
				return 0; //OK
		}
		//CHANGED to work with CK31 too (23 jan 2007)
		//could not read platform\name?
		OpenKey(L"SOFTWARE\\Intermec\\Version"); //separate check for ck60
		if (g_hkey != NULL)
		{
			if ( RegReadStr(L"IVA", val) == 0) //no error?
				return 0;
			else
				return -3; //could not read IVA
		}
		else
			return -2; //could not openkey
		
	}
	else
		return -1;
}

#endif