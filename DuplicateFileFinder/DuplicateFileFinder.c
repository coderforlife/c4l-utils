#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows XP.
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of Windows.
#endif

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "md5.h"

#include "pair.h"
#include "vector.h"
#include "map.h"

#define MAX_STR_LEN 32767

// Default search paths
TCHAR *folders_to_search_def[] = {
	TEXT("%USERPROFILE%\\Pictures"),
	TEXT("%USERPROFILE%\\Desktop"),
	TEXT("%USERPROFILE%\\Documents"),
	TEXT("%USERPROFILE%\\Downloads"),
	NULL
};

// Actual search paths (filled in later)
TCHAR **folders_to_search;

map *m; // files found, key: LONGLONG, value: vector<pair<TCHAR*,md5_value*> >
int file_count = 0; // number of files found
int dir_count = 0; // number of directories found
md5_value blank; // a blank md5 value used for errors/placeholders

#define NAME(X) ((TCHAR*)((pair*)X)->a)
#define HASH(X) ((md5_value*)((pair*)X)->b)

int longlongCmp(LONGLONG *a, LONGLONG *b) {
	return (*a < *b) ? -1 : ((*a == *b) ? 0: 1);
}

// Allocates memory for a string of maximum length l and fills the memory with 0s
#define MAKE_STRING(l) (TCHAR*)memset(malloc(sizeof(TCHAR)*l), 0, sizeof(TCHAR)*l)

// Checks if two md5_values are equal
#define HASH_EQ(a, b) (a.word64s[0] == b->word64s[0] && a.word64s[1] == b->word64s[1])

TCHAR fileSizeTemp[1024];

void *file_vector_clean(vector *v) {
	unsigned int i;
	for (i = 0; i < v->length; i++) {
		free(((pair*)v->x[i])->a);
		free(((pair*)v->x[i])->b);
	}
	return v;
}

// Copies an entire string
TCHAR *copyStr(const TCHAR *a) {
	DWORD len = _tcslen(a);
	TCHAR *x = _tcsncpy((TCHAR*)malloc((len+5)*sizeof(TCHAR)), a, len);
	x[len] = x[len+1] = 0;
	return x;
}

// Gets the file size in a nicely formatted string
// This uses the same memory each time so the result must be used right away
TCHAR *fileSize(LONGLONG size) {
	if (size < 1024) {
		_sntprintf(fileSizeTemp, 1024, TEXT("%I64d bytes"), size);
	} else if (size < 1024*1024) {
		_sntprintf(fileSizeTemp, 1024, TEXT("%.2lf KB"), size / 1024.0);
	} else if (size < 1024*1024*1024) {
		_sntprintf(fileSizeTemp, 1024, TEXT("%.2lf MB"), size / 1024 / 1024.0);
	} else {
		_sntprintf(fileSizeTemp, 1024, TEXT("%.2lf GB"), size / 1024 / 1024 / 1024.0);
	}
	return fileSizeTemp;
}

// Adds the name of a file to the parent directory it is in
// The parent directory is actually the wildcard search path so it ends in "\*"
TCHAR *makeFullPath(TCHAR *dir, TCHAR *sub) {
	TCHAR *path = MAKE_STRING(MAX_STR_LEN);
	int len = _tcslen(dir);
	_tcsncpy(path, dir, len-1); // don't copy the *
	_tcsncpy(path+len-1, sub, _tcslen(sub));
	return path;
}

void addToMap(LONGLONG size, TCHAR *file, md5_value md5) {
	pair *p = map_get(m, &size); //pair<LONGONG, vector<pair<TCHAR*,md5_value*> > >
	vector *v;
	md5_value *x = malloc(sizeof(md5_value));
	if (!p) {
		LONGLONG *t = malloc(sizeof(LONGLONG));
		*t = size;
		v = vector_create(64);
		map_set(m, t, v);
	} else {
		v = (vector*)p->b;
	}
	*x = md5;
	vector_append(v, create_pair(copyStr(file), x));
}

// Recursively finds all files in the path given
// The given path string MUST have room for 3 more characters
void findFiles(TCHAR *path) {
	LARGE_INTEGER filesize;
	WIN32_FIND_DATA ffd;
	HANDLE hFind;
	TCHAR *full;
	DWORD dwError;

	// convert path into a wildcard search string
	int len = _tcslen(path);
	path[len] = L'\\';
	path[len+1] = L'*';
	path[len+2] = 0;

	// increment the number of directories found
	dir_count++;

	// find all files in the directory
	hFind = FindFirstFile(path, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("FindFirstFile failed: %s (%d)\n"), path, GetLastError());
		return;
	}
	do {
		// ignore the current / parent directory 'files'
		if (_tcscmp(ffd.cFileName, TEXT("..")) == 0 || _tcscmp(ffd.cFileName, TEXT(".")) == 0) {
			continue;
		}
		full = makeFullPath(path, ffd.cFileName);
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// recurse into the directory
			findFiles(full);
		} else {
			// save the file information in the map
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			addToMap(filesize.QuadPart, full, blank);
			// increment the number of files found, and display a status if a multiple of 2000
			if ((++file_count) % 2000 == 0) {
				_tprintf(TEXT("\tFound %d files so far...\n"), file_count);
			}
		}
		free(full);
	}
	while (FindNextFile(hFind, &ffd) != 0);

	// report any errors and close the handle
	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		_tprintf(TEXT("FindNextFile failed (%d)\n"), dwError);
	}
	FindClose(hFind);
}

