#include "io.h"
#include "util.h"
#include "hook.h"
#include <unordered_map>

static std::unordered_map<HWND, IOHook*> hook_map;

static LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (hook_map.count(hwnd))
		return hook_map[hwnd]->wndProc(uMsg, wParam, lParam);
	return S_OK;
}

IOHook::~IOHook() {
}

LRESULT IOHook::wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return CallWindowProc(prev_wnd_proc_, hwnd_, uMsg, wParam, lParam);
}

BOOL IOHook::getMessage(LPMSG m) {
	return TRUE;
}

BOOL IOHook::peekMessage(LPMSG m) {
	return TRUE;
}

LONG_PTR IOHook::setWindowLongPtr(int nIndex, LONG_PTR dwNewLong) {
	if (nIndex == GWLP_WNDPROC) {
		WNDPROC o = prev_wnd_proc_;
		prev_wnd_proc_ = (WNDPROC)dwNewLong;
		return (LONG_PTR)o;
	}
	return 0;
}

void IOHookHigh::sendKeyDown(unsigned char key, bool syskey) {
	LPARAM lParam = 1 | (1 << 30);
	wndProc(syskey ? WM_SYSKEYDOWN : WM_KEYDOWN, key, lParam);
}

void IOHookHigh::sendKeyUp(unsigned char key, bool syskey) {
	LPARAM lParam = 1 | (3 << 30);
	wndProc(syskey ? WM_SYSKEYUP : WM_KEYUP, key, lParam);
}

void IOHookHigh::sendMouse(uint16_t x, uint16_t y, uint8_t button, UINT msg_o) {
	UINT msg = 0;
	if (button == 0)
		msg = WM_MOUSEMOVE;
	if (button == 1)
		msg = WM_LBUTTONDOWN;
	else if (button == 2)
		msg = WM_LBUTTONDOWN;
	else if (button == 3)
		msg = WM_LBUTTONDOWN;
	else LOG(WARN) << "Unknown mouse button " << button;
	if (msg)
		wndProc(msg + msg_o, 0, MAKELPARAM(x, y));
}
void IOHookHigh::sendMouseDown(uint16_t x, uint16_t y, uint8_t button) {
	if (button == 0) LOG(WARN) << "Unknown mouse button " << button;
	else sendMouse(x, y, button, 0/*DOWN*/);
}
void IOHookHigh::sendMouseUp(uint16_t x, uint16_t y, uint8_t button) {
	if (button == 0) LOG(WARN) << "Unknown mouse button " << button;
	else sendMouse(x, y, button, 1/*DOWN*/);
}

LRESULT IOHookHigh::wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (WM_MOUSEFIRST <= uMsg && uMsg <= WM_MOUSELAST) {
		// TODO: Handle mouse events
	}
	if (WM_KEYFIRST <= uMsg && uMsg <= WM_KEYLAST) {
		// Handle keyboard events
		if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) {
			if (0 < wParam && wParam < 255) {
				key_state[wParam] = DOWN;
				bool state_switch = !(lParam & (1 << 30));

				unsigned char special_state = (CTRL*key_state[VK_CONTROL]) | (SHIFT*key_state[VK_SHIFT]) | (ALT*key_state[VK_MENU]) | (SWITCH*state_switch);
				if ((wParam < VK_SHIFT || wParam > VK_MENU) && keyDown(wParam, special_state))
					return 0;
			}
		}
		else if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) {
			if (0 < wParam && wParam < 255) {
				key_state[wParam] = UP;
				if ((wParam < VK_SHIFT || wParam > VK_MENU) && keyUp(wParam))
					return 0;
			}
		}
//		else
//			LOG(WARN) << "Unhandled keyboard event " << uMsg;
	}
	//if (uMsg == WM_TIMER)
	//	LOG(INFO) << "Timer " << wParam << " " << lParam;
	return IOHook::wndProc(uMsg, wParam, lParam);
}



Hook<LONG_PTR, HWND, int, LONG_PTR> hSetWindowLongPtrA;
LONG_PTR WINAPI nSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
	if (hook_map.count(hWnd)) {
		LONG_PTR r = hook_map[hWnd]->setWindowLongPtr(nIndex, dwNewLong);
		if (r) return r;
	}
	return hSetWindowLongPtrA(hWnd, nIndex, dwNewLong);
}

