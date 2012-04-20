using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace WindowList
{
    internal static class WinApi
    {
        [DllImport("user32.dll")]
        public static extern IntPtr LoadIcon(IntPtr hInstance, IntPtr lpIconName);
        public static readonly IntPtr IDI_APPLICATION = new IntPtr(32512);

        [DllImport("user32.dll")] [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
        public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr data);

        [DllImport("user32.dll")]
        public static extern IntPtr GetParent(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount); // return is num characters, or 0 for error
        public static String GetClassName(IntPtr hWnd)
        {
            StringBuilder sb = new StringBuilder(1024);
            int n = GetClassName(hWnd, sb, sb.Capacity);
            return (n == 0) ? null : sb.ToString();
        }

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId); // return thread id, 0 for error
        //[DllImport("kernel32.dll")]
        //private static extern IntPtr OpenProcess(uint dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, uint dwProcessId);
        //[DllImport("psapi.dll")]
        //private static extern uint GetModuleFileNameEx(IntPtr hProcess, IntPtr hModule, StringBuilder lpBaseName, int nSize); // return is num characters, or 0 for error
        //[DllImport("kernel32.dll")][return: MarshalAs(UnmanagedType.Bool)]
        //private static extern bool CloseHandle(IntPtr hObject);
        //private const uint PROCESS_VM_READ = 0x00000010;
        //private const uint PROCESS_QUERY_INFORMATION = 0x00000400;
        public static String GetModuleFileName(IntPtr hWnd)
        {
            uint procId, threadId = GetWindowThreadProcessId(hWnd, out procId);
            using (Process p = Process.GetProcessById((int)procId))
            {
                try
                {
                    return p.MainModule.FileName;
                }
                catch (Exception)
                {
                    return null;
                }
            }

            //IntPtr hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, procId);
            //StringBuilder sb = new StringBuilder(1024);
            //uint n = GetModuleFileNameEx(hProcess, IntPtr.Zero, sb, sb.Capacity);
            //CloseHandle(hProcess);
            //return (n == 0) ? null : sb.ToString();
        }

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount); // return is num characters, or 0 for error
        public static String GetWindowText(IntPtr hWnd)
        {
            StringBuilder sb = new StringBuilder(1024);
            int n = GetWindowText(hWnd, sb, sb.Capacity);
            return (n == 0) ? null : sb.ToString();
        }

        [DllImport("user32.dll")][return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetWindowRect(IntPtr hwnd, out RECT lpRect);
        [StructLayout(LayoutKind.Sequential)]
        public struct RECT { public int left, top, right, bottom; }
        public static RECT? GetWindowRect(IntPtr hWnd)
        {
            RECT rect;
            return GetWindowRect(hWnd, out rect) ? (RECT?)rect : null;
        }

        [DllImport("user32.dll")][return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsHungAppWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern IntPtr SendMessage(IntPtr hWnd, UInt32 Msg, IntPtr wParam, IntPtr lParam);
        public const UInt32 WM_GETICON = 0x007F;
        public static readonly IntPtr ICON_SMALL = new IntPtr(0);
        public static readonly IntPtr ICON_BIG = new IntPtr(1);
        public static readonly IntPtr ICON_SMALL2 = new IntPtr(2);

        [DllImport("user32.dll", EntryPoint = "GetClassLong")]
        private static extern uint GetClassLongPtr32(IntPtr hWnd, int nIndex);
        [DllImport("user32.dll", EntryPoint = "GetClassLongPtr")]
        private static extern IntPtr GetClassLongPtr64(IntPtr hWnd, int nIndex);
        public static IntPtr GetClassLongPtr(IntPtr hWnd, int nIndex) { return (IntPtr.Size > 4) ? GetClassLongPtr64(hWnd, nIndex) : new IntPtr(GetClassLongPtr32(hWnd, nIndex)); }
        public const int GCLP_HICON = -14;
        public const int GCLP_HICONSM = -34;

        [DllImport("user32.dll")][return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll")][return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);
    }
}
