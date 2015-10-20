// StartRDMinject.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "StartRDMinject.h"

#include "tlhelp32.h"
#pragma comment(lib, "toolhelp.lib")
#ifndef TH32CS_SNAPNOHEAPS
	#define TH32CS_SNAPNOHEAPS 0x40000000
#endif

#define INJECT_DLL_NAME L"\\Windows\\RDMinjectDll.dll"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE			g_hInst;			// current instance
HWND				g_hWndMenuBar;		// menu bar handle

// Forward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

/*$DOCBEGIN$
 =======================================================================================================================
 *    £
 *    BOOL IsProcessRunning( TCHAR * pname ); £
 *    * Description: Get process table snapshot, look for pname running. £
 *    * Arguments: pname - pointer to name of program to look for. £
 *    for example, app.exe. £
 *    * Returns: TRUE - process is running. £
 *    FALSE - process is not running. £
 *    $DOCEND$ £
 *
 =======================================================================================================================
 */

BOOL IsProcessRunning( TCHAR *pname )
{
    HANDLE          hProcList;
    PROCESSENTRY32  peProcess;
    DWORD           thDeviceProcessID;
    TCHAR           lpname[MAX_PATH];
    if ( wcslen(pname)==0 )
    {
        return FALSE;
    }

    wcscpy( lpname, pname );
    _tcslwr( lpname );
    hProcList = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS | TH32CS_SNAPNOHEAPS, 0 );
    if ( hProcList == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }       /* end if */

    memset( &peProcess, 0, sizeof(peProcess) );
    peProcess.dwSize = sizeof( peProcess );
    if ( !Process32First( hProcList, &peProcess ) )
    {
        CloseToolhelp32Snapshot( hProcList );
        return FALSE;
    }       /* end if */

    thDeviceProcessID = 0;
    do
    {
        //_wcslwr( peProcess.szExeFile );
        if ( wcsicmp( peProcess.szExeFile, lpname ) == 0) //replaced wcsstr by wcsicmp
        {
            thDeviceProcessID = peProcess.th32ProcessID;
            break;
        }   /* end if */
    }
    while ( Process32Next( hProcList, &peProcess ) );
    if ( (GetLastError() == ERROR_NO_MORE_FILES) && (thDeviceProcessID == 0) )
    {
        CloseToolhelp32Snapshot( hProcList );
        return FALSE;
    }       /* end if */

    CloseToolhelp32Snapshot( hProcList );
    return TRUE;
}           /* IsProcessRunning */

DWORD FindPID(HWND hWnd){
	DWORD dwProcID = 0;
	DWORD threadID = GetWindowThreadProcessId(hWnd, &dwProcID);
	if(dwProcID != 0)
		return dwProcID;
	return 0;
}
//
// FindPID will return ProcessID for an ExeName
// retruns 0 if no window has a process created by exename
//
DWORD FindPID(TCHAR *exename)
{
	DWORD dwPID=0;
	TCHAR exe[MAX_PATH];
	wsprintf(exe, L"%s", exename);
	//Now make a snapshot for all processes and find the matching processID
  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPNOHEAPS, 0);
  if (hSnap != NULL)
  {
	  PROCESSENTRY32 pe;
	  pe.dwSize = sizeof(PROCESSENTRY32);
	  if (Process32First(hSnap, &pe))
	  {
		do
		{
		  if (wcsicmp (pe.szExeFile, exe) == 0)
		  {
			CloseToolhelp32Snapshot(hSnap);
			dwPID=pe.th32ProcessID ;
			return dwPID;
		}
		} while (Process32Next(hSnap, &pe));
	  }//processFirst
  }//hSnap
  CloseToolhelp32Snapshot(hSnap);
  return 0;

}

