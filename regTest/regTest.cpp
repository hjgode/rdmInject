// regTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../RDMinjectDLL/registry.h"

#define regMainKey L"\\Software\\Honeywell\\RDMinjectDLL"
#define defaultModuleFilename L"\\Windows\\wpctsc.exe"

TCHAR	MODULE_FILENAME[MAX_PATH]={defaultModuleFilename}; //which modul to hook

TCHAR	waitForClass[MAX_PATH] = {L"TSSHELLWND"};				//which win class to look for in first stage
TCHAR	waitForTitle[MAX_PATH] = {NULL};							//which win title to look for in first stage
//second window?, if both NULL we do not wait for a second window
TCHAR	waitForClassSecond[MAX_PATH] = {NULL}; //L"Dock";			//which win class to look for in second stage
TCHAR	waitForTitleSecond[MAX_PATH] = {NULL};						//which win class to look for in second stage
BOOL	bSecondWndIsChild = FALSE;						//is second window to be treated as a child of first window?

TCHAR	slaveExe[MAX_PATH] = {L"\\Windows\\IBaddBtnCAM.exe"};		//which exe should be started after first and second stage is confirmed
TCHAR	slaveArgs[MAX_PATH]={L""};									//arguments to be passed to exe

BOOL	bEnableSubClassWindow=FALSE;					//is sublcassing needed
DWORD	dwWINI_CHANGEDLPARM=132960904;
DWORD	dwWINI_CHANGEDWPARM=224;
DWORD	bSupressAllWinIniChange=1;

TCHAR* regStringVals[] = {	L"MODULE_FILENAME",	
							L"waitForClass",	L"waitForTitle",	
							L"waitForClassSecond",	L"waitForTitleSecond",	
							L"slaveExe",	L"slaveArgs", L"WINI_CHANGEDLPARM", NULL};
TCHAR* regStringDefaults[] = {L"\\windows\\wpctsc.exe", 
							L"TSSHELLWND", NULL, 
							NULL, NULL,
							L"", L"", L"132960904"};
void writeReg(){
	int iRes=0;
	wsprintf(MODULE_FILENAME, L"%s", defaultModuleFilename);
	if(OpenKey(regMainKey)!=0)
		iRes=OpenCreateKey(regMainKey);
	if(iRes!=0){
		DEBUGMSG(1, (L"could not open main reg key '%s'\n", regMainKey));
		return;
	}
	int i=0;
	do{
		if(regStringDefaults[i]!=NULL){
			if(RegWriteStr(regStringVals[i], regStringDefaults[i])!=0)
				DEBUGMSG(1, (L"regwritestr failed for '%s': '%s'\n", regStringVals[i], regStringDefaults[i]));
			else
				DEBUGMSG(1, (L"regwritestr OK for '%s': '%s'\n", regStringVals[i], regStringDefaults[i]));
		}
		else
		{
			if(RegWriteStr(regStringVals[i], L"")!=0)
				DEBUGMSG(1, (L"regwritestr failed for '%s': '%s'\n", regStringVals[i], regStringDefaults[i]));
			else
				DEBUGMSG(1, (L"regwritestr OK for '%s': '%s'\n", regStringVals[i], regStringDefaults[i]));
		}
		i++;
	}while(regStringVals[i]!=NULL);

	if(RegWriteDword(L"bSecondWndIsChild", (DWORD*)&bSecondWndIsChild)==0)
		DEBUGMSG(1, (L"RegWriteDword OK for 'bSecondWndIsChild'\n"));
	else
		DEBUGMSG(1, (L"RegWriteDword FAILED for 'bSecondWndIsChild'\n"));

	if(RegWriteDword(L"bEnableSubClassWindow", (DWORD*)&bEnableSubClassWindow)==0)
		DEBUGMSG(1, (L"RegWriteDword OK for 'bEnableSubClassWindow'\n"));
	else
		DEBUGMSG(1, (L"RegWriteDword FAILED for 'bEnableSubClassWindow'\n"));

	if(RegWriteDword(L"bSupressAllWinIniChange", &bSupressAllWinIniChange)==0)
		DEBUGMSG(1, (L"RegWriteDword OK for 'bSupressAllWinIniChange'\n"));
	else
		DEBUGMSG(1, (L"RegWriteDword FAILED for 'bSupressAllWinIniChange'\n"));
	CloseKey();
}

