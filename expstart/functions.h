#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

LPVOID GetResource(LPCTSTR name, DWORD *size);

BOOL EnablePriv(LPCTSTR lpszPriv);

DWORD getRSRCSectionVA(HANDLE proc, BYTE *offset);
DWORD getDirectoryOffset(HANDLE proc, BYTE *offset, LPTSTR id);
DWORD getFirstDirectoryOffset(HANDLE proc, BYTE *offset);

BOOL setData(HANDLE proc, BYTE *base, SIZE_T offset, VOID *data, DWORD length);

BOOL updateBitmap(HANDLE proc, LPTSTR id, BYTE *base, DWORD rsrcVA, DWORD bitmapOffset, LPVOID data, DWORD size);

#endif
