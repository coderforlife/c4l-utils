#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

#define DEFAULT_RESOLUTION 1 //ms

BOOL closing = FALSE;
BOOL ctrlHandler(DWORD fdwCtrlType) 
{
	switch (fdwCtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			closing = TRUE;
			return TRUE;

		default:
			return FALSE;
	}
}

int _tmain(int argc, TCHAR *argv[])
{
	TIMECAPS tc;
	UINT     timerRes = DEFAULT_RESOLUTION;

	if (argc > 1)
	{
		timerRes = _ttoi(argv[1]);
		if (timerRes <= 0)
		{
			_ftprintf(stderr, _T("The given timer resolution is invalid (it must be an integral value greater than 0).\n"));
			return -1;
		}
	}

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 
	{
		_ftprintf(stderr, _T("Error while trying to get the timer capabilities.\n"));
		return -1;
	}

	_tprintf(_T("Timer resolution range:   %u - %u ms\n"), tc.wPeriodMin, tc.wPeriodMax);
	_tprintf(_T("Desired timer resolution: %u ms\n"), timerRes);

	if (timerRes < tc.wPeriodMin)
	{
		timerRes = tc.wPeriodMin;
		_tprintf(_T("Timer resolution changed to: %u ms\n"), timerRes);
	}
	else if (timerRes > tc.wPeriodMax)
	{
		timerRes = tc.wPeriodMax;
		_tprintf(_T("Timer resolution changed to: %u ms\n"), timerRes);
	}

	if (timeBeginPeriod(timerRes) != TIMERR_NOERROR)
	{
		_ftprintf(stderr, _T("Error while setting the timer resolution.\n"));
		return -1;
	}

	_tprintf(_T("Timer resolution successfully set!\n"));

	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlHandler, TRUE))
	{
		_tprintf(_T("The timer change will be effective until this program is closed (e.g. CTRL+C)\n"));
		while (!closing)
		{
			Sleep(100);
		}
	}
	else
	{
		_tprintf(_T("The timer change will be effective for 10 seconds\n"));
		Sleep(10000);
	}

	timeEndPeriod(timerRes);
	_tprintf(_T("Timer restored\n"));

	return 0;
}