void readReg(){
	int iRes=0;
	iRes=OpenKey(regMainKey);
	if(iRes!=0){
		DEBUGMSG(1, (L"could not open main reg key '%s'\n", regMainKey));
		writeReg();
		return;
	}
	TCHAR szTemp[MAX_PATH];
	DWORD dwTemp=-1;
	int i=0;
	wsprintf(MODULE_FILENAME, L"%s", defaultModuleFilename);
	do{
		DWORD dwTemp=0;
		if(RegReadStr(regStringVals[i], szTemp)!=0)
			DEBUGMSG(1, (L"RegReadStr failed for '%s'\n", regStringVals[i]));
		else{
			DEBUGMSG(1, (L"RegReadStr OK for '%s': '%s'\n", regStringVals[i], szTemp));
			if(wcscmp(regStringVals[i], L"MODULE_FILENAME")==0){
				wsprintf(MODULE_FILENAME, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"waitForClass")==0){
				wsprintf(waitForClass, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"waitForTitle")==0){
				wsprintf(waitForTitle, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"waitForClassSecond")==0){
				wsprintf(waitForClassSecond, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"waitForTitleSecond")==0){
				wsprintf(waitForTitleSecond, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"slaveExe")==0){
				wsprintf(slaveExe, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"slaveArgs")==0){
				wsprintf(slaveArgs, L"%s",szTemp);
			}
			else if(wcscmp(regStringVals[i], L"WINI_CHANGEDLPARM")==0){
				dwTemp = _wtol(szTemp);
				if(dwTemp!=0)
					dwWINI_CHANGEDLPARM=dwTemp;
			}
			else if(wcscmp(regStringVals[i], L"WINI_CHANGEDWPARM")==0){
				dwTemp = _wtol(szTemp);
				if(dwTemp!=0)
					dwWINI_CHANGEDWPARM=dwTemp;
			}
			else{
				DEBUGMSG(1, (L"found unknown reg key"));
			}
		}
		i++;
	}while(regStringVals[i]!=NULL);
	
	if(RegReadDword(L"bSecondWndIsChild", (DWORD*)&dwTemp)==0){
		bSecondWndIsChild=(BOOL)dwTemp;
		DEBUGMSG(1, (L"RegReadDword OK for 'bSecondWndIsChild':%i\n", bSecondWndIsChild));
	}
	else
		DEBUGMSG(1, (L"RegReadDword FAILED for 'bSecondWndIsChild'\n"));

	if(RegReadDword(L"bEnableSubClassWindow", (DWORD*)&dwTemp)==0){
		bEnableSubClassWindow=(BOOL)dwTemp;
		DEBUGMSG(1, (L"RegReadDword OK for 'bEnableSubClassWindow':%i\n", bEnableSubClassWindow));
	}
	else
		DEBUGMSG(1, (L"RegReadDword FAILED for 'bEnableSubClassWindow'\n"));

	if(RegReadDword(L"bSupressAllWinIniChange", &dwTemp)==0){
		bSupressAllWinIniChange=dwTemp;
		DEBUGMSG(1, (L"RegReadDword OK for 'bSupressAllWinIniChange':%i\n", bSupressAllWinIniChange));
	}
	else
		DEBUGMSG(1, (L"RegReadDword FAILED for 'bSupressAllWinIniChange'\n"));

	CloseKey();
}

int _tmain(int argc, _TCHAR* argv[])
{
	readReg();
	writeReg();
	return 0;
}

