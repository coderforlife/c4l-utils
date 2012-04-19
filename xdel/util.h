#ifndef _UTIL_H_
#define _UTIL_H_

#include <tchar.h>

// Copies the part of a that is not in b
// b must be a part of a (in memory, not just content wise)
// b > a must hold true (as pointers)
// a new string is returned that contains a up till b
TCHAR *copyStrPart(TCHAR *a, TCHAR *b);

// Copies an entire string
TCHAR *copyStr(const TCHAR *a);

// Returns a new string that is the base directory of path
// The path can use either \ or /
// If it contains neither, a copy of the path is returned
// The new string has some extra space for any necessary modifications (like adding \*)
// The base path does NOT include trailing \ or /
TCHAR *getBaseDirectory(TCHAR *path);

// Gets the file name portion from a path (no new string is allocated)
// The path can use either \ or /
// If it contains neither, the path itself is returned 
TCHAR *getFileName(TCHAR *path);

// Combines a directory and a file into a full path, stored in dst
TCHAR *makeFullPath(TCHAR *dir, TCHAR *file, TCHAR *dst);

// Combines a directory, subdirectory, and a file into a full path, stored in dst
TCHAR *makeFullPath2(TCHAR *dir, TCHAR *subdir, TCHAR *file, TCHAR *dst);

#endif