BOOL foundIt=FALSE;
BOOL killedIt=FALSE;
HWND hWindow=NULL;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) //find window for PID
{
	DWORD wndPid;
//	TCHAR Title[128];
	// lParam = procInfo.dwProcessId;
	// lParam = exename;

	// This gets the windows handle and pid of enumerated window.
	GetWindowThreadProcessId(hwnd, &wndPid);

	// This gets the windows title text
	// from the window, using the window handle
	//  GetWindowText(hwnd, Title, sizeof(Title)/sizeof(Title[0]));
	if (wndPid == (DWORD) lParam)
	{
		//found matching PID
		foundIt=TRUE;
		hWindow=hwnd;
		return FALSE; //stop EnumWindowsProc
	}
	return TRUE;
}
BOOL KillExeWindow(TCHAR* exefile)
{
	//go thru all top level windows
	//get ProcessInformation for every window
	//compare szExeName to exefile
	// upper case conversion?
	foundIt=FALSE;
	killedIt=FALSE;
	//first find a processID for the exename
	DWORD dwPID = FindPID(exefile);
	if (dwPID != 0)
	{
		//now find the handle for the window that was created by the exe via the processID
		EnumWindows(EnumWindowsProc, (LPARAM) dwPID);
		if (foundIt)
		{
			//now try to close the window
			if (PostMessage(hWindow, WM_QUIT, 0, 0) == 0) //did not success?
			{
				//try the hard way
				HANDLE hProcess = OpenProcess(0, FALSE, dwPID);
				if (hProcess != NULL)
				{
					DWORD uExitCode=0;
					if ( TerminateProcess (hProcess, uExitCode) != 0)
					{
						//app terminated
						killedIt=TRUE;
					}
				}

			}
			else
				killedIt=TRUE;
		}
		else
		{
			//no window
			//try the hard way
			HANDLE hProcess = OpenProcess(0, FALSE, dwPID);
			if (hProcess != NULL)
			{
				DWORD uExitCode=0;
				if ( TerminateProcess (hProcess, uExitCode) != 0)
				{
					//app terminated
					killedIt=TRUE;
				}
				else
					killedIt=FALSE;
			}
		}
	}
	return killedIt;
}