int removeSingleFiles(LONGLONG *size, vector *files, map *m2) {
	if (files->length <= 1) {
		vector_destroy(file_vector_clean(files), TRUE);
		file_count--;
	} else {
		map_set(m2, size, files);
	}
	return 0;
}


// Calculates the MD5 hash of a file with the given size
// This is not the complete hash, instead it only covers the first and last 256
// bytes (or all bytes if size <=512). This minimal hash is much faster to
// compute for large files, and does not lose much of its ability to determine
// if two files are different.
md5_value md5hash(TCHAR *filename, LONGLONG size) {
	BYTE buffer[512];
	int n = (int)(size < 512 ? size : 256);
	md5_state_t md5;
	FILE *f = _tfopen(filename, TEXT("r"));

	if (!f) { return blank; }
	md5_init(&md5);
	n = fread(buffer, sizeof(BYTE), n, f);
	md5_append(&md5, buffer, n);
	if (size >= 512) {
		fseek(f, -256, SEEK_END);
		n = fread(buffer, sizeof(BYTE), 256, f);
		md5_append(&md5, buffer, n);
	}
	fclose(f);
	return md5_finish(&md5);
}

int calcFileHashes(LONGLONG *size, vector *files, unsigned int *_nFiles) {
	unsigned int i, nFiles = *_nFiles;
	for (i = 0; i < files->length; i++) {
		*HASH(files->x[i]) = md5hash(NAME(files->x[i]), *size);
		if ((++nFiles) % 1000 == 0) {
			_tprintf(TEXT("\tCalculated %d hashes so far (%d%%)...\n"), nFiles, nFiles * 100 / file_count);
		}
	}
	*_nFiles = nFiles;
	return 0;
}


#define IDENTICAL	1
#define DIFFERENT	0
#define FILE_ERR_1	-1
#define FILE_ERR_2	-2
#define FILE_ERR_12	-3

// Check if two files are identical given their filenames.
// This returns one of the above defines. A return value <0 indicates error.
int areIdenticalFiles(TCHAR *a, TCHAR *b) {
	int c, d;
	FILE *f = _tfopen(a, TEXT("r"));
	FILE *g = _tfopen(b, TEXT("r"));
	if (!f || !g) {
		if (!f) { _tprintf(TEXT("\t***ERROR: Failed to open file: %s\n"), a); } else { fclose(f); }
		if (!g) { _tprintf(TEXT("\t***ERROR: Failed to open file: %s\n"), b); } else { fclose(g); }
		return !f ? (!g ? FILE_ERR_12 : FILE_ERR_1) : FILE_ERR_2;
	}
	do {
		c = fgetc(f);
		d = fgetc(g);
    } while (c == d && c != EOF && d != EOF);
	fclose(f);
	fclose(g);
	return c == d ? IDENTICAL : DIFFERENT;
}

struct params {
	FILE *file;
	unsigned int nFiles;
	unsigned int lastHundred;
	unsigned int filesDiscarded;
	LONGLONG size;
	vector *t; //temporary storage
};

