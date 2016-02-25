// RDMinjectDLL.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "childwins.h"

#define RDM_MSG_QUEUE L"RDM_WM_MSGQUEUE" //RDM msg queue for WM_ messages
HANDLE h_RDM_msgQueue;
#define MODULE_FILENAME L"\\Windows\\wpctsc.exe" //which modul to hook

typedef struct {
	HWND hWnd;
	UINT msg;
	WPARAM wParm;
	LPARAM lParm;
}WM_EVT_DATA;

static HANDLE  hExitEvent = NULL;
static HANDLE  hThread = NULL;
static HWND    hWndRDM = NULL;
static WNDPROC RDMWndFunc = 0;

const TCHAR szTextFileName[] = L"\\rdmInjectDll.txt";

// In order to work, this DLL needs at least one export, 
// even a dummy one like the one below:
extern "C" __declspec(dllexport) int DummyFunction()
{
	return (int)ERROR_SUCCESS;
}

//####################################################################
#include "tlhelp32.h"
#pragma comment(lib, "toolhelp.lib")
#ifndef TH32CS_SNAPNOHEAPS
	#define TH32CS_SNAPNOHEAPS 0x40000000
#endif

/*$DOCBEGIN$
 =======================================================================================================================
 *    £
 *    runProcess(TCHAR* szFullName, TCHAR* args); £
 *    * Description: start a process and wait until it ends or signaled to stop waiting. £
 *    * Arguments: szFullname of process to start. £
 *    for example, \Windows\app.exe. £
 *    * Arguments: args for the process. £
 *    for example, --help. £
 *    * Returns: 0 - process has been run. £
 *    FALSE - process is not running. £
 *    $DOCEND$ £
 *
 =======================================================================================================================
 */
int runProcess(TCHAR* szFullName, TCHAR* args){
	STARTUPINFO startInfo;
	memset(&startInfo, 0, sizeof(STARTUPINFO));
	PROCESS_INFORMATION processInformation;
	DWORD exitCode=0;

	if(CreateProcess(szFullName, args, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &processInformation)!=0){
		// Successfully created the process.  Wait for it to finish.
		DEBUGMSG(1, (L"Process '%s' started.\n", szFullName));

		// Close the handles.
		CloseHandle( processInformation.hProcess );
		CloseHandle( processInformation.hThread );
		return 0;
 	}
	else{
		//error
		DWORD dwErr=GetLastError();
		DEBUGMSG(1, (L"CreateProcess for '%s' failed with error code=%i\n", szFullName, dwErr));
		return -1;
	}
}

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
//####################################################################################


