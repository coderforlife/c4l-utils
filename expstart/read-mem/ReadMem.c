#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "FindProcess.h"
#include "functions.h"

// Originally in functions.h
#include "Memory.h"
LPVOID getData(HANDLE proc, BYTE *base, SIZE_T offset, DWORD *length) {
	IMAGE_RESOURCE_DATA_ENTRY de;
	LPVOID data;
	if (!ReadMem(proc, &de, sizeof(IMAGE_RESOURCE_DATA_ENTRY), base+offset)) { return NULL; }
	*length = de.Size;
	data = malloc(de.Size);
	if (de.Size == 0) { return data; }
	if (!ReadMem(proc, data, de.Size, base+de.OffsetToData)) {
		free(data);
		return NULL;
	}
	return data;
}

int _tmain(int argc, _TCHAR* argv[]) {
	BYTE *base;
	DWORD pid, rsrcVA, bitmapOffset, i = 0;
	HANDLE proc;

	// Enable debugging privileges
	if (!EnablePriv(SE_DEBUG_NAME)) {
		_tprintf(TEXT("Enable debugging privileges failed: %u\n"), GetLastError());
		return 1;
	}

	// Find the process and get it's base address
	if (!(pid = FindProcess(TEXT("explorer.exe"), &base))) {
		_tprintf(TEXT("Could not find process\n"));
		return 1;
	}

	// Open the process with READ / WRITE and other privileges
	if (!(proc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid))) {
		_tprintf(TEXT("Could not open process: %u\n"), GetLastError());
		return 1;
	}

	// Get the RSRC section information
	if (!(rsrcVA = getRSRCSectionVA(proc, base))) {
		_tprintf(TEXT("Finding RSRC section failed: %u\n"), GetLastError());
		CloseHandle(proc);
		return 1;
	}

	// Get the BITMAP directory entry
	if (!(bitmapOffset = getDirectoryOffset(proc, base+rsrcVA, RT_BITMAP))) {
		_tprintf(TEXT("Finding BITMAP directory entry failed: %u\n"), GetLastError());
		CloseHandle(proc);
		return 1;
	}

	// Get all bitmaps
	for (i = 6801; i <= 6812; i++) {
		DWORD nameOffset = getDirectoryOffset(proc, base+rsrcVA+bitmapOffset, MAKEINTRESOURCE(i));
		DWORD langOffset = getFirstDirectoryOffset(proc, base+rsrcVA+nameOffset);
		LPBYTE data;
		DWORD size, j;

		if (!nameOffset || !langOffset) {
			_tprintf(TEXT("Failed to find BITMAP '%u': %u\n"), i, GetLastError());
			continue;
		}

		if (!(data = getData(proc, base, rsrcVA+langOffset, &size))) {
			_tprintf(TEXT("Failed to get BITMAP '%u' data: %u\n"), i, GetLastError());
			continue;
		}

		_tprintf(TEXT("BITMAP '%u' is %u bytes:"), i, size);
		for (j = 0; j < 15; j++) {
			_tprintf(TEXT(" %02X"), data[j]);
		}
		_tprintf(TEXT("... \n"));

		free(data);
	}

	CloseHandle(proc);

	return 0;
}
