namespace WindowList
{
    partial class WindowMover
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

        private System.Windows.Forms.NumericUpDown x;
        private System.Windows.Forms.NumericUpDown y;
        private System.Windows.Forms.NumericUpDown w;
        private System.Windows.Forms.NumericUpDown h;


        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.Label label1;
            System.Windows.Forms.Label label2;
            System.Windows.Forms.Label label3;
            System.Windows.Forms.Label label4;
            System.Windows.Forms.Button butSet;
            this.x = new System.Windows.Forms.NumericUpDown();
            this.y = new System.Windows.Forms.NumericUpDown();
            this.w = new System.Windows.Forms.NumericUpDown();
            this.h = new System.Windows.Forms.NumericUpDown();
            label1 = new System.Windows.Forms.Label();
            label2 = new System.Windows.Forms.Label();
            label3 = new System.Windows.Forms.Label();
            label4 = new System.Windows.Forms.Label();
            butSet = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.x)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.y)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.w)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.h)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 14);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(47, 13);
            label1.TabIndex = 0;
            label1.Text = "Position:";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new System.Drawing.Point(12, 40);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(30, 13);
            label2.TabIndex = 1;
            label2.Text = "Size:";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new System.Drawing.Point(131, 14);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(10, 13);
            label3.TabIndex = 6;
            label3.Text = ",";
            // 
            // label4
            // 
            label4.AutoSize = true;
            label4.Location = new System.Drawing.Point(131, 40);
            label4.Name = "label4";
            label4.Size = new System.Drawing.Size(12, 13);
            label4.TabIndex = 7;
            label4.Text = "x";
            // 
            // butSet
            // 
            butSet.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            butSet.Location = new System.Drawing.Point(138, 64);
            butSet.Name = "butSet";
            butSet.Size = new System.Drawing.Size(75, 23);
            butSet.TabIndex = 8;
            butSet.Text = "Set";
            butSet.UseVisualStyleBackColor = true;
            butSet.Click += new System.EventHandler(this.butSet_Click);
            // 
            // x
            // 
            this.x.Location = new System.Drawing.Point(65, 12);
            this.x.Maximum = new decimal(new int[] {
            66000,
            0,
            0,
            0});
            this.x.Minimum = new decimal(new int[] {
            66000,
            0,
            0,
            -2147483648});
            this.x.Name = "x";
            this.x.Size = new System.Drawing.Size(60, 20);
            this.x.TabIndex = 2;
            this.x.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // y
            // 
            this.y.Location = new System.Drawing.Point(149, 12);
            this.y.Maximum = new decimal(new int[] {
            66000,
            0,
            0,
            0});
            this.y.Minimum = new decimal(new int[] {
            66000,
            0,
            0,
            -2147483648});
            this.y.Name = "y";
            this.y.Size = new System.Drawing.Size(60, 20);
            this.y.TabIndex = 3;
            this.y.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // w
            // 
            this.w.Location = new System.Drawing.Point(65, 38);
            this.w.Maximum = new decimal(new int[] {
            66000,
            0,
            0,
            0});
            this.w.Minimum = new decimal(new int[] {
            66000,
            0,
            0,
            -2147483648});
            this.w.Name = "w";
            this.w.Size = new System.Drawing.Size(60, 20);
            this.w.TabIndex = 4;
            this.w.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // h
            // 
            this.h.Location = new System.Drawing.Point(149, 38);
            this.h.Maximum = new decimal(new int[] {
            66000,
            0,
            0,
            0});
            this.h.Minimum = new decimal(new int[] {
            66000,
            0,
            0,
            -2147483648});
            this.h.Name = "h";
            this.h.Size = new System.Drawing.Size(60, 20);
            this.h.TabIndex = 5;
            this.h.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // WindowMover
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(225, 99);
            this.Controls.Add(butSet);
            this.Controls.Add(label4);
            this.Controls.Add(label3);
            this.Controls.Add(this.h);
            this.Controls.Add(this.w);
            this.Controls.Add(this.y);
            this.Controls.Add(this.x);
            this.Controls.Add(label2);
            this.Controls.Add(label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "WindowMover";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.Text = "Move Window";
            ((System.ComponentModel.ISupportInitialize)(this.x)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.y)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.w)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.h)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

    }
}