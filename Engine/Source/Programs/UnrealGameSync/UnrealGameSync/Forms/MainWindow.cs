// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.SqlClient;
using System.Deployment.Application;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.VisualStyles;
using Microsoft.Win32;

using EventWaitHandle = System.Threading.EventWaitHandle;

namespace UnrealGameSync
{
	interface IMainWindowTabPanel : IDisposable
	{
		void Activate();
		void Deactivate();
		void Hide();
		void Show();
		bool IsBusy();
		bool CanClose();
		bool CanSyncNow();
		void SyncLatestChange();
		bool CanLaunchEditor();
		void LaunchEditor();

		Tuple<TaskbarState, float> DesiredTaskbarState
		{
			get;
		}

		string SelectedFileName
		{
			get;
		}
	}

	partial class MainWindow : Form, IWorkspaceControlOwner
	{
		[DllImport("uxtheme.dll", CharSet = CharSet.Unicode)]
		static extern int SetWindowTheme(IntPtr hWnd, string pszSubAppName, string pszSubIdList);

		[DllImport("user32.dll")]
		public static extern int SendMessage(IntPtr hWnd, Int32 wMsg, Int32 wParam, Int32 lParam);

		private const int WM_SETREDRAW = 11; 

		UpdateMonitor UpdateMonitor;
		string SqlConnectionString;
		string DataFolder;
		ActivationListener ActivationListener;
		BoundedLogWriter Log;
		UserSettings Settings;
		int TabMenu_TabIdx = -1;
		int ChangingWorkspacesRefCount;

		bool bAllowClose = false;

		bool bUnstable;
		bool bRestoreStateOnLoad;

		System.Threading.Timer ScheduleTimer;

		string OriginalExecutableFileName;

		IMainWindowTabPanel CurrentTabPanel;

		public MainWindow(UpdateMonitor InUpdateMonitor, string InSqlConnectionString, string InDataFolder, EventWaitHandle ActivateEvent, bool bInRestoreStateOnLoad, string InOriginalExecutableFileName, string InProjectFileName, bool bInUnstable, BoundedLogWriter InLog, UserSettings InSettings)
		{
			InitializeComponent();

			NotifyMenu_OpenUnrealGameSync.Font = new Font(NotifyMenu_OpenUnrealGameSync.Font, FontStyle.Bold);

			UpdateMonitor = InUpdateMonitor;
			SqlConnectionString = InSqlConnectionString;
			DataFolder = InDataFolder;
			ActivationListener = new ActivationListener(ActivateEvent);
			bRestoreStateOnLoad = bInRestoreStateOnLoad;
			OriginalExecutableFileName = InOriginalExecutableFileName;
			bUnstable = bInUnstable;
			Log = InLog;
			Settings = InSettings;

			TabControl.OnTabChanged += TabControl_OnTabChanged;
			TabControl.OnNewTabClick += TabControl_OnNewTabClick;
			TabControl.OnTabClicked += TabControl_OnTabClicked;
			TabControl.OnTabClosing += TabControl_OnTabClosing;
			TabControl.OnTabClosed += TabControl_OnTabClosed;
			TabControl.OnTabReorder += TabControl_OnTabReorder;
			TabControl.OnButtonClick += TabControl_OnButtonClick;

			SetupDefaultControl();

			int SelectTabIdx = -1;
			foreach(string ProjectFileName in Settings.OpenProjectFileNames)
			{
				if(!String.IsNullOrEmpty(ProjectFileName))
				{
					int TabIdx = TryOpenProject(ProjectFileName);
					if(TabIdx != -1 && Settings.LastProjectFileName != null && ProjectFileName.Equals(Settings.LastProjectFileName, StringComparison.InvariantCultureIgnoreCase))
					{
						SelectTabIdx = TabIdx;
					}
				}
			}

			if(SelectTabIdx != -1)
			{
				TabControl.SelectTab(SelectTabIdx);
			}
			else if(TabControl.GetTabCount() > 0)
			{
				TabControl.SelectTab(0);
			}
		}

