//-------------------------------------------------------------------------
//
// File: Utils.cpp
//
// helper functions
//
//--------------------------------------------------------------------------
#pragma once
#include "Utils.h"
#include "stdafx.h"

namespace Utils
{
	//Convert a single ascii character to a 4 bit hex value
	//If the character is outside outside the range [0-9a-fA-F] throw
	char CharToHex(char character)
	{
		character = static_cast<char>(tolower(character));
		if (!isalnum(character))
		{
			throw "Invalid character passed to CharToHex. Character must be alpha-numeric";
		}

		char temp = 0;
		if (isdigit(character))
		{
			temp = character - '0';
		}
		else
		{
			temp = character - 'a' + 10;
		}
		if (temp < 0 || temp > 15)
		{
			throw "Invalid character passed to CharToHex. Character must be in between 0-f";
		}
		return temp;
	}

	//Convert a null terminated string to a byte array
	void StringToHex(const char* str, BYTE* hex)
	{
		if (!str || !hex)
		{
			throw "NULL parameter passed to StringToHex";
		}

		while (*str != 0 && *str + 1 != 0)// && *hex != 0)
		{
			if (*str == '-')
			{
				//Handle uuid's. Ideally this belongs in a higher level function, but as we have simple use case, it's easier to handle here.
				//As the message headers are little-endian, we don't need to endian conversion
				str++;
				continue;
			}

			char hiNibble = CharToHex(*str);
			str++;

			char loNibble = CharToHex(*str);

			*hex = (hiNibble << 4) + loNibble;
			hex++;
			str++;
		}
	}

	void ClearScreen(HANDLE consoleHandle, uint32_t lines)
	{
		COORD coords{ 0, 0 };
		DWORD charsWritten, length;

		CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;
		GetConsoleScreenBufferInfo(consoleHandle, &consoleBufferInfo);

		if (lines == 0)
		{
			lines = consoleBufferInfo.dwSize.Y;
		}
		length = consoleBufferInfo.dwSize.X * lines;

		FillConsoleOutputCharacter(consoleHandle, _TCHAR(' '), length, coords, &charsWritten);

		SetConsoleCursorPosition(consoleHandle, coords);
	}
}