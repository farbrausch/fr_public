#define _CRT_SECURE_NO_DEPRECATE
#include "console.h"

Console::Console() {
	AllocConsole();
	hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hstdin = GetStdHandle(STD_INPUT_HANDLE);
	freopen("CONOUT$", "a", stdout); // redirect console to stdout
	freopen("CONOUT$", "r", stdin); // redirect stdin to console
}

void Console::getPos(int &x, int &y) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout,&csbi);
	y = csbi.dwCursorPosition.Y;
	x = csbi.dwCursorPosition.X;
}

void Console::setPos(int x, int y) {
	COORD c;
	c.X = x;
	c.Y = y;
	SetConsoleCursorPosition(hstdout,c);
}
 
void Console::fullscreen() {
	/*HWND wnd = GetConsoleWindow();
	PostMessage(wnd,WM_SYSKEYDOWN,VK_RETURN,(1<<29));*/
}

char Console::getch(WORD &vk) {
	INPUT_RECORD inrec;
	DWORD e_size;
	if(!ReadConsoleInput(hstdin,&inrec,1,&e_size))
		return 0;
	if(inrec.EventType != KEY_EVENT)
		return 0;
	if(!inrec.Event.KeyEvent.bKeyDown)
		return 0;
	vk = inrec.Event.KeyEvent.wVirtualKeyCode;
	return inrec.Event.KeyEvent.uChar.AsciiChar;
}

void Console::printf(const char* txt, ...) {
	char buffer[1024];

	va_list argptr;

	va_start(argptr,txt);

	int size = vsprintf(buffer,txt,argptr);

	va_end(argptr);

	DWORD e_size;

	WriteConsole(hstdout,buffer,size,&e_size,0);
}

Console::~Console() {
	FreeConsole();
}
