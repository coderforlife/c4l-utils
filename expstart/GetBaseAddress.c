#include "GetBaseAddress.h"
#include "Memory.h"

#include <tchar.h>

/* CONTEXT cntxt;
PVOID address;
cntxt.ContextFlags = CONTEXT_INTEGER;
GetThreadContext(pi.hThread, &cntxt);
_tprintf(TEXT("EBX: %X\n"), cntxt.Ebx);
address = (LPVOID)cntxt.Ebx; */

typedef struct _PEB32 {
	BYTE	Reserved1[2];
	BYTE	BeingDebugged;
	BYTE	Reserved2[1];
//	PVOID	Reserved3[2];
	PVOID	Reserved3;
	PVOID	ImageBaseAddress; // added, not in offical docs
	PVOID	LoaderData; /* wish I could use this instead, however it is not available. PPEB_LDR_DATA */
	PVOID	ProcessParameters; /*PRTL_USER_PROCESS_PARAMETERS*/
	BYTE	Reserved4[104];
	PVOID	Reserved5[52];
	PVOID	PostProcessInitRoutine; /*PPS_POST_PROCESS_INIT_ROUTINE*/
	BYTE	Reserved6[128];
	PVOID	Reserved7[1];
	ULONG	SessionId;
} PEB32, *PPEB32;

typedef struct _PEB64 {
	BYTE	Reserved1[2];
	BYTE	BeingDebugged;
//	BYTE	Reserved2[21];
	BYTE	Reserved2[13];
	PVOID	ImageBaseAddress; // added, not in offical docs
	PVOID	LoaderData; /* wish I could use this instead, however it is not available. PPEB_LDR_DATA */
	PVOID	ProcessParameters; /*PRTL_USER_PROCESS_PARAMETERS*/
	BYTE	Reserved3[520];
	PVOID	PostProcessInitRoutine; /*PPS_POST_PROCESS_INIT_ROUTINE*/
	BYTE	Reserved4[136];
	ULONG	SessionId;
} PEB64, *PPEB64;

#ifdef _WIN64
typedef PEB64 PEB;
typedef PPEB64 PPEB;
#else
typedef PEB32 PEB;
typedef PPEB32 PPEB;
#endif

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef LONG (WINAPI *NTQIP)(HANDLE, INT, PVOID, ULONG, PULONG);

PVOID GetBaseAddress(HANDLE proc) {
	PROCESS_BASIC_INFORMATION pbi;
	HMODULE lib = GetModuleHandle(_T("ntdll.dll"));
	NTQIP NtQueryInformationProcess;
	PEB peb;
	//pbi.PebBaseAddress = (PEB*)0x7ffdf000;
	if (!lib) { return NULL; }
	if (!(NtQueryInformationProcess = (NTQIP)GetProcAddress(lib, "NtQueryInformationProcess"))) { FreeLibrary(lib); return NULL; }
	NtQueryInformationProcess(proc, 0, &pbi, sizeof(pbi), NULL);
	FreeLibrary(lib);
	return ReadMem(proc, &peb, sizeof(PEB), pbi.PebBaseAddress) ? peb.ImageBaseAddress : NULL;
}