		void TabControl_OnButtonClick(int ButtonIdx, Point Location, MouseButtons Buttons)
		{
			if(ButtonIdx == 0)
			{
				BrowseForProject(TabControl.GetSelectedTabIndex());
			}
		}

		void TabControl_OnTabClicked(object TabData, Point Location, MouseButtons Buttons)
		{
			if(Buttons == System.Windows.Forms.MouseButtons.Right)
			{
				Activate();

				int InsertIdx = 0;

				while(TabMenu_RecentProjects.DropDownItems[InsertIdx] != TabMenu_Recent_Separator)
				{
					TabMenu_RecentProjects.DropDownItems.RemoveAt(InsertIdx);
				}

				TabMenu_TabIdx = -1;
				for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
				{
					if(TabControl.GetTabData(Idx) == TabData)
					{
						TabMenu_TabIdx = Idx;
						break;
					}
				}

				HashSet<string> ProjectList = new HashSet<string>(StringComparer.InvariantCultureIgnoreCase);
				foreach(string ProjectFileName in Settings.OtherProjectFileNames)
				{
					if(!String.IsNullOrEmpty(ProjectFileName))
					{
						string FullProjectFileName = Path.GetFullPath(ProjectFileName);
						if(ProjectList.Add(FullProjectFileName))
						{
							ToolStripMenuItem Item = new ToolStripMenuItem(FullProjectFileName, null, new EventHandler((o, e) => TryOpenProject(FullProjectFileName, TabMenu_TabIdx)));
							TabMenu_RecentProjects.DropDownItems.Insert(InsertIdx, Item);
							InsertIdx++;
						}
					}
				}

				TabMenu_RecentProjects.Visible = (ProjectList.Count > 0);

				TabMenu_TabNames_Stream.Checked = Settings.TabLabels == TabLabels.Stream;
				TabMenu_TabNames_WorkspaceName.Checked = Settings.TabLabels == TabLabels.WorkspaceName;
				TabMenu_TabNames_WorkspaceRoot.Checked = Settings.TabLabels == TabLabels.WorkspaceRoot;
				TabMenu_TabNames_ProjectFile.Checked = Settings.TabLabels == TabLabels.ProjectFile;
				TabMenu.Show(TabControl, Location);

				TabControl.LockHover();
			}
		}

		void TabControl_OnTabReorder()
		{
			SaveTabSettings();
		}

		void TabControl_OnTabClosed(object Data)
		{
			IMainWindowTabPanel TabPanel = (IMainWindowTabPanel)Data;
			if(CurrentTabPanel == TabPanel)
			{
				CurrentTabPanel = null;
			}
			TabPanel.Dispose();

			SaveTabSettings();
		}

		bool TabControl_OnTabClosing(object TabData)
		{
			IMainWindowTabPanel TabPanel = (IMainWindowTabPanel)TabData;
			return TabPanel.CanClose();
		}

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

			for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
			{
				((IMainWindowTabPanel)TabControl.GetTabData(Idx)).Dispose();
			}

			if(ActivationListener != null)
			{
				ActivationListener.Dispose();
				ActivationListener = null;
			}

			StopScheduleTimer();

			base.Dispose(disposing);
		}

		private void MainWindow_Load(object sender, EventArgs e)
		{
			if(Settings.bHasWindowSettings)
			{
				Rectangle WindowRectangle = Settings.WindowRectangle;
				if(Screen.AllScreens.Any(x => x.WorkingArea.IntersectsWith(WindowRectangle)))
				{
					Location = WindowRectangle.Location;
					Size = WindowRectangle.Size;
				}

				if(bRestoreStateOnLoad && !Settings.bWindowVisible)
				{
					// Yuck, flickery. Digging through the .NET reference source suggests that Application.ThreadContext 
					// forces the main form visible without any possibility to override.
					BeginInvoke(new MethodInvoker(() => Hide()));
				}
			}
			UpdateMonitor.OnUpdateAvailable += OnUpdateAvailableCallback;

			ActivationListener.Start();
			ActivationListener.OnActivate += OnActivationListenerAsyncCallback;

			StartScheduleTimer();
		}

