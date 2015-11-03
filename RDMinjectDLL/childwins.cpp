//childwins.cpp
#pragma once

//#include "childwins.h"
//#include <windows.h>
#include "stdafx.h"

BOOL bFoundWindow=FALSE;
HWND hFoundHWND=NULL;
BOOL bFoundClass=FALSE;
HWND hFoundHWNDClass=NULL;

HWND FindChildWindowByParent(HWND hWndTopLevel, TCHAR* szChildClassName)
{
	//see http://blogs.msdn.com/b/raffael/archive/2009/01/08/disable-webbrowser-s-context-menu-in-netcf-applications.aspx
	bool bFound = false;
    HWND hwndCur = NULL;
    HWND hwndCopyOfCur = NULL;
    TCHAR szWindowClass[MAX_PATH];

	DEBUGMSG(1, (L"FindChildWindowByParent...Main is 0x%08x, looking for WinClass='%s'\n", hWndTopLevel, szChildClassName));
    do
    {
		DEBUGMSG(1, (L"FindChildWindowByParent...hwndCur is 0x%08x\n", hwndCur));
        // Is the current child null?
        if (NULL == hwndCur)
        {
            // get the first child
            hwndCur = GetWindow(hWndTopLevel, GW_CHILD);
			DEBUGMSG(1, (L"FindChildWindowByParent...GetFirstChild->hwndCur is 0x%08x\n", hwndCur));
        }
        else
        {
            hwndCopyOfCur = hwndCur;
            // at this point hwndcur may be a parent of other windows
            hwndCur = GetWindow(hwndCur, GW_CHILD);
			DEBUGMSG(1, (L"FindChildWindowByParent...GetNextChild->hwndCur is 0x%08x\n", hwndCur));

            // in case it's not a parent, does it have "brothers"?
			if (NULL == hwndCur){
				hwndCur = GetWindow(hwndCopyOfCur, GW_HWNDNEXT);
				DEBUGMSG(1, (L"FindChildWindowByParent...GW_HWNDNEXT->hwndCur is 0x%08x\n", hwndCur));
			}
        }

        //if we found a window (child or "brother"), let's see if it's the one we were looking for
        if (NULL != hwndCur)
        {
			DEBUGMSG(1, (L"FindChildWindowByParent...GetClassName->hwndCur is 0x%08x\n", hwndCur));
			//get class name of window
            GetClassName(hwndCur, szWindowClass, MAX_PATH-2);
			//compare with what we look for
			if(wcsicmp(szWindowClass, szChildClassName)==0){
				DEBUGMSG(1, (L"FindChildWindowByParent...GetClassName match found.\n"));
				bFound=TRUE;
			}
        }
		else{
			DEBUGMSG(1, (L"FindChildWindowByParent...Nothing found break.\n"));
            break;
		}
    }
    while (!bFound);

	DEBUGMSG(1, (L"FindChildWindowByParent...EXIT: ->hwndCur is 0x%08x, bFound=%i\n", hwndCur, bFound));

    //found!
    return hwndCur;
}

HWND findWindowByTitle(HWND hWndStart, TCHAR* szTitle){

	HWND hWnd = NULL;
	HWND hWnd1 = NULL;
	
	hWnd = hWndStart;
	TCHAR cszWindowString [MAX_PATH]; // = new TCHAR(MAX_PATH);
	TCHAR cszClassString [MAX_PATH]; //= new TCHAR(MAX_PATH);

	while (hWnd!=NULL && !bFoundWindow){
		GetClassName(hWnd, cszClassString, MAX_PATH);
		GetWindowText(hWnd, cszWindowString, MAX_PATH);
		DEBUGMSG(1, (L"findWindowByTitle: \"%s\"  \"%s\"\n", cszClassString, cszWindowString));

		if(wcscmp(cszWindowString, szTitle)==0){
			bFoundWindow=TRUE;
			hFoundHWND=hWnd;
			return hFoundHWND;
		}

		// Do Next Window
		hWnd1 = GetWindow(hWnd, GW_CHILD);
		if( hWnd1 != NULL ){ 
			findWindowByTitle(hWnd1, szTitle);
		}
		hWnd=GetWindow(hWnd,GW_HWNDNEXT);		// Get Next Window
	}
	return hFoundHWND;
}

