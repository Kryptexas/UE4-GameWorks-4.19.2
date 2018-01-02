// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	/// <summary>
	/// Public Linux functions exposed to UAT
	/// </summary>
	public class LinuxExports
	{
		/// <summary>
		/// 
		/// </summary>
		/// <param name="SourceFile"></param>
		/// <param name="TargetFile"></param>
		public static void StripSymbols(FileReference SourceFile, FileReference TargetFile)
		{
			LinuxToolChain ToolChain = new LinuxToolChain(LinuxPlatform.DefaultArchitecture);
			ToolChain.StripSymbols(SourceFile, TargetFile);
		}
	}
}
