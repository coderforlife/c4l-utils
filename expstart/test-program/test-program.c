#define _WIN32_WINNT 0x0501

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include "mingw-unicode.c"
int _tmain(int argc, _TCHAR* argv[])
{
	MessageBox(NULL, TEXT("Hello World!"), TEXT("Hello World"), MB_OK);
	return 0;
}