HWND findWindowByClass(HWND hWndStart, TCHAR* szClass){

	HWND hWnd = NULL;
	HWND hWnd1 = NULL;

	hWnd = hWndStart;
	TCHAR cszWindowString [MAX_PATH]; // = new TCHAR(MAX_PATH);
	TCHAR cszClassString [MAX_PATH]; //= new TCHAR(MAX_PATH);

	while (hWnd!=NULL && bFoundClass==FALSE){
		GetClassName(hWnd, cszClassString, MAX_PATH);
		GetWindowText(hWnd, cszWindowString, MAX_PATH);
		DEBUGMSG(1, (L"findWindowByClass: \"%s\"  \"%s\"\n", cszClassString, cszWindowString));

		if(wcscmp(cszWindowString, szClass)==0){
			bFoundClass=TRUE;
			hFoundHWNDClass=hWnd;
			return hFoundHWND;
		}

		// Do Next Window
		hWnd1 = GetWindow(hWnd, GW_CHILD);
		if( hWnd1 != NULL ){ 
			findWindowByClass(hWnd1, szClass);
		}
		hWnd=GetWindow(hWnd,GW_HWNDNEXT);		// Get Next Window
	}
	return hFoundHWND;
}

HWND getChildWindowByTitle(TCHAR* szMainWinClass, TCHAR* szChildWinTitle){	//for use with PPC2003 Terminal Service Client
/*
	window text (window class)
	"192.168.128.5 - Terminal Services Client" (UIMainClass)	// top level window: FindWindow
		+	"<No name>"	UIContainerClass						// child window to UIMainClass
			+	"Input Capture Window" (IHWindowClass)			// child window to TerminalServerClient
*/
	//find the "Input Capture Window" child Window
	//Find the top levvel window
	//HWND hMainWnd = FindWindow(_T("TSSHELLWND"), NULL);
	HWND hMainWnd = FindWindow(szMainWinClass, NULL);

	//init find vars moved from being static inside findWindow() and findWindowByClass(), v15.01.2013
	bFoundWindow=FALSE;
	bFoundClass=FALSE;
	hFoundHWND=NULL;
	hFoundHWNDClass=NULL;
	//HWND hChildWnd = findWindow(hMainWnd, L"Input Capture Window");
	//HWND hChildWnd = findWindow(hMainWnd, L"WinShell");
	HWND hChildWnd = findWindowByTitle(hMainWnd, szChildWinTitle);
	if(hChildWnd)
		DEBUGMSG(1, (L"getTSChandle(): Found Child Window, Handle=0x%0x\n", hChildWnd));
	else
		DEBUGMSG(1, (L"getTSChandle(): No Child Windowfound\n"));
	return hChildWnd;
}

HWND getChildWindowByClass(TCHAR* szMainWinClass, TCHAR* szChildWinClass){	//for use with PPC2003 Terminal Service Client
/*
	window text (window class)
	"192.168.128.5 - Terminal Services Client" (UIMainClass)	// top level window: FindWindow
		+	"<No name>"	UIContainerClass						// child window to UIMainClass
			+	"Input Capture Window" (IHWindowClass)			// child window to TerminalServerClient
*/
	//find the "Input Capture Window" child Window
	//Find the top levvel window
	//HWND hMainWnd = FindWindow(_T("TSSHELLWND"), NULL);
	HWND hMainWnd = FindWindow(szMainWinClass, NULL);

	//init find vars moved from being static inside findWindow() and findWindowByClass(), v15.01.2013
	bFoundWindow=FALSE;
	bFoundClass=FALSE;
	hFoundHWND=NULL;
	hFoundHWNDClass=NULL;
	//HWND hChildWnd = findWindow(hMainWnd, L"Input Capture Window");
	//HWND hChildWnd = findWindow(hMainWnd, L"WinShell");
	HWND hChildWnd = findWindowByClass(hMainWnd, szChildWinClass);
	if(hChildWnd)
		DEBUGMSG(1, (L"getTSChandle(): Child Window, Handle=0x%0x\n", hChildWnd));
	else
		DEBUGMSG(1, (L"getTSChandle(): No Child Windowfound\n"));
	return hChildWnd;
}

