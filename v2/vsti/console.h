#pragma once

#include <windows.h>
#include <stdio.h>

class Console {
public:
	HANDLE hstdout;
	HANDLE hstdin;

	void printf(const char* txt, ...);
	char getch(WORD &vk);
	void getPos(int &x, int &y);
	void setPos(int x, int y);	
	void fullscreen();
	Console();
	~Console();
};