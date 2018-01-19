# RDM injectDLL1

Tool that enables using the SIP within Remote Desktop Mobile full screen mode.

the RDM injectDLL watches the process list for wpctsc.exe (the Remote Desktop
Mobile process) and then waits for the main window and it's remote host child 
window. The remote host child window is subclassed and all WM_NOTITY messages
with a defined ID will not be forwarded to the child window. This way the DLL
supresses the immediate close of the SIP (Software Input Panel) by the main 
RDM window. 

## Installation

1. Copy the Installation cab file "RDMinjectDLL1.cab" onto the device
    # For the CK75 use file RDMinjectDLL1_ck75.cab instead! #
2. Start the mobile file explorer on the device
3. In mobile file explorer browse to RDMinjectDLL1.cab on the device
4. Tap on RDMinjectDLL1.cab to start the installation
5. Reboot the device after the installation

## Un-Install on device

There is no uninstall possible, you need to clean boot the device.

Alternatively you can change the registry key value inside 
[HKLM]\System\Kernel\InjectDll from
  "\Windows\rdmInject.dll"
to
  "\Windows\rdmInject.dll1"
Then reboot the device and the tool is in-active.

## Details

The installation cab RDMinjectDLL1.cab file copies RDMInjectDLL.dll to
\Windows directory and sets the registry key [HKLM]\System\Kernel\InjectDll to
the MULTI_SZ string "\Windows\RDMinject.dll".

# History
v 0.1
    inital version
  0.2
    added code to control the Software Input Panel (SIP) position
    The SIP is moved down if wpctsc.exe session runs in full screen mode
    If a task switch is done during full screen session, the SIP position is
    not restored and covers the standard menu bar.
    The SIP is restored if the session is ended
    It is recommended to control the SIP using a keyboard shortcut. Then it can
    be shown or hidden at any time. So you can uncover the menu bar even if the
    SIP is at top bottom position.
  0.3
    changed code to work with D70e wearable firmware 40.04
  0.4
    added option to move SIP on all WININCHANGE messages
    bSupressAllWinIniChange should be 1 to intercept and suppress all 
    WININICHANGE messages
    other registry settings may or mya not be used. DO NOT touch.
    The wearable SIP indeed uses three different windows, only the ones aligned
    to the left will be moved.
  0.5
    added code to read WININCHANGED lParam to be captured from registry value
    WINI_CHANGEDLPARM.
    For CK71 and same Intermec generation this value is 132960904
    For CK75 the value is 133005960
    
    To examine the value, if the SIP shows and hides immediately although 
    rdmInject is installed, use CE Remote Spy (ccspy.exe) and start a remote 
    desktop session. Then look for window with class TSSHELLWND and select 
    the entry. Now click on Menu "Spy" and "Messages". Then try to show the SIP
    with the assigned key and watch the messages window for "WM_WINICHANGE"
    messages and note the LPARAM value. The value has then to be converted from
    hex to decimal and can then be used as WINI_CHANGEDLPARM value. Reboot the
    device to activate the change.
    

REGEDIT4

[HKEY_LOCAL_MACHINE\Software\Honeywell\RDMinjectDLL]
"bSupressAllWinIniChange"=dword:00000001
"bEnableSubClassWindow"=dword:00000000
"bSecondWndIsChild"=dword:00000000
"WINI_CHANGEDLPARM"="132960904"
"slaveArgs"=""
"slaveExe"=""
"waitForTitleSecond"=""
"waitForClassSecond"=""
"waitForTitle"=""
"waitForClass"="TSSHELLWND"
"MODULE_FILENAME"="\\windows\\wpctsc.exe"

    