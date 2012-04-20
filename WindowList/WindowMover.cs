using System;
using System.Windows.Forms;

namespace WindowList
{
    public partial class WindowMover : Form
    {
        private IntPtr hWnd;

        public WindowMover()
        {
            InitializeComponent();
        }

        public void SetWindowHandle(IntPtr hWnd)
        {
            this.hWnd = hWnd;

            WinApi.RECT rect = WinApi.GetWindowRect(hWnd).Value;

            this.x.Value = rect.left;
            this.y.Value = rect.top;
            this.w.Value = rect.right - rect.left;
            this.h.Value = rect.bottom - rect.top;
        }

        private void butSet_Click(object sender, EventArgs e)
        {
            WinApi.MoveWindow(this.hWnd, (int)this.x.Value, (int)this.y.Value, (int)this.w.Value, (int)this.h.Value, true);
        }
    }
}
