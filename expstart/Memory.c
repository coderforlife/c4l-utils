#include "Memory.h"

BOOL ReadMem(HANDLE proc, VOID *data, SIZE_T count, VOID *address) {
	SIZE_T dwRead = 0;
	return ReadProcessMemory(proc, address, data, count, &dwRead);
}

BOOL WriteMem(HANDLE proc, VOID *data, SIZE_T count, VOID *address) {
	SIZE_T dwWrite = 0;
	MEMORY_BASIC_INFORMATION memInfo;
	ZeroMemory(&memInfo, sizeof(MEMORY_BASIC_INFORMATION));
	if (!VirtualQueryEx(proc, address, &memInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
		return FALSE;
	} else if (memInfo.Protect == PAGE_READONLY) {
		DWORD oldProtect;
		if (!VirtualProtectEx(proc, address, count, PAGE_WRITECOPY, &oldProtect)) {
			return FALSE;
		}
	}
	return WriteProcessMemory(proc, address, data, count, &dwWrite);
}
