//childwins.h

#ifndef _CHILDWINS_H_
#define _CHILDWINS_H_
#include <windows.h>

//HWND findWindowByTitle(HWND hWndStart, TCHAR* szTitle)
//HWND findWindowByClass(HWND hWndStart, TCHAR* szClass)
HWND FindChildWindowByParent(HWND hWndTopLevel, TCHAR* szChildClassName);
HWND getChildWindowByTitle(TCHAR* , TCHAR* );
HWND getChildWindowByClass(TCHAR* , TCHAR* );
HWND getWindowMenu(HWND );

#endif //_CHILDWINS_H_