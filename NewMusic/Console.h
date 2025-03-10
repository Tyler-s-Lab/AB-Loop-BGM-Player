#pragma once

#include <string_view>

class Console {
public:
	Console();
	~Console();

public:
	void Clear();
	void Pause();

	void ErrorMessage(std::string_view message);
	void ErrorMessage(std::wstring_view message);

	void Write(std::string_view str);
	void Write(std::wstring_view str);

private:
	void* m_hstdout;
};

