#include "rawinput.h"

int rawInitMouse(HWND target) {
	RAWINPUTDEVICE rid;

	rid.usUsagePage = 0x01; 
	rid.usUsage = 0x02; 
	rid.dwFlags = 0;//RIDEV_INPUTSINK;
	rid.hwndTarget = target;

#ifdef _DEBUG
	rid.dwFlags = RIDEV_INPUTSINK;
#endif

	if ( FAILED(RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) )
		return -1;

	return 0;
}