static int iLevel=0;
HWND hWndMenuWorker=NULL;
BOOL CALLBACK findMenuWorker(HWND hwnd, LPARAM lParam) //find window for PID
{
//	LONG_PTR iLevel = lParam;	//change
	iLevel++;
	TCHAR caption[MAX_PATH];
	TCHAR classname[MAX_PATH];
	DWORD dwTargetProcID=(DWORD) lParam;
	DEBUGMSG(1,(L"Looking for dwTargetProcID=0x%0x\n", dwTargetProcID));

	//TCHAR procname[MAX_PATH];
	//TCHAR* szName; szName = (TCHAR*)malloc (MAX_PATH);
	
	//test if win is of process with ID in query
	DWORD dwProcID=0;
	GetWindowThreadProcessId(hwnd, &dwProcID);
	if(dwProcID!=dwTargetProcID){
		return TRUE; //next loop
	}

	DEBUGMSG(1, (L"Found window with procID\n"));
	GetClassName(hwnd, classname, MAX_PATH);
	//is this a menu_worker?
	if(wcscmp(classname,L"menu_worker")==0){
		DEBUGMSG(1, (L"Found menu_worker window for procID\n"));
		hWndMenuWorker=hwnd;
		return FALSE;//found the window so return
	}
	else
		DEBUGMSG(1, (L"class name: %s\n", classname));

	////dimensions and position
	//RECT rect;
	//GetWindowRect(hwnd, &rect);
	//TCHAR szRect[64];
	//wsprintf(szRect, L"%i;%i/%i;%i (%ix%i)", rect.left, rect.top, rect.right, rect.bottom, rect.right-rect.left, rect.bottom-rect.top);

	//////visible?, MINIMIZED not supported
	//DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	//TCHAR szStyle[64];
	//wsprintf(szStyle, L"[%s]", dwStyle&WS_VISIBLE ? L"visible":L"hidden" );

	////wsprintf(procname, L"%s", getProcessName(dwProcID, szName));
	////DEBUGMSG(1, (L"%i\t0x%08x\t0x%08x\t('%s')\t'%s'\t'%s'\t%s\t%s\n", iLevel, hwnd, dwProcID, procname, classname, caption, szRect, szStyle));
	////nclog(L"%i\t0x%08x\t0x%08x\t('%s')\t'%s'\t'%s'\t%s\t%s\n", iLevel, hwnd, dwProcID, procname, classname, caption, szRect, szStyle);

	//free(szName);

	//return FALSE; // to stop iteration before end of window list
	return true;
}

HWND getWindowMenu(HWND hwndTarget){
	DWORD dwProcID=0;
	hWndMenuWorker=NULL;
	//get procID of Window
	GetWindowThreadProcessId(hwndTarget, &dwProcID);
	DEBUGMSG(1,(L"Looking for procID=0x%0x\n", dwProcID));

	//iterate thru windows and find windows with class "menu_worker" for this process
	EnumWindows(findMenuWorker, (LPARAM) dwProcID);
	if(hWndMenuWorker!=NULL)
		DEBUGMSG(1,(L"found menu_worker\n"));
	else
		DEBUGMSG(1,(L"menu_worker NOT found\n"));
	return hWndMenuWorker;
}