Hook<LONG_PTR, HWND, int, LONG_PTR> hSetWindowLongPtrW;
LONG_PTR WINAPI nSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
	if (hook_map.count(hWnd)) {
		LONG_PTR r = hook_map[hWnd]->setWindowLongPtr(nIndex, dwNewLong);
		if (r) return r;
	}
	return hSetWindowLongPtrW(hWnd, nIndex, dwNewLong);
}
#ifdef UNICODE
#define hSetWindowLongPtr  hSetWindowLongPtrW
#else
#define hSetWindowLongPtr  hSetWindowLongPtrA
#endif // !UNICODE

bool wrapIO(HWND hwnd, std::shared_ptr<IOHook> h) {
	if (hook_map.count(hwnd)) {
		LOG(WARN) << "Same HWND hooked twice, ignoring second hook!";
		return false;
	}
	else {
		hook_map[hwnd] = h.get();
		h->hwnd_ = hwnd;
		h->prev_wnd_proc_ = (WNDPROC)hSetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
		return true;
	}
}

Hook<BOOL, LPMSG, HWND, UINT, UINT> hGetMessageA;
BOOL WINAPI nGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
	if (!hGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax)) return FALSE;
	if (hook_map.count(hWnd)) {
		return hook_map[hWnd]->getMessage(lpMsg);
	}
	return TRUE;
}

Hook<BOOL, LPMSG, HWND, UINT, UINT> hGetMessageW;
BOOL WINAPI nGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
	if (!hGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax)) return FALSE;
	if (hook_map.count(hWnd)) {
		return hook_map[hWnd]->getMessage(lpMsg);
	}
	return TRUE;
}

Hook<BOOL, LPMSG, HWND, UINT, UINT, UINT> hPeekMessageA;
BOOL WINAPI nPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
	if (!hPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg)) return FALSE;
	if (hook_map.count(hWnd)) {
		return hook_map[hWnd]->peekMessage(lpMsg);
	}
	return TRUE;
}

Hook<BOOL, LPMSG, HWND, UINT, UINT, UINT> hPeekMessageW;
BOOL WINAPI nPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
	if (!hPeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg)) return FALSE;
	if (hook_map.count(hWnd)) {
		return hook_map[hWnd]->peekMessage(lpMsg);
	}
	return TRUE;
}


Hook<BOOL, int, int> hSetCursorPos;
BOOL WINAPI nSetCursorPos(int X, int Y) {
	//LOG(INFO) << "Set cursor position " << X << " , " << Y;
	return hSetCursorPos(X, Y);
}
Hook<BOOL, LPPOINT> hGetCursorPos;
BOOL WINAPI nGetCursorPos(LPPOINT p) {
	BOOL r = hGetCursorPos(p);
	//LOG(INFO) << "Get cursor position " << p->x << " , " << p->y;
	return r;
}
Hook<HCURSOR, HCURSOR> hSetCursor;
HCURSOR WINAPI nSetCursor(HCURSOR c) {
//	LOG(INFO) << "Set cursor " << c;
	return hSetCursor(c);
}

void hookIO() {
	hSetWindowLongPtrW.setup(SetWindowLongPtrW, nSetWindowLongPtrW);
	hSetWindowLongPtrA.setup(SetWindowLongPtrA, nSetWindowLongPtrA);
	hSetCursorPos.setup(SetCursorPos, nSetCursorPos);
	hGetCursorPos.setup(GetCursorPos, nGetCursorPos);
	hSetCursor.setup(SetCursor, nSetCursor);
	hGetMessageA.setup(GetMessageA, nGetMessageA);
	hGetMessageW.setup(GetMessageW, nGetMessageW);
	hPeekMessageA.setup(PeekMessageA, nPeekMessageA);
	hPeekMessageW.setup(PeekMessageW, nPeekMessageW);
}
void unhookIO() {
	hSetWindowLongPtrW.disable();
	hSetWindowLongPtrA.disable();
	hSetCursorPos.disable();
	hGetCursorPos.disable();
	hSetCursor.disable();
	hGetMessageA.disable();
	hGetMessageW.disable();
	hPeekMessageA.disable();
	hPeekMessageW.disable();
}
