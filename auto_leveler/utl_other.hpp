#pragma once
#include <Windows.h>
#include <stdint.h>

namespace input
{
	void send( int32_t virtual_key, int32_t scan_code )
	{
		INPUT input{};

		input.type = INPUT_KEYBOARD;

		input.ki.dwFlags = KEYEVENTF_SCANCODE;
		input.ki.time = 0;
		input.ki.dwExtraInfo = 0;

		input.ki.wScan = scan_code;
		input.ki.wVk = virtual_key;

		SendInput( 1, &input, sizeof( INPUT ) );

		input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

		SendInput( 1, &input, sizeof( INPUT ) );
	}
}