		private void MainWindow_FormClosing(object Sender, FormClosingEventArgs EventArgs)
		{
			if(!bAllowClose && Settings.bKeepInTray)
			{
				Hide();
				EventArgs.Cancel = true; 
			}
			else
			{
				for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
				{
					IMainWindowTabPanel TabPanel = (IMainWindowTabPanel)TabControl.GetTabData(Idx);
					if(!TabPanel.CanClose())
					{
						EventArgs.Cancel = true;
						return;
					}
				}

				ActivationListener.OnActivate -= OnActivationListenerAsyncCallback;
				ActivationListener.Stop();

				UpdateMonitor.OnUpdateAvailable -= OnUpdateAvailable;
				UpdateMonitor.Close(); // prevent race condition

				StopScheduleTimer();

				Rectangle SaveBounds = (WindowState == FormWindowState.Normal)? Bounds : RestoreBounds;
				Settings.WindowRectangle = SaveBounds;
				Settings.bWindowVisible = Visible;

				Settings.Save();
			}
		}

		private void SetupDefaultControl()
		{
			List<StatusLine> Lines = new List<StatusLine>();

			StatusLine SummaryLine = new StatusLine();
			SummaryLine.AddText("To get started, open an existing Unreal project file on your hard drive.");
			Lines.Add(SummaryLine);

			StatusLine OpenLine = new StatusLine();
			OpenLine.AddLink("Open project...", FontStyle.Bold | FontStyle.Underline, () => { BrowseForProject(); });
			Lines.Add(OpenLine);

			DefaultControl.Set(Lines);
		}

		private void CreateErrorPanel(int ReplaceTabIdx, string ProjectFileName, string Message)
		{
			Log.WriteLine(Message);

			ErrorPanel ErrorPanel = new ErrorPanel(ProjectFileName);
			ErrorPanel.Parent = TabPanel;
			ErrorPanel.BorderStyle = BorderStyle.FixedSingle;
			ErrorPanel.BackColor = Color.White;
			ErrorPanel.Location = new Point(0, 0);
			ErrorPanel.Size = new Size(TabPanel.Width, TabPanel.Height);
			ErrorPanel.Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right | AnchorStyles.Bottom;
			ErrorPanel.Hide();

			List<StatusLine> Lines = new List<StatusLine>();

			StatusLine SummaryLine = new StatusLine();
			SummaryLine.AddText(String.Format("Unable to open '{0}'.", ProjectFileName));
			Lines.Add(SummaryLine);

			Lines.Add(new StatusLine(){ LineHeight = 0.5f });

			StatusLine ErrorLine = new StatusLine();
			ErrorLine.AddText(Message);
			Lines.Add(ErrorLine);

			Lines.Add(new StatusLine(){ LineHeight = 0.5f });

			StatusLine ActionLine = new StatusLine();
			ActionLine.AddLink("Close", FontStyle.Bold | FontStyle.Underline, () => { BeginInvoke(new MethodInvoker(() => { TabControl.RemoveTab(TabControl.FindTabIndex(ErrorPanel)); })); });
			ActionLine.AddText(" | ");
			ActionLine.AddLink("Retry", FontStyle.Bold | FontStyle.Underline, () => { BeginInvoke(new MethodInvoker(() => { TryOpenProject(ProjectFileName, TabControl.FindTabIndex(ErrorPanel)); })); });
			ActionLine.AddText(" | ");
			ActionLine.AddLink("Open another...", FontStyle.Bold | FontStyle.Underline, () => { BeginInvoke(new MethodInvoker(() => { BrowseForProject(TabControl.FindTabIndex(ErrorPanel)); })); });
			Lines.Add(ActionLine);

			ErrorPanel.Set(Lines);

			string NewTabName = "Error: " + Path.GetFileName(ProjectFileName);
			if (ReplaceTabIdx == -1)
			{
				TabControl.InsertTab(-1, NewTabName, ErrorPanel);
			}
			else
			{
				TabControl.InsertTab(ReplaceTabIdx + 1, NewTabName, ErrorPanel);
				TabControl.RemoveTab(ReplaceTabIdx);
			}

			UpdateProgress();
		}

