#include "util.h"

#include <stdlib.h>

// Copies the part of a that is not in b
// b must be a part of a (in memory, not just content wise)
// b > a must hold true (as pointers)
// a new string is returned that contains a up till b
TCHAR *copyStrPart(TCHAR *a, TCHAR *b) {
	//assert(a < b);
	size_t len = b - a;
	TCHAR *x = _tcsncpy((TCHAR*)malloc((len+5)*sizeof(TCHAR)), a, len);
	x[len] = 0;
	return x;
}

// Copies an entire string
TCHAR *copyStr(const TCHAR *a) {
	size_t len = _tcslen(a);
	TCHAR *x = _tcsncpy((TCHAR*)malloc((len+5)*sizeof(TCHAR)), a, len);
	x[len] = x[len+1] = 0;
	return x;
}

// Returns a new string that is the base directory of path
// The path can use either \ or /
// If it contains neither, a copy of the path is returned
// The new string has some extra space for any necessary modifications (like adding \*)
// The base path does NOT include trailing \ or /
TCHAR *getBaseDirectory(TCHAR *path) {
	TCHAR *a = _tcsrchr(path, L'\\');
	TCHAR *b = _tcsrchr(path, L'/');
	if (a == NULL && b == NULL)
		return copyStr(path);
	else if (a == NULL)
		return copyStrPart(path, b);
	else if (b == NULL)
		return copyStrPart(path, a);
	return copyStrPart(path, a > b ? a : b);
}

// Gets the file name portion from a path (no new string is allocated)
// The path can use either \ or /
// If it contains neither, the path itself is returned 
TCHAR *getFileName(TCHAR *path) {
	TCHAR *a = _tcsrchr(path, L'\\');
	TCHAR *b = _tcsrchr(path, L'/');
	if (a == NULL && b == NULL)
		return path;
	else if (a == NULL)
		return b+1;
	else if (b == NULL)
		return a+1;
	return a > b ? (a+1) : (b+1);
}

// Combines a directory and a file into a full path, stored in dst
TCHAR *makeFullPath(TCHAR *dir, TCHAR *file, TCHAR *dst) {
	size_t dir_len = _tcslen(dir);
	size_t file_len = _tcslen(file);
	_tcsncpy(dst, dir, dir_len);
	dst[dir_len] = L'\\';
	_tcsncpy(dst+dir_len+1, file, file_len);
	dst[dir_len+file_len+1] = 0;
	return dst;
}

// Combines a directory, subdirectory, and a file into a full path, stored in dst
TCHAR *makeFullPath2(TCHAR *dir, TCHAR *subdir, TCHAR *file, TCHAR *dst) {
	size_t dir_len = _tcslen(dir);
	size_t subdir_len = _tcslen(subdir);
	size_t file_len = _tcslen(file);
	_tcsncpy(dst, dir, dir_len);
	dst[dir_len] = L'\\';
	_tcsncpy(dst+dir_len+1, subdir, subdir_len);
	dst[dir_len+subdir_len+1] = L'\\';
	_tcsncpy(dst+dir_len+subdir_len+2, file, file_len);
	dst[dir_len+subdir_len+file_len+2] = 0;
	return dst;
}
