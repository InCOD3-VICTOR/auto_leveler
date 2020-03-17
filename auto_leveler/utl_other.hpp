#pragma once
#include <Windows.h>
#include <stdint.h>

namespace input
{
	void send( int32_t virtual_key )
	{
		INPUT input{};

		input.type = INPUT_KEYBOARD;

		input.ki.dwFlags = KEYEVENTF_SCANCODE;
		input.ki.time = 0;
		input.ki.dwExtraInfo = 0;

		input.ki.wScan = MapVirtualKeyA( virtual_key, MAPVK_VK_TO_VSC );
		input.ki.wVk = virtual_key;

		SendInput( 1, &input, sizeof( INPUT ) );

		input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

		SendInput( 1, &input, sizeof( INPUT ) );
	}
}