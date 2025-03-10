#include "Console.h"

#define WIN32_LEAN_AND_MEAN (1)
#include <Windows.h>
#include <cstdlib>

Console::Console() {
	m_hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

Console::~Console() {}

void Console::Clear() {
	system("cls");
}

void Console::Pause() {
	system("pause");
}

void Console::ErrorMessage(std::string_view message) {
	Write(message);
	Pause();
}

void Console::ErrorMessage(std::wstring_view message) {
	Write(message);
	Pause();
}

void Console::Write(std::string_view str) {
	WriteConsoleA(m_hstdout, str.data(), static_cast<DWORD>(str.length()), NULL, NULL);
}

void Console::Write(std::wstring_view str) {
	WriteConsoleW(m_hstdout, str.data(), static_cast<DWORD>(str.length()), NULL, NULL);
}
