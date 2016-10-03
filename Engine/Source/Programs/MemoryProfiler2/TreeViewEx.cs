/**
 * Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Drawing.Design;
using System.Collections.Specialized;

namespace MemoryProfiler2
{
	/// <summary> 
	/// Represents a Windows tree view control, which displays a collection of items in a tree.
	/// This version adds following extensions:
	///		Double buffered rendering
	/// </summary>
	public class TreeViewEx : System.Windows.Forms.TreeView
	{
		protected override void OnHandleCreated( EventArgs e )
		{
			TreeViewWin32.SendMessage( this.Handle, TreeViewWin32.TVM_SETEXTENDEDSTYLE, (IntPtr)TreeViewWin32.TVS_EX_DOUBLEBUFFER, (IntPtr)TreeViewWin32.TVS_EX_DOUBLEBUFFER );
			base.OnHandleCreated( e );
		}
	}

	/// <summary> Helper class with native Win32 functions, constants and structures. </summary>
	public class TreeViewWin32
	{
		[DllImport( "user32.dll", EntryPoint = "SendMessage" )]
		public static extern IntPtr SendMessage( IntPtr hwnd, UInt32 wMsg, IntPtr wParam, IntPtr lParam );

		public const UInt32 TV_FIRST = 0x1100;

		/// <summary> Informs the tree-view control to set extended styles. </summary>
		public const UInt32 TVM_SETEXTENDEDSTYLE = TV_FIRST + 44;

		/// <summary> Specifies how the background is erased or filled. </summary>
		public const UInt32 TVS_EX_DOUBLEBUFFER = 0x0004;
	}
}