int findIdenticalFiles(LONGLONG *size, vector *v, struct params *x) {
	unsigned int i;
	while (v->length > 1) {
		pair *p = vector_delete(v, v->length-1);
		TCHAR *cur = NAME(p);
		md5_value cur_hash = *HASH(p);
		free(HASH(p));
		free(p);
		vector_append(x->t, cur);
		// go through each file of the same size looking for identical hashes, then check actual identical-ness
		for (i = 0; i < v->length;) {
			int retval;
			if (!HASH_EQ(cur_hash, HASH(v->x[i]))) {
				i++;
				continue;
			}
			retval = areIdenticalFiles(cur, NAME(v->x[i]));
			if (retval == IDENTICAL) {
				pair *p = vector_delete(v, i);
				vector_append(x->t, NAME(p));
				free(HASH(p));
				free(p);
			} else if (retval == DIFFERENT) {
				i++;
			} else {
				if (retval == FILE_ERR_2 || retval == FILE_ERR_12) {
					pair *p = vector_delete(v, i);
					free(NAME(p));
					free(HASH(p));
					free(p);
				}
				if (retval == FILE_ERR_1 || retval == FILE_ERR_12) {
					break;
				}
			}
		}
		// found duplicates!
		if (x->t->length > 1) {
			x->nFiles += x->t->length;
			if (x->nFiles / 100 > x->lastHundred) {
				x->lastHundred = x->nFiles / 100;
				_tprintf(TEXT("\tFound %d00 duplicate files and discared %d files so far (%d%%)...\n"), x->lastHundred, x->filesDiscarded, (x->filesDiscarded+x->lastHundred*100)*100 / file_count);
			}
			qsort(x->t->x, x->t->length, sizeof(TCHAR*), (compare_func)&_tcscmp);
			// save the list of duplicates found
			_ftprintf(x->file, TEXT("File Size: %s\n"), fileSize(*size));
			for (i = 0; i < x->t->length; i++) {
				_ftprintf(x->file, TEXT("%s\n"), x->t->x[i]);
			}
			_ftprintf(x->file, TEXT("\n"));
			x->size += (*size) * (x->t->length - 1);
		} else {
			x->filesDiscarded++;
		}
		vector_clear(x->t, TRUE);
	}
	if (v->length == 1) {
		x->filesDiscarded++;
	}
	return 0;
}

int _tmain(int argc, _TCHAR* argv[]) {
	unsigned int i, nFiles = 0;
	LONGLONG size = 0;
	pair *p;
	FILE *f;
	map *m2;
	vector *empty_files;
	struct params x;

	// setup blank md5_value variable
	blank.word64s[0] = 0;
	blank.word64s[1] = 0;

	// create file map
	m = map_create((compare_func)&longlongCmp);

	// Setup folder search paths
	if (argc > 1) {
		// files were given on the command line, use them
		folders_to_search = malloc(sizeof(TCHAR*)*argc);
		for (i = 1; i < (unsigned int)argc; i++)
			folders_to_search[i-1] = argv[i];
		folders_to_search[argc-1] = NULL;
	} else {
		// use the default folders
		folders_to_search = folders_to_search_def;
	}

	// expand all environmental strings, copy, and save path names
	_tprintf(TEXT("Will search for duplicate files in:\n"));
	for (i = 0; folders_to_search[i]; i++) {
		TCHAR *buf = MAKE_STRING(MAX_STR_LEN);
		ExpandEnvironmentStrings(folders_to_search[i], buf, MAX_STR_LEN);
		folders_to_search[i] = buf;
		_tprintf(TEXT("\t%s\n"), buf);
	}
	// call the recursive find function on all paths
	_tprintf(TEXT("\nCompiling list of all files...\n"));
	for (i = 0; folders_to_search[i]; i++) {
		findFiles(folders_to_search[i]);
		free(folders_to_search[i]);
	}
	_tprintf(TEXT("\tFound %d files in %d directories\n"), file_count, dir_count);

	// go through the map and find every case where there is only one file with a particular size
	_tprintf(TEXT("\nElimating all files with no possible identical pair...\n"));
	m2 = map_create((compare_func)&longlongCmp); // there is likely a better way than copying each good entry over to a new map
	map_traverse(m, INORDER, (traverse_cb)&removeSingleFiles, m2);
	//map_destroy(m, TRUE);
	m = m2;
	_tprintf(TEXT("\t%d potential duplicates remain\n"), file_count);

	// calculate file hashes for all remaining files
	_tprintf(TEXT("\nCalculating file hashes...\n"));
	map_traverse(m, INORDER, (traverse_cb)&calcFileHashes, &nFiles);
	_tprintf(TEXT("\tDone calculating file hashes\n"));

	_tprintf(TEXT("\nLooking for duplicate files...\n"));
	f = _tfopen(TEXT("duplicates.txt"), TEXT("w"));

	// report empty files
	p = map_remove(m, &size);
	if (p) {
		empty_files = p->b; // get and remove the 0 sized files
		if (empty_files && empty_files->length) {
			_ftprintf(f, TEXT("Empty Files:\n"));
			for (i = 0; i < empty_files->length; i++) {
				_ftprintf(f, TEXT("%s\n"), NAME(empty_files->x[i]));
			}
			_ftprintf(f, TEXT("\n"));
		}
		vector_destroy(file_vector_clean(empty_files), TRUE);
	}

	x.file = f;
	x.nFiles = x.lastHundred = x.filesDiscarded = 0;
	x.size = 0;
	x.t = vector_create(64);
	map_traverse(m, INORDER, (traverse_cb)&findIdenticalFiles, &x);
	vector_destroy(x.t, TRUE);
	_tprintf(TEXT("\tFound %d duplicate files wasting a total of %s\n"), x.nFiles, fileSize(x.size));

	fclose(f);
	map_destroy(m, TRUE);
	return 0;
}

