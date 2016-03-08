// moveSIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int getMenuHeight(){
	//system function to get menu height
	int iMnuHeight = GetSystemMetrics(SM_CYMENU);  //26 is incorrect!
	DEBUGMSG(1, (L"Menu Height is %i\n", iMnuHeight));
	//get menu height
	HWND hwndMenu=FindWindow(L"menu_worker",NULL);
	if(hwndMenu!=INVALID_HANDLE_VALUE){
		RECT rectMnu;
		GetWindowRect(hwndMenu, &rectMnu);
		iMnuHeight=rectMnu.bottom-rectMnu.top;
		DEBUGMSG(1, (L"Menu Height menu_worker is %i\n", iMnuHeight));
	}
	return iMnuHeight;
}

int moveSIP(bool bRestore){
	int iRet=0;
	HWND hwndSIP = NULL;
	RECT rectSIP;
	//find the SIP win class: SipWndClass
	hwndSIP = FindWindow(L"SipWndClass", NULL);
	if(hwndSIP!=NULL){
		DEBUGMSG(1, (L"Found SipWndClass\n"));
		
		//hard restore: 0/212, 800/412
		//MoveWindow(hwndSIP, 0, 212, 800, 200, TRUE);

		//is it shown?
		if(GetWindowRect(hwndSIP, &rectSIP)){

			DEBUGMSG(1, (L"SipWndClass rect = %i/%i, %i/%i\n", rectSIP.left, rectSIP.top, rectSIP.right, rectSIP.bottom));
			int iMH = getMenuHeight();
			int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
			int iSIPHeight=rectSIP.bottom-rectSIP.top;

			DEBUGMSG(1, (L"screen height=%i, SIP height=%i, menu height=%i\n", iScreenHeight, iSIPHeight, iMH));
			DEBUGMSG(1, (L"SipWndClass rect = %i/%i, %i/%i\n", rectSIP.left, rectSIP.top, rectSIP.right, rectSIP.bottom));

			//212 
			//SIP height = 412 - 212 = 200
			//480 - 212 = 268 => SIP height + MENU height
			//480 - 200 - 68 <=> screenH - SIPH - menuH
			int iNormalYpos=iScreenHeight-iSIPHeight-iMH;
			DEBUGMSG(1, (L"y normal pos=%i\n", iNormalYpos));
			if(!bRestore){
				//possibly move it down, verify portait
				if(rectSIP.left==0 && rectSIP.top==iNormalYpos/* && rectSIP.right==800 */){
					DEBUGMSG(1, (L"Moving SipWndClass\n"));
					//SetWindowPos(hwndSIP, NULL, rectSIP.left, 212 + iMH, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER );
					//move SIP down for menuHeight
					MoveWindow(hwndSIP, rectSIP.left, iNormalYpos+iMH, rectSIP.right, iSIPHeight, TRUE);
					iRet=1;
				}
				else{
					DEBUGMSG(1, (L"SipWndClass already moved or down\n"));
					iRet=0;
				}
			}else{ //restore org position, 0/212, 800/412
				DEBUGMSG(1, (L"RESTORE: Moving SipWndClass\n"));
				// y-position = screen height - menuHeight - SIP height
				MoveWindow(hwndSIP, rectSIP.left, iNormalYpos, rectSIP.right, iSIPHeight, TRUE);
				iRet=1;
			}
		}
		else{
			DEBUGMSG(1, (L"GetWindowRect SipWndClass FAILED\n"));
			iRet=-2;
		}
	}
	else{
		DEBUGMSG(1, (L"SipWndClass NOT FOUND\n"));
		iRet=-3;
	}
	return iRet;
}
int moveSIP(){
	return moveSIP(FALSE);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc==2){
		DEBUGMSG(1, (L"argv[1]=%s\n", argv[1]));
		if(wcsicmp(argv[1], L"movedown")==0)
			moveSIP();
	}
	else {
		DEBUGMSG(1, (L"restoring SIP pos\n"));
		moveSIP(TRUE);
	}
	return 0;
}

