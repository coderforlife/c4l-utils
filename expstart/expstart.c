#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "GetBaseAddress.h"
#include "functions.h"

const LPTSTR prog = TEXT("explorer.exe");
const LPTSTR name = TEXT("expstart.exe");

int logError(LPTSTR s) {
	TCHAR text[1024];
	wsprintf(text, s, GetLastError());
	MessageBox(NULL, text, name, MB_OK | MB_ICONERROR);
	return 1;
}

int logErrorLoop(LPTSTR s, DWORD i) {
	TCHAR text[1024];
	wsprintf(text, s, i, GetLastError());
	MessageBox(NULL, text, name, MB_OK | MB_ICONERROR);
	return 1;
}

void resume(PROCESS_INFORMATION pi) {
	if (ResumeThread(pi.hThread) == -1) {
		logError(TEXT("Resuming thread failed: %u\n"));
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	TCHAR WinDir[MAX_PATH];
	BYTE *base;
	DWORD rsrcVA, bitmapOffset, i = 0;
	HANDLE proc;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Make Windows directory to be current directory
	if (!GetWindowsDirectory(WinDir, MAX_PATH) || !SetCurrentDirectory(WinDir)) {
		logError(TEXT("Failed to set Windows directory as current: %u\n"));
	}

	// Enable debugging privileges
	if (!EnablePriv(SE_DEBUG_NAME)) {
		return logError(TEXT("Failed to enable debugging privileges: %u\n"));
	}

	// Set explorer as the shell (otherwise it doesn't start properly)
	HKEY winlogon;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), 0, KEY_SET_VALUE, &winlogon) != ERROR_SUCCESS) {
		return logError(TEXT("Failed to open registry key\n"));
	}
	if (RegSetValueEx(winlogon, TEXT("Shell"), 0, REG_SZ, (LPBYTE)prog, (_tcslen(prog)+1)*sizeof(TCHAR)) != ERROR_SUCCESS) {
		return logError(TEXT("Failed to set registry value\n"));
	}

	// Start the process
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (!CreateProcess(prog, NULL, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		return logError(TEXT("Failed to create process: %u\n"));
	}
	proc = pi.hProcess;

	// Get the base address
	if (!(base = GetBaseAddress(proc))) {
		resume(pi);
		return logError(TEXT("Failed to get base address: %u\n"));
	}

	// Get the RSRC section information
	if (!(rsrcVA = getRSRCSectionVA(proc, base))) {
		resume(pi);
		return logError(TEXT("Failed to find RSRC section: %u\n"));
	}

	// Get the BITMAP directory entry
	if (!(bitmapOffset = getDirectoryOffset(proc, base+rsrcVA, RT_BITMAP))) {
		resume(pi);
		return logError(TEXT("Failed to find BITMAP directory entry: %u\n"));
	}

	// Update all bitmaps
	for (i = 6801; i <= 6812; i++) {
		// Retrieve the resource
		DWORD size;
		LPVOID data = GetResource(MAKEINTRESOURCE(i), &size);
		if (!data) {
			logErrorLoop(TEXT("Failed to get resource '%u': %u\n"), i);
			continue;
		}

		// Update the data
		if (!updateBitmap(proc, MAKEINTRESOURCE(i), base, rsrcVA, bitmapOffset, data, size)) {
			logErrorLoop(TEXT("Failed to update BITMAP '%u': %u\n"), i);
			continue;
		}
	}

	// Resume the new process
	resume(pi);

	// Make sure it has started
	Sleep(5000);

	// Set this program back as the shell
	if (RegSetValueEx(winlogon, TEXT("Shell"), 0, REG_SZ, (LPBYTE)name, (_tcslen(name)+1)*sizeof(TCHAR)) != ERROR_SUCCESS) {
		return logError(TEXT("Failed to re-set registry value\n"));
	}
	RegCloseKey(winlogon);

	return 0;
}
