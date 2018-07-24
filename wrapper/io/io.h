#pragma once
#include <Windows.h>
#include <memory>
#include <vector>

struct IOHook {
	HWND hwnd_ = 0;
	WNDPROC  prev_wnd_proc_ = 0;
	virtual ~IOHook();

	// Low level io API
	virtual LRESULT wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL getMessage(LPMSG m);
	virtual BOOL peekMessage(LPMSG m);
	virtual LONG_PTR setWindowLongPtr(int nIndex, LONG_PTR dwNewLong);
};

struct IOHookHigh : public IOHook {
	enum SpecialStatus {
		CTRL = 1,
		SHIFT = 2,
		ALT = 4,
		SWITCH = 8,
	};
	enum KeyState {
		UP = 0,
		DOWN = 1,
	};
	std::vector<uint8_t> key_state;

	// Handle key down and up events (return true to swallow)
	virtual bool handleKeyDown(unsigned char key, unsigned char special_status) = 0;
	virtual bool handleKeyUp(unsigned char key) = 0;
	virtual void sendKeyDown(unsigned char key, bool syskey=false);
	virtual void sendKeyUp(unsigned char key, bool syskey = false);
	virtual void sendMouse(uint16_t x, uint16_t y, uint8_t button, UINT o);
	virtual void sendMouseDown(uint16_t x, uint16_t y, uint8_t button);
	virtual void sendMouseUp(uint16_t x, uint16_t y, uint8_t button);

	virtual LRESULT wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	IOHookHigh() :key_state(255, UP) {}
};

bool wrapIO(HWND hwnd, std::shared_ptr<IOHook> h);
void hookIO();
void unhookIO();