		public void ShowAndActivate()
		{
			Show();
			Activate();
		}

		private void OnActivationListenerCallback()
		{
			// Check if we're trying to reopen with the unstable version; if so, trigger an update to trigger a restart with the new executable
			if(!bUnstable && (Control.ModifierKeys & Keys.Shift) != 0)
			{
				UpdateMonitor.TriggerUpdate();
			}
			else
			{
				ShowAndActivate();
			}
		}

		private void OnActivationListenerAsyncCallback()
		{
			BeginInvoke(new MethodInvoker(OnActivationListenerCallback));
		}

		private void OnUpdateAvailableCallback()
		{ 
			BeginInvoke(new MethodInvoker(OnUpdateAvailable));
		}

		private void OnUpdateAvailable()
		{
			if(!ContainsFocus && Form.ActiveForm != this)
			{
				for(int TabIdx = 0; TabIdx < TabControl.GetTabCount(); TabIdx++)
				{
					IMainWindowTabPanel TabPanel = (IMainWindowTabPanel)TabControl.GetTabData(TabIdx);
					if(TabPanel.IsBusy())
					{
						return;
					}
				}

				bAllowClose = true;
				Close();
			}
		}

		private void NotifyIcon_MouseDown(object sender, MouseEventArgs e)
		{
			// Have to set up this stuff here, because the menu is laid out before Opening() is called on it after mouse-up.
			if(CurrentTabPanel != null)
			{
				bool bCanSyncNow = CurrentTabPanel.CanSyncNow();
				bool bCanLaunchEditor = CurrentTabPanel.CanLaunchEditor();
				NotifyMenu_SyncNow.Visible = CurrentTabPanel.CanSyncNow();
				NotifyMenu_LaunchEditor.Visible = CurrentTabPanel.CanLaunchEditor();
				NotifyMenu_ExitSeparator.Visible = bCanSyncNow || bCanLaunchEditor;
			}
		}

		private void NotifyIcon_DoubleClick(object sender, EventArgs e)
		{
			ShowAndActivate();
		}

		private void NotifyMenu_OpenUnrealGameSync_Click(object sender, EventArgs e)
		{
			ShowAndActivate();
		}

		private void NotifyMenu_SyncNow_Click(object sender, EventArgs e)
		{
			CurrentTabPanel.SyncLatestChange();
		}

		private void NotifyMenu_LaunchEditor_Click(object sender, EventArgs e)
		{
			CurrentTabPanel.LaunchEditor();
		}

		private void NotifyMenu_Exit_Click(object sender, EventArgs e)
		{
			bAllowClose = true;
			Close();
		}

		private void MainWindow_Activated(object sender, EventArgs e)
		{
			if(CurrentTabPanel != null)
			{
				CurrentTabPanel.Activate();
			}
		}

		private void MainWindow_Deactivate(object sender, EventArgs e)
		{
			if(CurrentTabPanel != null)
			{
				CurrentTabPanel.Deactivate();
			}
		}

		public void SetupScheduledSync()
		{
			StopScheduleTimer();

			ScheduleWindow Schedule = new ScheduleWindow(Settings.bScheduleEnabled, Settings.ScheduleChange, Settings.ScheduleTime);
			if(Schedule.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				Schedule.CopySettings(out Settings.bScheduleEnabled, out Settings.ScheduleChange, out Settings.ScheduleTime);
				Settings.Save();
			}

			StartScheduleTimer();
		}

		private void StartScheduleTimer()
		{
			StopScheduleTimer();

			if(Settings.bScheduleEnabled)
			{
				DateTime CurrentTime = DateTime.Now;
				DateTime NextScheduleTime = new DateTime(CurrentTime.Year, CurrentTime.Month, CurrentTime.Day, Settings.ScheduleTime.Hours, Settings.ScheduleTime.Minutes, Settings.ScheduleTime.Seconds);

				if(NextScheduleTime < CurrentTime)
				{
					NextScheduleTime = NextScheduleTime.AddDays(1.0);
				}

				TimeSpan IntervalToFirstTick = NextScheduleTime - CurrentTime;
				ScheduleTimer = new System.Threading.Timer(x => BeginInvoke(new MethodInvoker(ScheduleTimerElapsed)), null, IntervalToFirstTick, TimeSpan.FromDays(1));
			}
		}

