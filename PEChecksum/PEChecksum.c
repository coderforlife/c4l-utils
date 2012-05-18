#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h> // only things used are simple IO (could be replaced with stdio) and some file structures
#include <tchar.h>
#include <stdio.h>

// Macro to find the offset of a field of a structure
#ifndef offsetof
#define offsetof(st, m) ((size_t) ( (char *)&((st *)(0))->m - (char *)0 ))
#endif

// NT Signature + IMAGE_FILE_HEADER + Most of IMAGE_OPTIONAL_HEADER
// This is relative to the PE Header Offset
#define CHECKSUM_OFFSET sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+offsetof(IMAGE_OPTIONAL_HEADER, CheckSum)

/**
Closes the handle but saves the current value of GetLastError() instead of
having it set to 0 by CloseHandle().
Always returns FALSE.
**/
BOOL CloseSaveError(HANDLE f) {
	DWORD dwError = GetLastError();
	CloseHandle(f);
	SetLastError(dwError);
	return FALSE;
}

/**
Easier to use ReadFile function. On failure of ReadFile or reading sufficent
bytes the file handle is closed and FALSE is returned. Otherwise returns TRUE.
**/
BOOL Read(HANDLE f, LPVOID lpBuffer, DWORD nNumberOfBytesToRead) {
	DWORD dwRead;
	if (!ReadFile(f, lpBuffer, nNumberOfBytesToRead, &dwRead, NULL) || dwRead != nNumberOfBytesToRead)
		return CloseSaveError(f);
	return TRUE;
}

/**
Easier to use SetFilePointer function. On failure of SetFilePointer bytes the
file handle is closed and FALSE is returned. Otherwise returns TRUE.
**/
BOOL Seek(HANDLE f, LONG lDistanceToMove, DWORD dwMoveMethod) {
	if (!SetFilePointer(f, lDistanceToMove, 0, dwMoveMethod))
		return CloseSaveError(f);
	return TRUE;
}

/**
Calculates the checksum for a chunk of data.
Requires the current checksum (or 0 for start), the data, and the length of the data.
Returns the new checksum.
**/
WORD ChkSum(WORD oldChk, USHORT *ptr, DWORD len) {
	DWORD c = oldChk, l, j;
	while (len) {
		l = min(len, 0x4000);
		len -= l;
		for (j=0; j<l; ++j) {
			c += *ptr++;
		}
		c = (c&0xffff) + (c>>16);
	}
	c = (c&0xffff) + (c>>16);
	return (WORD)c;
}

#define BUF_SIZE 0x10000
/**
Updates the PE file checksum, given the current check sum and an offset to the
PE file header. Returns the new checksum, or 0 if failed (0 could techincally
be the checksum though). Check err for errors.
**/
DWORD UpdatePEChkSum(LPCTSTR filename, DWORD curCheckSum, DWORD peOffset, OUT BOOL *err) {
	DWORD nBytes = 0, dwCheck = 0, yy;
	LPBYTE mem;
	HANDLE f = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	*err = TRUE;
	if (f == INVALID_HANDLE_VALUE) { return 0; }

	// Read the entire file, calculating the checksum along the way
	mem = LocalAlloc(LMEM_FIXED, BUF_SIZE);
	if (!mem) { return 0; }
	while (ReadFile(f, mem, BUF_SIZE, &nBytes, 0) && nBytes > 0) {
		dwCheck = ChkSum((WORD)dwCheck, (PUSHORT)mem, nBytes/2);
	}
	if (nBytes & 1) { // We have to handle an extra byte if it exists
		dwCheck += mem[nBytes-1];
		dwCheck = (dwCheck>>16) + (dwCheck&0xffff);
	}
	LocalFree(mem);

	// Adjust the checksum
	yy = ((dwCheck-1 < curCheckSum) ? (dwCheck-1) : dwCheck) - curCheckSum;
	yy = (yy&0xffff) + (yy>>16);
	yy = (yy&0xffff) + (yy>>16);
	yy += GetFileSize(f, 0);

	// Wrtite the checksum
	if (!Seek(f, peOffset+CHECKSUM_OFFSET, FILE_BEGIN)) { return 0; }
	if (!WriteFile(f, &yy, sizeof(DWORD), &nBytes, 0) || nBytes != sizeof(DWORD)) { return 0; }

	// Finish up and return new checksum
	CloseHandle(f);
	*err = FALSE;
	return yy;
}

/**
Reads the current checksum of the PE. Returns the current checksum, or 0 if
failed (0 could techincally be the checksum though). Check err for errors.
The peOffset is also returned for use with UpdatePEChkSum.
**/
DWORD GetCurPEChkSum(LPCTSTR filename, OUT DWORD *peOffset, OUT BOOL *err) {
	IMAGE_DOS_HEADER dosh;
	DWORD checkSum;
	HANDLE f = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	*err = TRUE;
	if (f == INVALID_HANDLE_VALUE) { return 0; }
	if (!Read(f, &dosh, sizeof(IMAGE_DOS_HEADER))) { return 0; }
	if (!Seek(f, dosh.e_lfanew+CHECKSUM_OFFSET, FILE_BEGIN)) { return 0; }
	if (!Read(f, &checkSum, sizeof(DWORD))) { return 0; }
	CloseHandle(f);
	*peOffset = dosh.e_lfanew;
	*err = FALSE;
	return checkSum;
}

/** Main function **/
int _tmain(int argc, _TCHAR *argv[]) {
	int i;
	TCHAR *filename;
	DWORD cur, peOffset, cs;
	BOOL err;

	if (argc <= 1) {
		_tprintf(TEXT("Usage: every command line argument is treated as a file and the checksum is retrieved and updated\n"));
	}
	for (i = 1; i < argc; i++) {
		filename = argv[i];
		cur = GetCurPEChkSum(filename, &peOffset, &err);
		if (err) {
			_ftprintf(stderr, TEXT("%s: Could not open be opened: 0x%08X\n"), filename, GetLastError());
		} else {
			cs = UpdatePEChkSum(filename, cur, peOffset, &err);
			if (err) {
				_ftprintf(stderr, TEXT("%s: Could not open be opened: 0x%08X\n"), filename, GetLastError());
			} else if (cur == cs) {
				_tprintf(TEXT("%s: Checksum did not change from 0x%08X\n"), filename, cur);
			} else {
				_tprintf(TEXT("%s: Checksum updated from 0x%08X to 0x%08X\n"), filename, cur, cs);
			}
		}
	}
	return 0;
}

