// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealGameSync
{
	partial class SyncFilter
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

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SyncFilter));
			this.OkButton = new System.Windows.Forms.Button();
			this.CancButton = new System.Windows.Forms.Button();
			this.FilterTextGlobal = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.TabControl = new System.Windows.Forms.TabControl();
			this.TabGlobal = new System.Windows.Forms.TabPage();
			this.TabWorkspace = new System.Windows.Forms.TabPage();
			this.FilterTextWorkspace = new System.Windows.Forms.TextBox();
			this.TabControl.SuspendLayout();
			this.TabGlobal.SuspendLayout();
			this.TabWorkspace.SuspendLayout();
			this.SuspendLayout();
			// 
			// OkButton
			// 
			this.OkButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.OkButton.Location = new System.Drawing.Point(888, 450);
			this.OkButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.OkButton.Name = "OkButton";
			this.OkButton.Size = new System.Drawing.Size(75, 22);
			this.OkButton.TabIndex = 2;
			this.OkButton.Text = "Ok";
			this.OkButton.UseVisualStyleBackColor = true;
			this.OkButton.Click += new System.EventHandler(this.OkButton_Click);
			// 
			// CancButton
			// 
			this.CancButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.CancButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.CancButton.Location = new System.Drawing.Point(807, 450);
			this.CancButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.CancButton.Name = "CancButton";
			this.CancButton.Size = new System.Drawing.Size(75, 22);
			this.CancButton.TabIndex = 3;
			this.CancButton.Text = "Cancel";
			this.CancButton.UseVisualStyleBackColor = true;
			this.CancButton.Click += new System.EventHandler(this.CancButton_Click);
			// 
			// FilterTextGlobal
			// 
			this.FilterTextGlobal.AcceptsReturn = true;
			this.FilterTextGlobal.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.FilterTextGlobal.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.FilterTextGlobal.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.FilterTextGlobal.Location = new System.Drawing.Point(0, 7);
			this.FilterTextGlobal.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.FilterTextGlobal.Multiline = true;
			this.FilterTextGlobal.Name = "FilterTextGlobal";
			this.FilterTextGlobal.Size = new System.Drawing.Size(940, 361);
			this.FilterTextGlobal.TabIndex = 1;
			this.FilterTextGlobal.WordWrap = false;
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 16);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(490, 13);
			this.label1.TabIndex = 4;
			this.label1.Text = "The files to be synced from Perforce will be filtered by the rules below. All fil" +
    "es will be synced by default.";
			// 
			// TabControl
			// 
			this.TabControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.TabControl.Controls.Add(this.TabGlobal);
			this.TabControl.Controls.Add(this.TabWorkspace);
			this.TabControl.Location = new System.Drawing.Point(15, 42);
			this.TabControl.Name = "TabControl";
			this.TabControl.SelectedIndex = 0;
			this.TabControl.Size = new System.Drawing.Size(948, 398);
			this.TabControl.TabIndex = 5;
			// 
			// TabGlobal
			// 
			this.TabGlobal.Controls.Add(this.FilterTextGlobal);
			this.TabGlobal.Location = new System.Drawing.Point(4, 22);
			this.TabGlobal.Name = "TabGlobal";
			this.TabGlobal.Padding = new System.Windows.Forms.Padding(3);
			this.TabGlobal.Size = new System.Drawing.Size(940, 372);
			this.TabGlobal.TabIndex = 0;
			this.TabGlobal.Text = "Global";
			this.TabGlobal.UseVisualStyleBackColor = true;
			// 
			// TabWorkspace
			// 
			this.TabWorkspace.Controls.Add(this.FilterTextWorkspace);
			this.TabWorkspace.Location = new System.Drawing.Point(4, 22);
			this.TabWorkspace.Name = "TabWorkspace";
			this.TabWorkspace.Padding = new System.Windows.Forms.Padding(3);
			this.TabWorkspace.Size = new System.Drawing.Size(940, 372);
			this.TabWorkspace.TabIndex = 1;
			this.TabWorkspace.Text = "Workspace";
			this.TabWorkspace.UseVisualStyleBackColor = true;
			// 
			// FilterTextWorkspace
			// 
			this.FilterTextWorkspace.AcceptsReturn = true;
			this.FilterTextWorkspace.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.FilterTextWorkspace.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.FilterTextWorkspace.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.FilterTextWorkspace.Location = new System.Drawing.Point(0, 7);
			this.FilterTextWorkspace.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.FilterTextWorkspace.Multiline = true;
			this.FilterTextWorkspace.Name = "FilterTextWorkspace";
			this.FilterTextWorkspace.Size = new System.Drawing.Size(940, 361);
			this.FilterTextWorkspace.TabIndex = 2;
			this.FilterTextWorkspace.WordWrap = false;
			// 
			// SyncFilter
			// 
			this.AcceptButton = this.OkButton;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.CancButton;
			this.ClientSize = new System.Drawing.Size(975, 485);
			this.Controls.Add(this.TabControl);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.CancButton);
			this.Controls.Add(this.OkButton);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(580, 300);
			this.Name = "SyncFilter";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Sync Filter";
			this.TabControl.ResumeLayout(false);
			this.TabGlobal.ResumeLayout(false);
			this.TabGlobal.PerformLayout();
			this.TabWorkspace.ResumeLayout(false);
			this.TabWorkspace.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Button OkButton;
		private System.Windows.Forms.Button CancButton;
		private System.Windows.Forms.TextBox FilterTextGlobal;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TabControl TabControl;
		private System.Windows.Forms.TabPage TabGlobal;
		private System.Windows.Forms.TabPage TabWorkspace;
		private System.Windows.Forms.TextBox FilterTextWorkspace;
	}
}