		private void StopScheduleTimer()
		{
			if(ScheduleTimer != null)
			{
				ScheduleTimer.Dispose();
				ScheduleTimer = null;
			}
		}

		private void ScheduleTimerElapsed()
		{
			for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
			{
				WorkspaceControl Workspace = TabControl.GetTabData(Idx) as WorkspaceControl;
				if(Workspace != null)
				{
					Workspace.ScheduleTimerElapsed();
				}
			}
		}

		void TabControl_OnTabChanged(object NewTabData)
		{
			SendMessage(Handle, WM_SETREDRAW, 0, 0);
			SuspendLayout();

			if(CurrentTabPanel != null)
			{
				CurrentTabPanel.Deactivate();
				CurrentTabPanel.Hide();
			}

			if(NewTabData == null)
			{
				CurrentTabPanel = null;
				Settings.LastProjectFileName = null;
				DefaultControl.Show();
			}
			else
			{
				CurrentTabPanel = (IMainWindowTabPanel)NewTabData;
				Settings.LastProjectFileName = CurrentTabPanel.SelectedFileName;
				DefaultControl.Hide();
			}

			Settings.Save();

			if(CurrentTabPanel != null)
			{
				CurrentTabPanel.Activate();
				CurrentTabPanel.Show();
			}

			ResumeLayout();

			SendMessage(Handle, WM_SETREDRAW, 1, 0);
			Refresh();
		}

		public void BrowseForProject(WorkspaceControl Workspace)
		{
			int TabIdx = TabControl.FindTabIndex(Workspace);
			if(TabIdx != -1)
			{
				BrowseForProject(TabIdx);
			}
		}

		public void RequestProjectChange(WorkspaceControl Workspace, string ProjectFileName)
		{
			int TabIdx = TabControl.FindTabIndex(Workspace);
			if(TabIdx != -1)
			{
				TryOpenProject(ProjectFileName, TabIdx);
			}
		}

		public void BrowseForProject(int ReplaceTabIdx = -1)
		{
			OpenFileDialog Dialog = new OpenFileDialog();
			Dialog.Filter = "Project files (*.uproject)|*.uproject|Project directory lists (*.uprojectdirs)|*.uprojectdirs|All supported files (*.uproject;*.uprojectdirs)|*.uproject;*.uprojectdirs|All files (*.*)|*.*" ;
			Dialog.FilterIndex = Settings.FilterIndex;
			if(Dialog.ShowDialog(this) == System.Windows.Forms.DialogResult.OK)
			{
				Settings.FilterIndex = Dialog.FilterIndex;
				Settings.OtherProjectFileNames = Enumerable.Concat(new string[]{ Path.GetFullPath(Dialog.FileName) }, Settings.OtherProjectFileNames).Where(x => !String.IsNullOrEmpty(x)).Distinct(StringComparer.InvariantCultureIgnoreCase).ToArray();
				Settings.Save();

				int TabIdx = TryOpenProject(Dialog.FileName, ReplaceTabIdx);
				if(TabIdx != -1)
				{
					TabControl.SelectTab(TabIdx);
					SaveTabSettings();
				}
			}
		}

