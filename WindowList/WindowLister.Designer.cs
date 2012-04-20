namespace WindowList
{
    partial class WindowLister
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        private System.Windows.Forms.ListView list;

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.Button butRefresh;
            System.Windows.Forms.ColumnHeader colName;
            System.Windows.Forms.ColumnHeader colHung;
            System.Windows.Forms.ColumnHeader colVisible;
            System.Windows.Forms.ColumnHeader colClassName;
            System.Windows.Forms.ColumnHeader colFileName;
            System.Windows.Forms.ColumnHeader colPosition;
            System.Windows.Forms.ColumnHeader colSize;
            this.list = new System.Windows.Forms.ListView();
            butRefresh = new System.Windows.Forms.Button();
            colName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            colHung = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            colVisible = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            colClassName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            colFileName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            colPosition = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            colSize = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.SuspendLayout();
            // 
            // butRefresh
            // 
            butRefresh.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            butRefresh.Location = new System.Drawing.Point(743, 440);
            butRefresh.Name = "butRefresh";
            butRefresh.Size = new System.Drawing.Size(84, 28);
            butRefresh.TabIndex = 1;
            butRefresh.Text = "Refresh";
            butRefresh.UseVisualStyleBackColor = true;
            butRefresh.Click += new System.EventHandler(this.refresh_event);
            // 
            // colName
            // 
            colName.Text = "Name";
            colName.Width = 150;
            // 
            // colHung
            // 
            colHung.Text = "Hung";
            colHung.Width = 40;
            // 
            // colVisible
            // 
            colVisible.Text = "Visible";
            colVisible.Width = 40;
            // 
            // colClassName
            // 
            colClassName.Text = "Class Name";
            colClassName.Width = 150;
            // 
            // colFileName
            // 
            colFileName.Text = "File Name";
            colFileName.Width = 250;
            // 
            // colPosition
            // 
            colPosition.Text = "Position";
            colPosition.Width = 80;
            // 
            // colSize
            // 
            colSize.Text = "Size";
            colSize.Width = 80;
            // 
            // list
            // 
            this.list.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            colName,
            colHung,
            colVisible,
            colClassName,
            colFileName,
            colPosition,
            colSize});
            this.list.Location = new System.Drawing.Point(12, 12);
            this.list.Name = "list";
            this.list.Size = new System.Drawing.Size(815, 422);
            this.list.TabIndex = 0;
            this.list.UseCompatibleStateImageBehavior = false;
            this.list.View = System.Windows.Forms.View.Details;
            this.list.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.list_ColumnClick);
            this.list.DoubleClick += new System.EventHandler(this.list_DoubleClick);
            // 
            // WindowLister
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(839, 480);
            this.Controls.Add(this.list);
            this.Controls.Add(butRefresh);
            this.Name = "WindowLister";
            this.Text = "Application Window List";
            this.TopMost = true;
            this.Load += new System.EventHandler(this.refresh_event);
            this.ResumeLayout(false);

        }

        #endregion
    }
}

