//-------------------------------------------------------------------------
//
// File: Utils.h
//
// helper functions
//
//--------------------------------------------------------------------------
#pragma once
#include "stdafx.h"

namespace Utils
{
	//Convert a single ascii character to a 4 bit hex value
	//If the character is outside outside the range [0-9a-fA-F] throw
	char CharToHex(char character);

	//Convert a null terminated string to a byte array
	void StringToHex(const char* str, BYTE* hex);

	//Clear the console screen
	//Lines number of lines to clear. default clears the whole screen
	void ClearScreen(HANDLE consoleHandle, uint32_t lines = 0);
}