		int TryOpenProject(string ProjectFileName, int ReplaceTabIdx = -1)
		{
			Log.WriteLine("Trying to open project {0}", ProjectFileName);

			// Normalize the filename
			ProjectFileName = Path.GetFullPath(ProjectFileName).Replace('/', Path.DirectorySeparatorChar);

			// Make sure the project exists
			if(!File.Exists(ProjectFileName))
			{
				CreateErrorPanel(ReplaceTabIdx, ProjectFileName, String.Format("{0} does not exist.", ProjectFileName));
				return -1;
			}

			// Check that none of the other tabs already have it open
			for(int TabIdx = 0; TabIdx < TabControl.GetTabCount(); TabIdx++)
			{
				if(ReplaceTabIdx != TabIdx)
				{
					WorkspaceControl Workspace = TabControl.GetTabData(TabIdx) as WorkspaceControl;
					if(Workspace != null)
					{
						if(Workspace.SelectedFileName.Equals(ProjectFileName, StringComparison.InvariantCultureIgnoreCase))
						{
							TabControl.SelectTab(TabIdx);
							return TabIdx;
						}
						else if(ProjectFileName.StartsWith(Workspace.BranchDirectoryName + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase))
						{
							if(MessageBox.Show(String.Format("{0} is already open under {1}.\n\nWould you like to close it?", Path.GetFileNameWithoutExtension(Workspace.SelectedFileName), Workspace.BranchDirectoryName, Path.GetFileNameWithoutExtension(ProjectFileName)), "Branch already open", MessageBoxButtons.YesNo) == System.Windows.Forms.DialogResult.Yes)
							{
								TabControl.RemoveTab(TabIdx);
							}
							else
							{
								return -1;
							}
						}
					}
				}
			}

			// Make sure the path case is correct. This can cause UBT intermediates to be out of date if the case mismatches.
			ProjectFileName = Utility.GetPathWithCorrectCase(new FileInfo(ProjectFileName));

			// Detect the project settings in a background thread
			using(DetectProjectSettingsTask DetectSettings = new DetectProjectSettingsTask(ProjectFileName, Log))
			{
				string ErrorMessage;
				if(!ModalTaskWindow.Execute(this, DetectSettings, "Opening Project", "Opening project, please wait...", out ErrorMessage))
				{
					if(!String.IsNullOrEmpty(ErrorMessage))
					{
						CreateErrorPanel(ReplaceTabIdx, ProjectFileName, ErrorMessage);
					}
					return -1;
				}

				// Hide the default control if it's visible
				DefaultControl.Hide();

				// Now that we have the project settings, we can construct the tab
				WorkspaceControl Workspace = new WorkspaceControl(this, SqlConnectionString, DataFolder, bRestoreStateOnLoad, OriginalExecutableFileName, ProjectFileName, bUnstable, DetectSettings, Log, Settings);
				Workspace.Parent = TabPanel;
				Workspace.Location = new Point(0, 0);
				Workspace.Size = new Size(TabPanel.Width, TabPanel.Height);
				Workspace.Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right | AnchorStyles.Bottom;
				Workspace.Hide();

				// Add the tab
				string NewTabName = GetTabName(Workspace);
				if(ReplaceTabIdx == -1)
				{
					int NewTabIdx = TabControl.InsertTab(-1, NewTabName, Workspace);
					return NewTabIdx;
				}
				else
				{
					TabControl.InsertTab(ReplaceTabIdx + 1, NewTabName, Workspace);
					TabControl.RemoveTab(ReplaceTabIdx);
					return ReplaceTabIdx;
				}
			}
		}

		public void StreamChanged(WorkspaceControl Workspace)
		{
			BeginInvoke(new MethodInvoker(() => StreamChangedCallback(Workspace)));
		}

		public void StreamChangedCallback(WorkspaceControl Workspace)
		{
			if(ChangingWorkspacesRefCount == 0)
			{
				ChangingWorkspacesRefCount++;

				for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
				{
					if(TabControl.GetTabData(Idx) == Workspace)
					{
						string ProjectFileName = Workspace.SelectedFileName;
						if(TryOpenProject(ProjectFileName, Idx) == -1)
						{
							TabControl.RemoveTab(Idx);
						}
						break;
					}
				}

				ChangingWorkspacesRefCount--;
			}
		}

		void SaveTabSettings()
		{
			List<string> OpenProjectFileNames = new List<string>();
			for(int TabIdx = 0; TabIdx < TabControl.GetTabCount(); TabIdx++)
			{
				IMainWindowTabPanel TabPanel = (IMainWindowTabPanel)TabControl.GetTabData(TabIdx);
				OpenProjectFileNames.Add(TabPanel.SelectedFileName);
			}
			Settings.OpenProjectFileNames = OpenProjectFileNames.ToArray();
			Settings.Save();
		}