//try to launch RDM
int startRDM(){
	DEBUGMSG(1, (L"Entering startRDM()\n"));
	int cnt=0; //repeat counter
	BOOL isOK = TRUE;
	int iRet=0;

	do{
		cnt++;
		//if tsc is already running, kill it

		//first ensure TSSHELLWND is not minimized or connect will hang (why?)
#if _WIN32_WCE == 0x420
		HWND hwndTSC = FindWindow(L"UIMainClass", NULL);//FindWindow(NULL, L"Terminal Services Client");	
		if(hwndTSC==NULL)
			hwndTSC = FindWindow(NULL, L"Terminal Services Client");
		//at start we see the 'connect' dialog
		//in a connected session the class and title changes!
#else
		HWND hwndTSC = FindWindow(L"TSSHELLWND",NULL);
#endif
		if(hwndTSC==NULL)
			DEBUGMSG(1, (L"TSC is NOT running\n"));
		else
			DEBUGMSG(1, (L"TSC is running as window: 0x%08x\n",hwndTSC));

		if(hwndTSC!=NULL)
			ShowWindow(hwndTSC, SW_SHOWNORMAL);
#if _WIN32_WCE == 0x420
		if(IsProcessRunning(L"mstsc40.exe")){		//on pocketpc we have mstsc40.exe
			if( KillExeWindow(L"mstsc40.exe") ){
#else
		if(IsProcessRunning(L"wpctsc.exe")){
			if( KillExeWindow(L"wpctsc.exe") ){
#endif
				//killedit OK
				Sleep(1000);
			}
			else{
				//was unable to kill
				iRet = -1; //unable to kill wpctsc
				continue;
			}
		}

		DWORD dProcIDTSC=0; //to save proc ID of TSC main window
		//start a new instance of tsc
		PROCESS_INFORMATION pi;
#if _WIN32_WCE == 0x420
		if (CreateProcess(L"\\windows\\mstsc40.exe", L"", NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi)!=0)
#else
		if (CreateProcess(L"\\windows\\wpctsc.exe", L"", NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi)!=0)
#endif
		{
			//OK
			Sleep(1000); //give some time to setup
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
		else
		{
			//start failed
			iRet = -2; //unable to start wpctsc
			continue;
		}
	}while (!isOK && cnt<3);
	DEBUGMSG(1, (L"Leaving startRDM() with code: %i\n", iRet));
	return iRet;
}

DWORD AddDllNameToRegistry(TCHAR *szDllName)
{
	/* 
	 * Injecting a DLL into a Process Space (Windows Embedded CE 6.0)
	 * http://msdn.microsoft.com/en-us/library/ee483158(v=winembedded.60).aspx
	 *
	 * There are times when a DLL must be automatically added to a process 
	 * space to perform a required action, for example, a debugging tool 
	 * that tracks some actions.
	 * 
	 * If neither the source code nor the ability to build the code is 
	 * available, you are limited in the kinds of debugging you can perform. 
	 * The kernel can load a DLL into any process space.
	 *
	 * To enable this process, add the name of the DLL to the following 
	 * registry key:
	 *
	 *  HKEY_LOCAL_MACHINE\SYSTEM\KERNEL
	 *     "InjectDLL" = REG_MULTI_SZ : "MyDLL1.DLL","MyDLL2.DLL",
	 *
	 * The data type is REG_MULTI_SZ or an array of strings, which can list 
	 * more than one DLL. The name of the DLL can contain the full path and 
	 * file name, or just the file name.
	 *
	 * Applications that install DLLs to be injected into a process should 
	 * append or remove only their specific DLL from the registry.
	 *
	 * When a process is created and all implicitly-linked DLLs are loaded, 
	 * the kernel loads each DLL listed in InjectDLL. Failure to load the 
	 * DLL does not prevent the application from launching. A DLL could fail 
	 * to load if the DLL returns FALSE in DllMain or if the application is 
	 * trusted and the DLL being loaded is not. DLLs can check in DLLMain to 
	 * determine what process they are being loaded into by calling 
	 * GetModuleFileName. 
	 * The values you need to pass are (NULL, &Filename, nSize).
	 *
	 */
	DWORD dwDisposition = 0;
	HKEY  hKeyReg = NULL;
	LONG  lRet = ERROR_SUCCESS;
	
	DEBUGMSG(1, (L"AddDllNameToRegistry-RegCreateKeyEx HKLM/System/Kernel...\n"));
	if ((lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
		                       L"System\\Kernel", 
							   0,
							   NULL,
							   0,
							   0,
							   NULL,
							   &hKeyReg,
							   &dwDisposition)) != ERROR_SUCCESS)
	{
		DWORD dwError = GetLastError();
		DEBUGMSG(1, (L"AddDllNameToRegistry-RegCreateKeyEx HKLM/System/Kernel failed: %i\n", dwError));
		//TCHAR szError[MAX_PATH] = {0};
		//wsprintf(szError,
		//	L"RegCreateKeyEx() failed with %d [0x%08X]",
		//	dwError,
		//	dwError);
		//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
		return (DWORD)lRet;
	}

	if (dwDisposition == REG_CREATED_NEW_KEY) // The key did not exist and was created.
	{
		DEBUGMSG(1, (L"AddDllNameToRegistry-RegCreateKeyEx HKLM/System/Kernel NEW_KEY\n"));
		// Array of null-terminated strings, must be terminated by two null characters
		UINT cbBytes = wcslen(szDllName) * sizeof(WCHAR) + 2;
		BYTE *bData = (BYTE *)LocalAlloc(LPTR, cbBytes);
		if (bData == NULL)
		{
			DWORD dwError = GetLastError();
			DEBUGMSG(1, (L"AddDllNameToRegistry-RegCreateKeyEx HKLM/System/Kernel LocalAlloc() failed: %i\n", dwError));
			//TCHAR szError[MAX_PATH] = {0};
			//wsprintf(szError,
			//	L"LocalAlloc() failed with %d [0x%08X]",
			//	dwError,
			//	dwError);
			//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
			RegCloseKey(hKeyReg);
			return (DWORD)lRet;
		}

		memset(bData, 0, cbBytes);
		memcpy(bData, szDllName, wcslen(szDllName) * sizeof(WCHAR));

		DEBUGMSG(1, (L"AddDllNameToRegistry-RegSetValueEx HKLM/System/Kernel/InjectDLL ...\n"));
		if ((lRet = RegSetValueEx(hKeyReg,
			                      L"InjectDLL",
								  NULL,
								  REG_MULTI_SZ,
								  bData,
								  cbBytes)) != ERROR_SUCCESS)
		{
			DWORD dwError = GetLastError();
			DEBUGMSG(1, (L"AddDllNameToRegistry-RegSetValueEx HKLM/System/Kernel/InjectDLL failed: %i\n", dwError));
			//TCHAR szError[MAX_PATH] = {0};
			//wsprintf(szError,
			//	L"RegSetValueEx() failed with %d [0x%08X]",
			//	dwError,
			//	dwError);
			//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
			LocalFree(bData);
			RegCloseKey(hKeyReg);
			return (DWORD)lRet;
		}
		LocalFree(bData);
		RegCloseKey(hKeyReg);
		DEBUGMSG(1, (L"AddDllNameToRegistry-RegSetValueEx HKLM/System/Kernel/InjectDLL OK: '%s'\n", szDllName));
		return ERROR_SUCCESS;
	}

	DWORD dwSize = 0;
	DWORD dwType = REG_MULTI_SZ;

	DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx HKLM/System/Kernel/InjectDLL... \n"));
	// Retrieve the size of the MULTI_SZ_REG value
	if ((lRet = RegQueryValueEx(hKeyReg,
		                        L"InjectDLL",
								NULL,
								&dwType,
								NULL,
								&dwSize)) != ERROR_SUCCESS)
	{
		if (lRet == ERROR_FILE_NOT_FOUND)
		{
			DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx HKLM/System/Kernel/InjectDLL: ERROR_FILE_NOT_FOUND... \n"));
			// Array of null-terminated strings, must be terminated by two null characters
			UINT cbBytes = wcslen(szDllName) * sizeof(WCHAR) + 2;
			
			BYTE *bData = (BYTE *)LocalAlloc(LPTR, cbBytes);
			if (bData == NULL)
			{
				DWORD dwError = GetLastError();
				DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx HKLM/System/Kernel/InjectDLL: LocalAlloc() failed: %i\n", dwError));
				//TCHAR szError[MAX_PATH] = {0};
				//wsprintf(szError,
				//	L"LocalAlloc() failed with %d [0x%08X]",
				//	dwError,
				//	dwError);
				//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
				RegCloseKey(hKeyReg);
				return (DWORD)lRet;
			}

			memset(bData, 0, cbBytes);
			memcpy(bData, szDllName, wcslen(szDllName) * sizeof(WCHAR));

			DEBUGMSG(1, (L"AddDllNameToRegistry-RegSetValueEx HKLM/System/Kernel/InjectDLL...\n"));
			if ((lRet = RegSetValueEx(hKeyReg,
									  L"InjectDLL",
									  NULL,
									  REG_MULTI_SZ,
									  bData,
									  cbBytes)) != ERROR_SUCCESS)
			{
				DWORD dwError = GetLastError();
				DEBUGMSG(1, (L"AddDllNameToRegistry-RegSetValueEx HKLM/System/Kernel/InjectDLL: failed: %i\n", dwError));
				//TCHAR szError[MAX_PATH] = {0};
				//wsprintf(szError,
				//	L"RegSetValueEx() failed with %d [0x%08X]",
				//	dwError,
				//	dwError);
				//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
				LocalFree(bData);
				RegCloseKey(hKeyReg);
				return (DWORD)lRet;
			}
			LocalFree(bData);
			RegCloseKey(hKeyReg);
			return ERROR_SUCCESS;		
		}
		else
		{
			DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx HKLM/System/Kernel/InjectDLL: failed: %i\n", lRet));
			//TCHAR szError[MAX_PATH] = {0};
			//wsprintf(szError,
			//	L"[a] RegQueryValueEx() failed with %d [0x%08X]",
			//	lRet,
			//	lRet);
			//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
			RegCloseKey(hKeyReg);
			return (DWORD)lRet;
		}
	}
	
	UINT cbBytes = dwSize + (wcslen(szDllName) * sizeof(WCHAR));
	BYTE *bData = (BYTE *)LocalAlloc(LPTR, cbBytes);
	if (bData == NULL)
	{
		DWORD dwError = GetLastError();
		DEBUGMSG(1, (L"AddDllNameToRegistry-LocalAlloc() HKLM/System/Kernel/InjectDLL: failed: %i\n", dwError));
		//TCHAR szError[MAX_PATH] = {0};
		//wsprintf(szError,
		//	L"LocalAlloc() failed with %d [0x%08X]",
		//	dwError,
		//	dwError);
		//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
		RegCloseKey(hKeyReg);
		return (DWORD)lRet;
	}

	memset(bData, 0, cbBytes);
	DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx: HKLM/System/Kernel/InjectDLL...\n"));
	if ((lRet = RegQueryValueEx(hKeyReg,
		                        L"InjectDLL",
								NULL,
								&dwType,
								bData,
								&dwSize)) != ERROR_SUCCESS)
	{
		DWORD dwError = GetLastError();
		DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx: HKLM/System/Kernel/InjectDLL failed %i\n", dwError));
		//TCHAR szError[MAX_PATH] = {0};
		//wsprintf(szError,
		//	L"[b] RegQueryValueEx() failed with %d [0x%08X]",
		//	dwError,
		//	dwError);
		//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
		LocalFree(bData);
		RegCloseKey(hKeyReg);
		return (DWORD)lRet;
	}

	WCHAR *p = (WCHAR *)bData;
	while (*p != NULL)
	{
		if (_wcsicmp(p, szDllName) == 0) 
		{
			DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx: HKLM/System/Kernel/InjectDLL The dll name is already present in REG_MULTI_SZ\n"));
			// The dll name is already present in REG_MULTI_SZ,
			// let's exit...
			LocalFree(bData);
			RegCloseKey(hKeyReg);
			return ERROR_SUCCESS;
		}
		p += wcslen(p) + 1;
	}

	// The dll name is NOT present in REG_MULTI_SZ,
	// let's add it...

	memcpy(p, szDllName, wcslen(szDllName) * sizeof(WCHAR));
	DEBUGMSG(1, (L"AddDllNameToRegistry-RegQueryValueEx: HKLM/System/Kernel/InjectDLL The dll name is NOT present in REG_MULTI_SZ...\n"));
	if ((lRet = RegSetValueEx(hKeyReg,
							  L"InjectDLL",
							  NULL,
							  REG_MULTI_SZ,
							  bData,
							  cbBytes)) != ERROR_SUCCESS)
	{
		DWORD dwError = GetLastError();
		DEBUGMSG(1, (L"AddDllNameToRegistry-RegSetValueEx: HKLM/System/Kernel/InjectDLL failed %i\n", dwError));
		//TCHAR szError[MAX_PATH] = {0};
		//wsprintf(szError,
		//	L"RegSetValueEx() failed with %d [0x%08X]",
		//	dwError,
		//	dwError);
		//MessageBox(NULL, szError, NULL, MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
		LocalFree(bData);
		RegCloseKey(hKeyReg);
		return (DWORD)lRet;
	}

	LocalFree(bData);
	RegCloseKey(hKeyReg);

	DEBUGMSG(1, (L"AddDllNameToRegistry exit with ERROR_SUCCESS\n"));
	return ERROR_SUCCESS;
}

DWORD RemoveDllNameFromRegistry(TCHAR *szDllName){
	TCHAR* dllNames[MAX_PATH];
	DWORD dwDisposition = 0;
	HKEY  hKeyReg = NULL;
	LONG  lRet = ERROR_SUCCESS;
	DWORD cbBytes=0;
	DWORD dwType=0;

	//Does the key exist?
	DEBUGMSG(1, (L"RemoveDllNameFromRegistry-RegOpenKeyEx HKLM/System/Kernel/InjectDLL...\n"));
	if ((lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\Kernel", 0, KEY_ALL_ACCESS, &hKeyReg))!=ERROR_SUCCESS){
		return 0;
	}

	//lRet=RegDeleteValue
	//is there a InjectDLL key?
	dwType=REG_MUI_SZ;
	//get the needed size
	if ((lRet = RegQueryValueEx(hKeyReg,
		                      L"InjectDLL",
							  NULL,
							  &dwType,
							  NULL,
							  &cbBytes)) != ERROR_SUCCESS) //there is no InjectDLL key
	{
		DWORD dwError = GetLastError();
		DEBUGMSG(1, (L"RemoveDllNameFromRegistry-RegQueryValueEx HKLM/System/Kernel/InjectDLL failed %i\n", dwError));
		//get the data
		if(dwError==ERROR_MORE_DATA){
			DEBUGMSG(1, (L"RemoveDllNameFromRegistry-RegQueryValueEx HKLM/System/Kernel/InjectDLL ERROR_MORE_DATA\n"));
			TCHAR* bData;
			bData=(TCHAR*) malloc(cbBytes);
			lRet=RegQueryValueEx(hKeyReg,
										  L"InjectDLL",
										  NULL,
										  &dwType,
										  (BYTE*)bData,
										  &cbBytes);
			//split the data at \0, last is \0\0
			TCHAR* pTchar=bData; int offset=0;
			TCHAR szTemp[MAX_PATH]; TCHAR* pTemp=&szTemp[0];
			while(pTchar!='\0'){
				pTchar++;
			}
			LocalFree(bData);

			;
		}

		DEBUGMSG(1, (L"RemoveDllNameFromRegistry-exit with %i\n", lRet));
		return (DWORD)lRet;
	}
	else{
		DEBUGMSG(1, (L"RemoveDllNameFromRegistry-RegDeleteValue ...\n"));
		lRet=RegDeleteValue(hKeyReg,  L"InjectDLL");
		if(lRet==ERROR_SUCCESS)
			DEBUGMSG(1, (L"InjectDLL deleted\n"));
		else
			DEBUGMSG(1, (L"InjectDLL deletion failed\n"));
	}// RegQueryValueEx
	RegCloseKey(hKeyReg);
	DEBUGMSG(1, (L"RemoveDllNameFromRegistry-exit with ERROR_SUCCESS\n"));
	return ERROR_SUCCESS;

}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_STARTRDMINJECT));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STARTRDMINJECT));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable

    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the device specific controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_STARTRDMINJECT, szWindowClass, MAX_LOADSTRING);

    //If it is already running, then focus on the window, and exit
    hWnd = FindWindow(szWindowClass, szTitle);	
    if (hWnd) 
    {
        // set focus to foremost child window
        // The "| 0x00000001" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    } 

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

    hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    // When the main window is created using CW_USEDEFAULT the height of the menubar (if one
    // is created is not taken into account). So we resize the window after creating it
    // if a menubar is present
    if (g_hWndMenuBar)
    {
        RECT rc;
        RECT rcMenuBar;

        GetWindowRect(hWnd, &rc);
        GetWindowRect(g_hWndMenuBar, &rcMenuBar);
        rc.bottom -= (rcMenuBar.bottom - rcMenuBar.top);
		
        MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    static SHACTIVATEINFO s_sai;
	
    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
				case IDM_ACTIVATE:
					AddDllNameToRegistry(INJECT_DLL_NAME);
					break;
				case IDM_DEACTIVATE:
					RemoveDllNameFromRegistry(INJECT_DLL_NAME);
					break;
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
                case IDM_OK:
                    SendMessage (hWnd, WM_CLOSE, 0, 0);				
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
            SHMENUBARINFO mbi;

            memset(&mbi, 0, sizeof(SHMENUBARINFO));
            mbi.cbSize     = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hWnd;
            mbi.nToolBarId = IDR_MENU;
            mbi.hInstRes   = g_hInst;

            if (!SHCreateMenuBar(&mbi)) 
            {
                g_hWndMenuBar = NULL;
            }
            else
            {
                g_hWndMenuBar = mbi.hwndMB;
            }

            // Initialize the shell activate info structure
            memset(&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);

			//prepare injection
			AddDllNameToRegistry(INJECT_DLL_NAME);
			//possibly kill and start a new instance
			Sleep(3000);
			startRDM();

            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: Add any drawing code here...
            
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            CommandBar_Destroy(g_hWndMenuBar);
            PostQuitMessage(0);
			
			//RemoveDllNameFromRegistry(INJECT_DLL_NAME);

            break;

        case WM_ACTIVATE:
            // Notify shell of our activate message
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;
        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            {
                // Create a Done button and size it.  
                SHINITDLGINFO shidi;
                shidi.dwMask = SHIDIM_FLAGS;
                shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
                shidi.hDlg = hDlg;
                SHInitDialog(&shidi);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}
