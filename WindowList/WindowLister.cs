using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace WindowList
{
    public partial class WindowLister : Form
    {
        private WindowMover mover;
        private ListViewColumnSorter lvwColumnSorter;
        private ImageList imgList;

        public WindowLister()
        {
            InitializeComponent();
            this.Icon = System.Drawing.Icon.FromHandle(WinApi.LoadIcon(Marshal.GetHINSTANCE(Assembly.GetExecutingAssembly().GetModules()[0]), WinApi.IDI_APPLICATION));
            this.list.ListViewItemSorter = lvwColumnSorter = new ListViewColumnSorter();
            this.list.SmallImageList = this.imgList = new ImageList();
        }

        private bool WindowEnum(IntPtr hWnd, IntPtr data)
        {
            if (WinApi.GetParent(hWnd) != IntPtr.Zero) return true;

            WinApi.RECT rect = WinApi.GetWindowRect(hWnd).Value;
            bool hung = WinApi.IsHungAppWindow(hWnd);
            ListViewItem item = new ListViewItem(WinApi.GetWindowText(hWnd));

            if (!hung)
            {
                IntPtr hIcon;
                if (((hIcon = WinApi.SendMessage(hWnd, WinApi.WM_GETICON, WinApi.ICON_SMALL, IntPtr.Zero)) != IntPtr.Zero) ||
                    ((hIcon = WinApi.SendMessage(hWnd, WinApi.WM_GETICON, WinApi.ICON_BIG, IntPtr.Zero)) != IntPtr.Zero) ||
                    ((hIcon = WinApi.SendMessage(hWnd, WinApi.WM_GETICON, WinApi.ICON_SMALL2, IntPtr.Zero)) != IntPtr.Zero) ||
                    ((hIcon = WinApi.GetClassLongPtr(hWnd, WinApi.GCLP_HICONSM)) != IntPtr.Zero) ||
                    ((hIcon = WinApi.GetClassLongPtr(hWnd, WinApi.GCLP_HICON)) != IntPtr.Zero))
                {
                    item.ImageIndex = this.imgList.Images.Count;
                    this.imgList.Images.Add(System.Drawing.Icon.FromHandle(hIcon));
                }
            }

            item.Tag = hWnd;

            item.SubItems.Add(hung.ToString());
            item.SubItems.Add(WinApi.IsWindowVisible(hWnd).ToString());
            item.SubItems.Add(WinApi.GetClassName(hWnd));
            item.SubItems.Add(WinApi.GetModuleFileName(hWnd));
            item.SubItems.Add("(" + rect.left + ", " + rect.top + ")");
            item.SubItems.Add("[" + (rect.right - rect.left) + " x " + (rect.bottom - rect.top) + "]");

            list.Items.Add(item);
            if (list.Items.Count % 50 == 0)
                list.Update();

            return true;
        }

        public void RefreshData()
        {
            this.list.Items.Clear();
            this.imgList.Images.Clear();
            WinApi.EnumWindows(new WinApi.EnumWindowsProc(WindowEnum), IntPtr.Zero);
            this.list.Sort();
        }

        private void refresh_event(object sender, EventArgs e) { RefreshData(); }
        private void list_ColumnClick(object sender, ColumnClickEventArgs e)
        {
            if (e.Column == lvwColumnSorter.SortColumn)
            {
                lvwColumnSorter.Order = lvwColumnSorter.Order == SortOrder.Ascending ? SortOrder.Descending : SortOrder.Ascending;
            }
            else
            {
                lvwColumnSorter.SortColumn = e.Column;
                lvwColumnSorter.Order = SortOrder.Ascending;
            }
            list.Sort();
        }
        private void list_DoubleClick(object sender, EventArgs e)
        {
            if (this.list.SelectedItems.Count == 0)
                return;
            ListViewItem item = this.list.SelectedItems[0];
            if (mover == null)
                mover = new WindowMover();
            mover.SetWindowHandle((IntPtr)item.Tag);
            mover.ShowDialog(this);
        }
    }
}
