// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace UnrealGameSync
{
	public partial class SyncFilter : Form
	{
		string[] DefaultLines =
		{
			"; Rules are specified one per line, and may use any standard Perforce wildcards:",
			";    ?    Matches one character.",
			";    *    Matches any sequence of characters, except for a directory separator.",
			";    ...  Matches any sequence of characters, including directory separators.",
			";",
			"; Patterns may match any file fragment (eg. *.pdb), or may be rooted to the branch (eg. /Engine/Binaries/.../*.pdb).",
			"; To exclude files which match a pattern, prefix the rule with the '-' character (eg. -/Engine/Documentation/...)",
			";",
			"; Global rules are applied to the files being synced first, followed by any workspace-specific patterns.",
			"",
			""
		};

		public string[] GlobalFilter;
		public string[] WorkspaceFilter;

		public SyncFilter(string[] InGlobalFilter, string[] InWorkspaceFilter)
		{
			InitializeComponent();

			GlobalFilter = InGlobalFilter;
			WorkspaceFilter = InWorkspaceFilter;
	
			SetLines(GlobalFilter, FilterTextGlobal);
			SetLines(WorkspaceFilter, FilterTextWorkspace);
		}

		private void SetLines(string[] Lines, TextBox FilterText)
		{
			if (Lines == null || !Lines.Any(x => x.Trim().Length > 0))
			{
				FilterText.Lines = DefaultLines;
			}
			else
			{
				FilterText.Lines = Lines;
			}
			FilterText.Select(FilterText.Text.Length, 0);
		}

		private string[] GetLines(TextBox FilterText)
		{
			List<string> NewLines = new List<string>(FilterText.Lines);
			while (NewLines.Count > 0 && DefaultLines.Any(x => x == NewLines[0].Trim()))
			{
				NewLines.RemoveAt(0);
			}
			while (NewLines.Count > 0 && NewLines.Last().Trim().Length == 0)
			{
				NewLines.RemoveAt(NewLines.Count - 1);
			}
			return NewLines.Count > 0 ? FilterText.Lines : new string[0];
		}

		private void OkButton_Click(object sender, EventArgs e)
		{
			GlobalFilter = GetLines(FilterTextGlobal);
			WorkspaceFilter = GetLines(FilterTextWorkspace);
			DialogResult = DialogResult.OK;
		}

		private void CancButton_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;
		}
	}
}
