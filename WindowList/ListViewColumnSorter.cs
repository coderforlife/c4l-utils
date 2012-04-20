using System;
using System.Collections;
using System.Windows.Forms;

namespace WindowList
{
    class ListViewColumnSorter : IComparer
    {
        private CaseInsensitiveComparer ObjectCompare;
        public int SortColumn;
        public SortOrder Order;
        public ListViewColumnSorter()
        {
            SortColumn = 0;
            Order = SortOrder.None;
            ObjectCompare = new CaseInsensitiveComparer();
        }
        public virtual int Compare(Object x, Object y)
        {
            ListViewItem listviewX = (ListViewItem)x;
            ListViewItem listviewY = (ListViewItem)y;
            int compareResult = ObjectCompare.Compare(listviewX.SubItems[SortColumn].Text, listviewY.SubItems[SortColumn].Text);
            if (Order == SortOrder.Ascending)
                return compareResult;
            else if (Order == SortOrder.Descending)
                return -compareResult;
            else
                return 0;
        }
    }
}
