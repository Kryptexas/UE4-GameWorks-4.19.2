// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel.Design;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;

namespace UnrealVS
{
	internal class BatchBuilder
	{
		/** constants */

		private const int BatchBuilderToolWindowId = 0x1300;

		/** methods */

		public BatchBuilder()
		{
			// Create the command for the tool window
			var ToolWindowCommandId = new CommandID(GuidList.UnrealVSCmdSet, BatchBuilderToolWindowId);
			var ToolWindowMenuCommand = new MenuCommand(ShowToolWindow, ToolWindowCommandId);
			UnrealVSPackage.Instance.MenuCommandService.AddCommand(ToolWindowMenuCommand);
		}

		/// <summary>
		/// Called from the package class when there are options to be read out of the solution file.
		/// </summary>
		/// <param name="Stream">The stream to load the option data from.</param>
		public void LoadOptions(Stream Stream)
		{
			BatchBuilderToolControl ToolControl = GetToolControl();

			if (ToolControl != null)
			{
				ToolControl.LoadOptions(Stream);
			}
		}

		/// <summary>
		/// Called from the package class when there are options to be written to the solution file.
		/// </summary>
		/// <param name="Stream">The stream to save the option data to.</param>
		public void SaveOptions(Stream Stream)
		{
			BatchBuilderToolControl ToolControl = GetToolControl();

			if (ToolControl != null)
			{
				ToolControl.SaveOptions(Stream);
			}
		}

		/// <summary>
		/// Tick function called from the package
		/// </summary>
		public void Tick()
		{
			BatchBuilderToolControl ToolControl = GetToolControl();

			if (ToolControl != null)
			{
				ToolControl.Tick();
			}
		}

		private void ShowToolWindow(object sender, EventArgs e)
		{
			// Get the instance number 0 of this tool window. This window is single instance so this instance
			// is actually the only one.
			// The last flag is set to true so that if the tool window does not exists it will be created.
			ToolWindowPane ToolWindow = UnrealVSPackage.Instance.FindToolWindow(typeof(BatchBuilderToolWindow), 0, true);
			if ((null == ToolWindow) || (null == ToolWindow.Frame))
			{
				throw new NotSupportedException(Resources.ToolWindowCreateError);
			}
			IVsWindowFrame ToolWindowFrame = (IVsWindowFrame)ToolWindow.Frame;
			Microsoft.VisualStudio.ErrorHandler.ThrowOnFailure(ToolWindowFrame.Show());
		}

		private BatchBuilderToolControl GetToolControl()
		{
			BatchBuilderToolWindow ToolWindow =
				UnrealVSPackage.Instance.FindToolWindow(typeof(BatchBuilderToolWindow), 0, false) as BatchBuilderToolWindow;

			if (ToolWindow != null)
			{
				return ToolWindow.Content as BatchBuilderToolControl;
			}
			return null;
		}
	}
}
