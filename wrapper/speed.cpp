#include "speed.h"
#include <Windows.h>
#include "hook.h"

#pragma comment(lib, "winmm.lib")

// This is very experimental turn on at your own risk! It will likely brake Physx (simulation step size too large)
// In addition overriding some kernel.dll function, might not be the best idea in general
//#define HOOK_SPEED

const double speed = 10.;
const LONGLONG update_freq = 1000;

Hook<BOOL, LARGE_INTEGER*> hQueryPerformanceCounter;
BOOL WINAPI nQueryPerformanceCounter(LARGE_INTEGER *lpCount) {
	BOOL r = hQueryPerformanceCounter(lpCount);
	if (lpCount) {
		static LONGLONG last_poll = lpCount->QuadPart, counter = 0;

		if (last_poll + update_freq < lpCount->QuadPart) {
			// Update the counter and last_poll. update_freq makes sure we don't suffer from numerical issues when the difference between lpCount and last_poll is small
			counter += (LONGLONG) (speed * (lpCount->QuadPart - last_poll));
			last_poll = lpCount->QuadPart;
			lpCount->QuadPart = counter;
		}
		else
			lpCount->QuadPart = counter + (LONGLONG)( speed * (lpCount->QuadPart - last_poll) );
	}
	return r;
}
Hook<DWORD> htimeGetTime;
DWORD WINAPI ntimeGetTime() {
	//LOG(INFO) << "timeGetTime";
	return  DWORD(htimeGetTime() * speed);
}
Hook<ULONGLONG> hGetTickCount64;
ULONGLONG WINAPI nGetTickCount64(void) {
	//LOG(INFO) << "GetTickCount64";
	return  ULONGLONG(hGetTickCount64() * speed);
}
Hook<DWORD> hGetTickCount;
DWORD WINAPI nGetTickCount(void) {
	//LOG(INFO) << "GetTickCount";
	return  DWORD(hGetTickCount() * speed);
}
Hook<void, LPFILETIME> hGetSystemTimeAsFileTime;
void WINAPI nGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime) {
	//LOG(INFO) << "GetSystemTimeAsFileTime";
	hGetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

Hook<DWORD, DWORD, CONST HANDLE *, BOOL, DWORD> hWaitForMultipleObjects;
DWORD WINAPI nWaitForMultipleObjects(DWORD nCount, CONST HANDLE * lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
	return hWaitForMultipleObjects(nCount, lpHandles, bWaitAll, (DWORD)(dwMilliseconds / speed));
}

Hook<DWORD, HANDLE, DWORD> hWaitForSingleObject;
DWORD WINAPI nWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
	return hWaitForSingleObject(hHandle, (DWORD)(dwMilliseconds / speed));
}

Hook<void , DWORD> hSleep;
void WINAPI nSleep(DWORD dwMilliseconds) {
	hSleep((DWORD)(dwMilliseconds / speed));
}
void hookSpeed() {
#ifdef HOOK_SPEED
	hQueryPerformanceCounter.setup(QueryPerformanceCounter, nQueryPerformanceCounter);
	htimeGetTime.setup(timeGetTime, ntimeGetTime);
	hGetTickCount64.setup(GetTickCount64, nGetTickCount64);
	hGetTickCount.setup(GetTickCount, nGetTickCount);
	hGetSystemTimeAsFileTime.setup(GetSystemTimeAsFileTime, nGetSystemTimeAsFileTime);
	hWaitForMultipleObjects.setup(WaitForMultipleObjects, nWaitForMultipleObjects);
	hWaitForSingleObject.setup(WaitForSingleObject, nWaitForSingleObject);
	hSleep.setup(Sleep, nSleep);
#endif
}
void unhookSpeed() {
#ifdef HOOK_SPEED
	hQueryPerformanceCounter.disable();
	htimeGetTime.disable();
	hGetTickCount64.disable();
	hGetTickCount.disable();
	hGetSystemTimeAsFileTime.disable();
	hWaitForMultipleObjects.disable();
	hWaitForSingleObject.disable();
	hSleep.disable();
#endif
}