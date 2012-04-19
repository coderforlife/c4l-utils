#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <wchar.h>
#include <tchar.h>

#ifndef TEXT
#ifdef _UNICODE
#define TEXT(x) L##x
#else
#define TEXT(x) x
#endif
#endif

#define RETURN_USAGE			-20
#define RETURN_FILE_1_FAIL		-11
#define RETURN_FILE_2_FAIL		-12
#define RETURN_FILE_1_FAIL_SEEK	-13
#define RETURN_FILE_2_FAIL_SEEK	-14
#define RETURN_IDENTICAL		0
#define RETURN_FILE_1_SHORTER	-1
#define RETURN_FILE_2_SHORTER	-2

inline static int isOffsetArg(_TCHAR* arg) {
	return ((arg[0] == TEXT('/') || arg[0] == TEXT('-')) && _tcscmp(arg+1, TEXT("offset")) == 0);
}

inline static int isNumericalArg(_TCHAR* arg) {
	while (*arg != 0) { if (!_istdigit(*arg)) { return 0; } ++arg; }
	return 1;
}

inline static int decimalWidth(unsigned long x) {
	int width = 0;
	while (x > 0) { x /= 10; ++width; }
	return width;
}

#include "mingw-unicode.c"
// Compares two files byte wise
// Returns one of the RETURN_* constants or, if a difference is found, a positive value indicating the one more than byte position of the difference found.
int _tmain(int argc, _TCHAR* argv[]) {
	TCHAR *fileName1, *fileName2;
	long off1 = 0, off2 = 0;
	FILE *file1, *file2;
	unsigned long x = 0;
	int i = -1, a, b, retval;
	
	if (argc >= 3 || argc <= 7) {
		i = 1;
		fileName1 = argv[i++];
		if (i+3 <= argc && isOffsetArg(argv[i]) && isNumericalArg(argv[i+1])) {
			off1 = _ttoi(argv[i+1]);
			i += 2;
		}
		fileName2 = argv[i++];
		if (i+2 <= argc && isOffsetArg(argv[i]) && isNumericalArg(argv[i+1])) {
			off2 = _ttoi(argv[i+1]);
			i += 2;
		}
	}
	
	if (i != argc) {
		_tprintf(TEXT("Usage: %s file1 [/offset x] file2 [/offset x]\n"), argv[0]);
		return RETURN_USAGE;
	}

	// Open the files
	file1 = _tfopen(fileName1, TEXT("rb"));
	if (!file1) {
		_tprintf(TEXT("Could not open file 1: '%s'\n"), fileName1);
		return RETURN_FILE_1_FAIL;
	}
	file2 = _tfopen(fileName2, TEXT("rb"));
	if (!file2) {
		_tprintf(TEXT("Could not open file 2: '%s'\n"), fileName2);
		fclose(file1);
		return RETURN_FILE_2_FAIL;
	}

	// Seek to the offsets
	if (off1 != 0) {
		if (fseek(file1, off1, SEEK_SET)) {
			_tprintf(TEXT("Could not open seek in file 1: '%s' %ld\n"), fileName1, off1);
			fclose(file1); fclose(file2);
			return RETURN_FILE_1_FAIL_SEEK;
		}
	}
	if (off2 != 0) {
		if (fseek(file2, off2, SEEK_SET)) {
			_tprintf(TEXT("Could not open seek in file 2: '%s' %ld\n"), fileName2, off2);
			fclose(file1); fclose(file2);
			return RETURN_FILE_2_FAIL_SEEK;
		}
	}


	// Get the first characters
	a = fgetc(file1);
	b = fgetc(file2);

	// Loop through till the end of one of the files
	while (!feof(file1) && !feof(file2)) {
		if (a != b) {
			int width1 = decimalWidth(x + off1), width2 = decimalWidth(x + off2), width = width1 > width2 ? width1 : width2;
			printf("Difference found at byte %u (#%08x) from the offsets:\n", x, x);
			printf("File 1: '%c' %3d (#%02x)  Position: %*u (#%08x)\n", (char)a, a, a, width, x + off1, x + off1);
			printf("File 2: '%c' %3d (#%02x)  Position: %*u (#%08x)\n", (char)b, b, b, width, x + off2, x + off2);
			break;
		}
		// Get next character
		x++;
		a = fgetc(file1);
		b = fgetc(file2);
	}
	
	// Report non-difference messages
	if (feof(file1) && feof(file2)) {
		_tprintf(TEXT("The files are the same\n"));
		retval = RETURN_IDENTICAL;
	} else if (feof(file1) && !feof(file2)) {
		_tprintf(TEXT("'%s' is longer, but up to the end of '%s' they were identical\n"), fileName2, fileName1);
		retval = RETURN_FILE_1_SHORTER;
	} else if (!feof(file1) && feof(file2)) {
		_tprintf(TEXT("'%s' is longer, but up to the end of '%s' they were identical\n"), fileName1, fileName2);
		retval = RETURN_FILE_2_SHORTER;
	} else {
		retval = x + 1;
	}

	// Close files
	fclose(file1);
	fclose(file2);

	return retval;
}
