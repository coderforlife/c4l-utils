#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#pragma comment( lib, "psapi.lib" )

#include <tchar.h>
#include <stdio.h>

#define FT_TO_SEC(ft) (((((__int64)ft.dwHighDateTime)<<32) | ((__int64)ft.dwLowDateTime)) / 10000000.0)
#define LI_TO_SEC(li) (((((__int64)li.HighPart)<<32) | ((__int64)li.LowPart)) / 10000000.0)

void DisplayError(LPTSTR desc, DWORD dwLastError)
{
	LPTSTR MessageBuffer;
	DWORD dwBufferLength;
	if ((dwBufferLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&MessageBuffer, 0, NULL)) > 0)
	{
		_ftprintf(stderr, TEXT("%s: 0x%08X %s\n"), desc, dwLastError, MessageBuffer);
		LocalFree(MessageBuffer);
	}
	else
	{
		_ftprintf(stderr, TEXT("%s: 0x%08X\n"), desc, dwLastError);
	}
}

int _tmain(int argc, TCHAR* argv[])
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;
	IO_COUNTERS ic;
	PROCESS_MEMORY_COUNTERS pmc;
	DWORD exitCode = 0;

	LPTSTR lpCmdLine = GetCommandLine();
	BOOL quoted = lpCmdLine[0] == L'"';

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pmc, sizeof(pmc));

	// Get the command line, run the process, and then wait for it to terminate
	++lpCmdLine; // skips the " or the first letter (all paths are at least 1 letter)
	while (*lpCmdLine)
	{
		if (quoted && lpCmdLine[0] == L'"') { quoted = FALSE; } // found end quote
		else if (!quoted && lpCmdLine[0] == L' ') // found an unquoted space, now skip all spaces
		{
			do { ++lpCmdLine; } while (lpCmdLine[0] == L' ');
			break;
		}
		++lpCmdLine;
	}
	if (!CreateProcess(NULL, lpCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		DisplayError(TEXT("Failed to start process"), GetLastError());
		return 127;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Get and display process information
	if (GetProcessTimes(pi.hProcess, &CreationTime, &ExitTime, &KernelTime, &UserTime))
	{
		_ftprintf(stderr, TEXT("Real Time:         %.3f sec\nUser Time:         %.3f sec\nKernel Time:       %.3f sec\n"),
			FT_TO_SEC(ExitTime) - FT_TO_SEC(CreationTime), FT_TO_SEC(UserTime), FT_TO_SEC(KernelTime));
	}
	else { DisplayError(TEXT("Failed to get time usage of process"), GetLastError()); }
	if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc)))
	{
		_ftprintf(stderr, TEXT("Page Faults:       %Iu\n"),    pmc.PageFaultCount);
		_ftprintf(stderr, TEXT("Max Memory:        %Iu Kb\nMax Commit:        %Iu Kb\n"), pmc.PeakWorkingSetSize / 1024, pmc.PeakPagefileUsage / 1024);
		_ftprintf(stderr, TEXT("Max Paged Pool:    %Iu Kb\nMax Nonpaged Pool: %Iu Kb\n"), pmc.QuotaPeakPagedPoolUsage / 1024, pmc.QuotaPeakNonPagedPoolUsage / 1024);
	}
	else { DisplayError(TEXT("Failed to get memory usage of process"), GetLastError()); }
	if (GetProcessIoCounters(pi.hProcess, &ic))
	{
		_ftprintf(stderr, TEXT("I/O Read:          %I64u Kb during %I64u operations\n"), ic.ReadTransferCount / 1024, ic.ReadOperationCount);
		_ftprintf(stderr, TEXT("I/O Write:         %I64u Kb during %I64u operations\n"), ic.WriteTransferCount / 1024, ic.WriteOperationCount);
		_ftprintf(stderr, TEXT("I/O Other:         %I64u Kb during %I64u operations\n"), ic.OtherTransferCount / 1024, ic.OtherOperationCount);
	}
	else { DisplayError(TEXT("Failed to get I/O usage of process"), GetLastError()); }

	// Close process and thread handles and return the exit code
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return exitCode;
}
