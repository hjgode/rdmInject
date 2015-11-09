# rdmInject
inject DLL into RDM to watch RDM Windows Messages

Using HKLM\System\Kernel\inhectDLL list this DLL will look for Remote Desktop Mobile being started and then adds some extras.

Currently the automatic hide of the SIP (Software Input Panel) is catched when RDM runs in full screen mode. So the SIP remains visible until hidden manually.

Additinally the DLL will start two other apps, RdmAddonBatt2 and RdmAddonWifi, to show the battery and RSSI when RDM is in full screen mode.

