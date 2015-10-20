// RDMinjectDLL.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "childwins.h"

#define RDM_MSG_QUEUE L"RDM_WM_MSGQUEUE" //RDM msg queue for WM_ messages
HANDLE h_RDM_msgQueue;

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


#if DEBUG
BOOL WriteRecordToTextFile(LPCTSTR szRecord)
{
	BOOL   bRet = FALSE;
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

	return bRet;
}

#endif

LRESULT CALLBACK SubclassWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR szRecord[MAX_PATH];
	wsprintf(szRecord, L"WM_: msg=%i, wP=%i, lP=%i", message, wParam, lParam);
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

	DEBUGMSG(1, (L"SubclassWndProc: hwnd=0x%08x, msg=%i, wP=%i, lP=%i\n", window, message, wParam, lParam));

	switch (message)
	{
		// This message is sent when an application passes data to another 
		// application. 
//		case WM_COPYDATA:
//			{
//#if DEBUG
//				WriteRecordToTextFile(L"WM_COPYDATA");
//#endif
//
//				// COPYDATASTRUCT 
//				// http://msdn.microsoft.com/en-us/library/aa922015.aspx
//
//				// dwData specifies the barcode symbology
//				//        Sse the Symbology Type.h header files
//				//
//				// cbData specifies the size, in bytes, of the barcode 
//				//        data pointed to by the lpData member. 
//				//
//				// lpData points to the barcode data
//
//				COPYDATASTRUCT* data = (COPYDATASTRUCT*)lParam;
//
//				// Passed this point, feel free to manipulate the 
//				// barcode data as/if needed...
//
//				// In the below example, we wrap the barcode data with
//				// 'Double Chevron Left' and 'Double Chevron Right' characters
//				// if the barcode symbology is UPC-A
//
//				if (data->dwData == ScsSymbology_UPCA)
//				{
//					int size = (data->cbData + 3) * sizeof(WCHAR);
//					WCHAR* strData = (WCHAR*)LocalAlloc(LPTR, size);
//
//					wcscpy(strData, L"«"); // 'Double Chevron Left' character
//					memcpy(&strData[1], data->lpData, data->cbData);
//					wcscat(strData, L"»"); // 'Double Chevron Right' character
//
//#if DEBUG
//					WriteRecordToTextFile(strData);
//#endif
//
//					// Momentarely save lpData and cbData
//					PVOID oldPtr = data->lpData;
//					DWORD oldData = data->cbData;
//
//					// Copy back the (altered) barcode data to the 
//					// COPYDATASTRUCT structure
//					data->cbData = (wcslen(strData)) * sizeof(WCHAR);
//					data->lpData = (PVOID)(LPCTSTR)strData;
//
//					LRESULT lRet = CallWindowProc(RDMWndFunc, window, message, wParam, lParam);
//					LocalFree(strData);
//
//					// Restore lpData and cbData
//					data->lpData = oldPtr;
//					data->cbData = oldData;
//
//					return lRet;
//				}
//			}
//			break;

		case WM_CLOSE:
			SetWindowLong(hWndRDM, GWL_WNDPROC, (LONG)RDMWndFunc);
			break;

		default:
			break;
	}

	return CallWindowProc(RDMWndFunc, window, message, wParam, lParam);
}

DWORD WaitForProcessToBeUpAndReadyThread(PVOID)
{
	hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hExitEvent == NULL)
	{
#if DEBUG
		DWORD dwError = GetLastError();
		TCHAR szError[MAX_PATH] = {0};
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
			TCHAR szError[MAX_PATH] = {0};
			wsprintf(szError, 
				L"WaitForSingleObject() timed out.");
			WriteRecordToTextFile(szError);
#endif
			continue;
		}

		if (dwRet == WAIT_OBJECT_0 + 0) // hExitEvent signaled!
		{
#if DEBUG
			TCHAR szError[MAX_PATH] = {0};
			wsprintf(szError, 
				L"'Exit' event has been signaled.");
			WriteRecordToTextFile(szError);
#endif
			break;
		}

		if (dwRet == WAIT_FAILED)
		{
#if DEBUG
			DWORD dwError = GetLastError();
			TCHAR szError[MAX_PATH] = {0};
			wsprintf(szError, 
				L"WaitForSingleObject() failed with %d [0x%08X]",
				dwError,
				dwError);
			WriteRecordToTextFile(szError);
#endif
			break;
		}
	}//FindWindow(L"TSSHELWND", NULL)

	CloseHandle(hExitEvent);
	hExitEvent = NULL;

	if (hWndRDM == NULL)
		return dwRet; //failed

	//Find child window to subclass?
	hWndRDM=FindChildWindowByParent(hWndRDM, L"UIMainClass"); 
	//FindChildWindowByParent(hWnd, L"TerminalServerClient");
	//FindChildWindowByParent(hWnd, L"WinShell");
	//hWnd=FindWindow(L"MS_SIPBUTTON",L"MS_SIPBUTTON");

	RDMWndFunc = (WNDPROC)SetWindowLong(hWndRDM, GWL_WNDPROC, (LONG)SubclassWndProc);
	if (RDMWndFunc == NULL)
	{
#if DEBUG
		DWORD dwError = GetLastError();
		TCHAR szError[MAX_PATH] = {0};
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

		if (_wcsicmp(szModuleFileName, L"\\Windows\\wpctsc.exe") != 0){
			wsprintf(szRecord, L"Compare ModuleFileName failed: %s", szModuleFileName);
			WriteRecordToTextFile(szRecord);
			return TRUE;
		}
		MSGQUEUEOPTIONS msqOptions; memset(&msqOptions, 0, sizeof(MSGQUEUEOPTIONS));

		msqOptions.dwSize=sizeof(MSGQUEUEOPTIONS);
		msqOptions.dwFlags=MSGQUEUE_NOPRECOMMIT | MSGQUEUE_ALLOW_BROKEN;
		msqOptions.cbMaxMessage=0; //no limit
		msqOptions.bReadAccess=FALSE; //need write access, no read
		
		h_RDM_msgQueue = CreateMsgQueue(RDM_MSG_QUEUE, &msqOptions);

		if(h_RDM_msgQueue==NULL){
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
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		TCHAR szModuleFileName[MAX_PATH+1] = {0};
		if ((GetModuleFileName(NULL, szModuleFileName, MAX_PATH)) == NULL)
			return TRUE;

		if (_wcsicmp(szModuleFileName, L"\\Windows\\wpctsc.exe") != 0)
			return TRUE;

#if DEBUG
		TCHAR szRecord[MAX_PATH] = {0};
		wsprintf(szRecord,
			L"{0x%08X} %s detached",
			GetCurrentProcessId(),
			szModuleFileName);
		WriteRecordToTextFile(szRecord);
#endif
	
		if(h_RDM_msgQueue!=NULL)
			CloseHandle(h_RDM_msgQueue);

		if (hExitEvent != NULL)
		{
			if (SetEvent(hExitEvent))
				WaitForSingleObject(hThread, 2000);
			CloseHandle(hExitEvent);
		}
		CloseHandle(hThread);
	}
    return TRUE;
}