		void TabControl_OnNewTabClick(Point Location, MouseButtons Buttons)
		{
			if(Buttons == MouseButtons.Left)
			{
				BrowseForProject();
			}
		}

		string GetTabName(WorkspaceControl Workspace)
		{
			switch(Settings.TabLabels)
			{
				case TabLabels.Stream:
					return Workspace.StreamName;
				case TabLabels.ProjectFile:
					return Workspace.SelectedFileName;
				case TabLabels.WorkspaceName:
					return Workspace.ClientName;
				case TabLabels.WorkspaceRoot:
				default:
					return Workspace.BranchDirectoryName;
			}
		}

		public void SetTabNames(TabLabels NewTabNames)
		{
			if(Settings.TabLabels != NewTabNames)
			{
				Settings.TabLabels = NewTabNames;
				Settings.Save();

				for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
				{
					WorkspaceControl Workspace = TabControl.GetTabData(Idx) as WorkspaceControl;
					if(Workspace != null)
					{
						TabControl.SetTabName(Idx, GetTabName(Workspace));
					}
				}
			}
		}

		private void TabMenu_OpenProject_Click(object sender, EventArgs e)
		{
			BrowseForProject(TabMenu_TabIdx);
		}

		private void TabMenu_TabNames_Stream_Click(object sender, EventArgs e)
		{
			SetTabNames(TabLabels.Stream);
		}

		private void TabMenu_TabNames_WorkspaceName_Click(object sender, EventArgs e)
		{
			SetTabNames(TabLabels.WorkspaceName);
		}

		private void TabMenu_TabNames_WorkspaceRoot_Click(object sender, EventArgs e)
		{
			SetTabNames(TabLabels.WorkspaceRoot);
		}

		private void TabMenu_TabNames_ProjectFile_Click(object sender, EventArgs e)
		{
			SetTabNames(TabLabels.ProjectFile);
		}

		private void TabMenu_RecentProjects_ClearList_Click(object sender, EventArgs e)
		{
			Settings.OtherProjectFileNames = new string[0];
			Settings.Save();
		}

		private void TabMenu_Closed(object sender, ToolStripDropDownClosedEventArgs e)
		{
			TabControl.UnlockHover();
		}

		private void RecentMenu_ClearList_Click(object sender, EventArgs e)
		{
			Settings.OtherProjectFileNames = new string[0];
			Settings.Save();
		}

		public void UpdateProgress()
		{
			TaskbarState State = TaskbarState.NoProgress;
			float Progress = -1.0f;

			for(int Idx = 0; Idx < TabControl.GetTabCount(); Idx++)
			{
				IMainWindowTabPanel TabPanel = (IMainWindowTabPanel)TabControl.GetTabData(Idx);

				Tuple<TaskbarState, float> DesiredTaskbarState = TabPanel.DesiredTaskbarState;
				if(DesiredTaskbarState.Item1 == TaskbarState.Error)
				{
					State = TaskbarState.Error;
					TabControl.SetHighlight(Idx, Tuple.Create(Color.FromArgb(204, 64, 64), 1.0f));
				}
				else if(DesiredTaskbarState.Item1 == TaskbarState.Paused && State != TaskbarState.Error)
				{
					State = TaskbarState.Paused;
					TabControl.SetHighlight(Idx, Tuple.Create(Color.FromArgb(255, 242, 0), 1.0f));
				}
				else if(DesiredTaskbarState.Item1 == TaskbarState.Normal && State != TaskbarState.Error && State != TaskbarState.Paused)
				{
					State = TaskbarState.Normal;
					Progress = Math.Max(Progress, DesiredTaskbarState.Item2);
					TabControl.SetHighlight(Idx, Tuple.Create(Color.FromArgb(28, 180, 64), DesiredTaskbarState.Item2));
				}
				else
				{
					TabControl.SetHighlight(Idx, null);
				}
			}

			if(State == TaskbarState.Normal)
			{
				Taskbar.SetState(Handle, TaskbarState.Normal);
				Taskbar.SetProgress(Handle, (ulong)(Progress * 1000.0f), 1000);
			}
			else
			{
				Taskbar.SetState(Handle, State);
			}
		}
	}
}