BOOL WriteRecordToTextFile(LPCTSTR szRecord)
{
	BOOL   bRet = FALSE;
#if DEBUG
	HANDLE hTextFile = INVALID_HANDLE_VALUE;
	TCHAR  szBuffer[MAX_PATH] = {0};

	hTextFile = CreateFile(
		szTextFileName, 
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE, 
		NULL, 
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (hTextFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (SetFilePointer(hTextFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
	{
		CloseHandle(hTextFile);
		return FALSE;
	}

	SYSTEMTIME   st;
	GetLocalTime(&st);
	wsprintf(szBuffer, 
		L"%02d:%02d:%02d : %s\r\n",
		st.wHour, 
		st.wMinute,
		st.wSecond,
		szRecord);

	DWORD dwBytesToWrite = wcslen(szBuffer);
	char* chBuffer = (char*)LocalAlloc(LPTR, dwBytesToWrite);
	wcstombs(chBuffer, szBuffer, dwBytesToWrite);

	DWORD dwBytesWritten;
	bRet = WriteFile(
		hTextFile, 
		chBuffer, 
		dwBytesToWrite, 
		&dwBytesWritten, 
		NULL);
	CloseHandle(hTextFile);
	LocalFree(chBuffer);

#endif
	return bRet;
}


LRESULT CALLBACK SubclassWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR szRecord[MAX_PATH];
	wsprintf(szRecord, L"WM_: msg=0x%08x, wP=%i, lP=%i", message, wParam, lParam);
	WriteRecordToTextFile(szRecord);
	
	WM_EVT_DATA myData;
	myData.hWnd=window;
	myData.msg=message;
	myData.wParm=wParam;
	myData.lParm=lParam;
	if(WriteMsgQueue(h_RDM_msgQueue, &myData, sizeof(WM_EVT_DATA), 0, 0))
		DEBUGMSG(1, (L"subclass RDM: WriteMsgQueue OK\n"));
	else
		DEBUGMSG(1, (L"subclass RDM: WriteMsgQueue failed: %i\n", GetLastError()));

	DEBUGMSG(1, (L"SubclassWndProc: hwnd=0x%08x, msg=0x%08x, wP=%i, lP=%i\n", window, message, wParam, lParam));

	switch (message)
	{
		case WM_WININICHANGE: //sent when SIP is shown, do not forward?
			//if(lParam!=NULL){
			if(lParam==133071496){
//				DEBUGMSG(1, (L"Got WM_WININICHANGE with '%s'\n", (TCHAR)lParam)); //SubclassWndProc: hwnd=0x7c080a60, msg=0x0000001a, wP=224, lP=133071496
				return 0; //lie about message handled
			}
			break;
		case WM_SIZE:
			DEBUGMSG(1, (L"Got WM_SIZE with wP=0x%08x, lP=0x%08x\n", wParam, lParam));
			break;

		case WM_CLOSE:
			//restore old WndProc address
			SetWindowLong(hWndRDM, GWL_WNDPROC, (LONG)RDMWndFunc);
			break;

		default:
			break;
	}

	return CallWindowProc(RDMWndFunc, window, message, wParam, lParam);
}

DWORD WaitForProcessToBeUpAndReadyThread(PVOID)
{
	WriteRecordToTextFile(L"starting watch thread, set exit event");
	hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	DWORD dwError =0;
	TCHAR szError[MAX_PATH] = {0};

	if (hExitEvent == NULL)
	{
#if DEBUG
		dwError = GetLastError();
		wsprintf(szError, 
			L"CreateEvent() failed with %d [0x%08X]",
			dwError,
			dwError);
		WriteRecordToTextFile(szError);
		return dwError;
#else
		return GetLastError();
#endif
	}

	DWORD dwRet = WAIT_OBJECT_0;
	while ((hWndRDM = FindWindow(L"TSSHELLWND", NULL)) == NULL)
	{
		dwRet = WaitForSingleObject(hExitEvent, 1000);

		if (dwRet == WAIT_TIMEOUT)
		{
#if DEBUG
			wsprintf(szError, L"WaitForSingleObject() timed out.");
			WriteRecordToTextFile(szError);
#endif
			continue;
		}

		if (dwRet == WAIT_OBJECT_0 + 0) // hExitEvent signaled!
		{
#if DEBUG
			wsprintf(szError, L"'Exit' event has been signaled.");
			WriteRecordToTextFile(szError);
#endif
			break;
		}

		if (dwRet == WAIT_FAILED)
		{
			break;
		}
	}//FindWindow(L"TSSHELWND", NULL)

	if (hWndRDM == NULL){
		return dwRet; //failed
	}

	//Find child window to subclass?
#if DEBUG
	wsprintf(szError, L"Looking for child window...");
	WriteRecordToTextFile(szError);
#endif
	HWND hChildWin=FindChildWindowByParent(hWndRDM, L"UIMainClass");
	do{
		hChildWin=FindChildWindowByParent(hWndRDM, L"UIMainClass");
		
		dwRet = WaitForSingleObject(hExitEvent, 1000);
		if (dwRet == WAIT_TIMEOUT)
		{
#if DEBUG
			wsprintf(szError, L"WaitForSingleObject() timed out.");
			WriteRecordToTextFile(szError);
#endif
			continue;
		}

		if (dwRet == WAIT_OBJECT_0 + 0) // hExitEvent signaled!
		{
#if DEBUG
			wsprintf(szError, L"'Exit' event has been signaled.");
			WriteRecordToTextFile(szError);
#endif
			break;
		}

		if (dwRet == WAIT_FAILED)
		{
			break;
		}
	}while(hChildWin==NULL);

	//FindChildWindowByParent(hWnd, L"TerminalServerClient");
	//FindChildWindowByParent(hWnd, L"WinShell");
	//hWnd=FindWindow(L"MS_SIPBUTTON",L"MS_SIPBUTTON");
#if DEBUG
	wsprintf(szError, L"child window has handle %i (0x%08x)", hChildWin, hChildWin);
	WriteRecordToTextFile(szError);
#endif
	///TODO check if hwndchild or hwndRDM???
	///TESTED: In both cases use hWndRDM, but we need to wait for UIMainClass being loaded before we can start here
	if(hChildWin!=NULL){
		DEBUGMSG(1, (L"### Using child window\n"));
		RDMWndFunc = (WNDPROC)SetWindowLong(hWndRDM, GWL_WNDPROC, (LONG)SubclassWndProc);
	}else{
		DEBUGMSG(1, (L"### Using main window\n"));
		RDMWndFunc = (WNDPROC)SetWindowLong(hWndRDM, GWL_WNDPROC, (LONG)SubclassWndProc);
	}

	if (RDMWndFunc == NULL)
	{
#if DEBUG
		dwError = GetLastError();
		wsprintf(szError, 
			L"SetWindowLong() failed with %d [0x%08X]",
			dwError,
			dwError);
		WriteRecordToTextFile(szError);
		return dwError;
#else
		return GetLastError();
#endif
	}
	CloseHandle(hExitEvent);
	hExitEvent = NULL;

	if(runProcess(L"\\Windows\\RdmAddonBatt2.exe", L"")==0)
		DEBUGMSG(1, (L"\\Windows\\RdmAddonBatt2.exe started\n"));
	else
		DEBUGMSG(1, (L"\\Windows\\RdmAddonBatt2.exe not started?"));

	if(runProcess(L"\\Windows\\RdmAddonWiFi.exe", L"")==0)
		DEBUGMSG(1, (L"\\Windows\\RdmAddonWiFi.exe started\n"));
	else
		DEBUGMSG(1, (L"\\Windows\\RdmAddonWiFi.exe not started?"));

	WriteRecordToTextFile(L"EXIT watch thread");
	return ERROR_SUCCESS;
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		// Disable the DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls
		DisableThreadLibraryCalls ((HMODULE)hModule);

		TCHAR szModuleFileName[MAX_PATH+1] = {0};
		TCHAR szRecord[MAX_PATH] = {0};
		if ((GetModuleFileName(NULL, szModuleFileName, MAX_PATH)) == NULL){
			wsprintf(szRecord, L"GetModuleFileName failed!");
			WriteRecordToTextFile(szRecord);
			return TRUE;
		}

		if (_wcsicmp(szModuleFileName, MODULE_FILENAME) != 0){
			wsprintf(szRecord, L"Compare ModuleFileName failed: %s", szModuleFileName);
			WriteRecordToTextFile(szRecord);
			return TRUE;
		}

		//we are attached to process wpctsc.exe
		MSGQUEUEOPTIONS msqOptions; 
		memset(&msqOptions, 0, sizeof(MSGQUEUEOPTIONS));

		msqOptions.dwSize=sizeof(MSGQUEUEOPTIONS);
		msqOptions.dwFlags=MSGQUEUE_NOPRECOMMIT | MSGQUEUE_ALLOW_BROKEN;
		msqOptions.cbMaxMessage=MAX_PATH; //size of message
		msqOptions.dwMaxMessages=10;		
		msqOptions.bReadAccess=FALSE; //need write access, no read
		

		if(h_RDM_msgQueue==NULL){
			h_RDM_msgQueue = CreateMsgQueue(RDM_MSG_QUEUE, &msqOptions);
			DWORD dwErr=GetLastError();
			wsprintf(szRecord, L"CreateMsgQueue failed: %i", dwErr);
			WriteRecordToTextFile(szRecord);
			DEBUGMSG(1, (L"CreateMsgQueue failed: %i\n", dwErr));
		}

#if DEBUG
		wsprintf(szRecord,
			L"{0x%08X} %s attached",
			GetCurrentProcessId(),
			szModuleFileName);
		WriteRecordToTextFile(szRecord);
#endif

		// Create a thread to wait for the "ScannerControlScanner" window,
		// which can be seen as the transimssion belt between the Scanner
		// Control Services and TekTerm, to be up and ready...
		if ((hThread = CreateThread(NULL, 0, WaitForProcessToBeUpAndReadyThread, 0, 0, NULL)) == NULL)
		{
#if DEBUG
			DWORD dwError = GetLastError();
			TCHAR szError[MAX_PATH] = {0};
			wsprintf(szError,
				L"CreateThread() failed with %d [0x%08X]\r\n", 
				dwError,
				dwError);
			WriteRecordToTextFile(szError);
#endif
		}
		else{
			//start additional helpers?
		}//CreateThread
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		TCHAR szModuleFileName[MAX_PATH+1] = {0};
		if ((GetModuleFileName(NULL, szModuleFileName, MAX_PATH)) == NULL)
			return TRUE;

		if (_wcsicmp(szModuleFileName, MODULE_FILENAME) != 0)
			return TRUE;

#if DEBUG
		TCHAR szRecord[MAX_PATH] = {0};
		wsprintf(szRecord,
			L"{0x%08X} %s detached",
			GetCurrentProcessId(),
			szModuleFileName);
		WriteRecordToTextFile(szRecord);
#endif
	
		if(h_RDM_msgQueue!=NULL){
			WriteRecordToTextFile(L"closing msgqueue");
			CloseMsgQueue(h_RDM_msgQueue);
			//CloseHandle(h_RDM_msgQueue);
		}

		if (hExitEvent != NULL)
		{
			WriteRecordToTextFile(L"set exit event");
			if (SetEvent(hExitEvent))
				WaitForSingleObject(hThread, 2000);
			CloseHandle(hExitEvent);
		}
		WriteRecordToTextFile(L"close thread handle");
		CloseHandle(hThread);

		WriteRecordToTextFile(L"kill RdmAddonBatt2.exe");
		KillExeWindow(L"RdmAddonBatt2.exe");
	}
    return TRUE;
}

