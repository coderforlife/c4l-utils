// OpenHiddenSystemDrive: Opens the hidden system drive in an Explorer Window
// Copyright (C) 2010  Jeffrey Bush <jeff@coderforlife.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.

#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include <basetyps.h> // needed for mingw32 for REFIID in shellapi
#include <shellapi.h>

#include <stdlib.h>
#include <tchar.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) sizeof(a)/sizeof(a[0])
#endif

static int error(TCHAR *text, DWORD err) {
	TCHAR error[1024];
	_sntprintf(error, ARRAYSIZE(error), text, err);
	MessageBox(NULL, error, TEXT("Error"), MB_ICONERROR | MB_OK);
	return 1;
}

static int open(TCHAR *path) {
	UINT_PTR retval = (UINT_PTR)ShellExecute(NULL, NULL, path, NULL, path, SW_SHOWDEFAULT);
	return (retval > 32) ? 0 : error(TEXT("Failed to open the Explorer window: %d"), retval);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	HANDLE sh;
	TCHAR vname[MAX_PATH+1], paths[1024], /*label[MAX_PATH+1],*/ fsname[MAX_PATH+1];
	BOOL first = TRUE;
	DWORD len, err;
	ULARGE_INTEGER total;

	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// Get the first volume (handle is used to iterate)
	sh = FindFirstVolume(vname, ARRAYSIZE(vname));
	if (sh == INVALID_HANDLE_VALUE) {
		return error(TEXT("FindFirstVolume failed: %d"), GetLastError());
	}

	// Iterate through all volumes
	while (first || FindNextVolume(sh, vname, ARRAYSIZE(vname))) {
		first = FALSE;
		
		// Make sure the volume path is valid
		len = _tcslen(vname);
		if (len < 5 || _tcsncmp(vname, TEXT("\\\\?\\"), 4) != 0 || vname[len-1] != L'\\')
			continue;

		// Check the drive type (must be FIXED)
		if (GetDriveType(vname) != DRIVE_FIXED)
			continue;

		// Make sure there are no paths (it is hidden)
		if (!GetVolumePathNamesForVolumeName(vname, paths, ARRAYSIZE(paths), &len) || paths[0] != 0)
			continue;

		// Make sure the label is System Reserved and filesystem is NTFS
		if (!GetVolumeInformation(vname, /*label, ARRAYSIZE(label)*/ NULL, 0, NULL, NULL, NULL, fsname, ARRAYSIZE(fsname)) ||
			_tcscmp(fsname, TEXT("NTFS")) != 0) //|| _tcscmp(label, TEXT("System Reserved")) != 0) // the label is translated to the installed locale
			continue;

		// Accept any size less than 1GB
		if (!GetDiskFreeSpaceEx(vname, NULL, &total, NULL) || total.QuadPart > 1073741824LL)
			continue;

		// Found it!
		FindVolumeClose(sh);
		return open(vname);
	}

	// Report and errors and close the handle
	err = GetLastError();
	FindVolumeClose(sh);
	if (err != ERROR_NO_MORE_FILES) {
		return error(TEXT("FindNextVolume failed: %d"), err);
	} else {
		MessageBox(NULL, TEXT("The hidden system drive was not found"), TEXT("Drive Not Found"), MB_ICONWARNING | MB_OK);
	}
	return 1;
}
