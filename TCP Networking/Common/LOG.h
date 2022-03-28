#pragma once

#include <iostream>
#include <Windows.h>
#include <string>

namespace Debreky {
	enum Type
	{
		Error = 0, Warning, Info
	};
#ifdef _DEBUG

	void Color(int color)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	}

	void LOG(std::string&& input, Type type)
	{
		switch (type)
		{
		case Debreky::Error:
			Color(4);
			std::cout << "[ERROR] : ";
			Color(7);
			std::cout << input;
			break;
		case Debreky::Warning:
			Color(14);
			std::cout << "[WARNING] : ";
			Color(7);
			std::cout << input;
			break;
		case Debreky::Info:
			Color(10);
			std::cout << "[INFO] : ";
			Color(7);
			std::cout << input;
			break;
		default:
			break;
		}
	}

#else
	void LOG(const char* input, Type type) {}
#endif
}