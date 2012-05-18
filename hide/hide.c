#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

int WINAPI _tWinMain(HINSTANCE a, HINSTANCE b, LPTSTR lpCmdLine, int d) {
	UNREFERENCED_PARAMETER(a);
	UNREFERENCED_PARAMETER(b);
	UNREFERENCED_PARAMETER(d);

	//MessageBox(NULL, lpCmdLine, TEXT("Command Line"), MB_OK);

	DWORD exitCode = 0;

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, lpCmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
		return -2;
	}
	// Wait until child process exits and get the exit code
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &exitCode);
	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return exitCode;
}
