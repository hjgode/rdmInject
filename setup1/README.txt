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
2. Start the mobile file explorer on the device
3. In mobile file explorer browse to RDMinjectDLL1.cab on the device
4. Tap on RDMinjectDLL1.cab to start the installation
5. Reboot the device after the installation

## Un-Install on device

There is no uninstall possible, you need to clean boot the device.

## Details

The installation cab RDMinjectDLL1.cab file copies RDMInjectDLL.dll to
\Windows directory and sets the registry key [HKLM]\System\Kernel\InjectDll to
the MULTI_SZ string "\Windows\RDMinject.dll".  