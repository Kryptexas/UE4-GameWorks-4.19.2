/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;

using AndroidTools;

namespace MemoryProfiler2
{
	public partial class FAndroidSymbolParser : ISymbolParser
	{
		private FAndroidManagedSymbolParser Parser = null;
		public FAndroidSymbolParser()
		{
			Parser = new AndroidTools.FAndroidManagedSymbolParser();
		}

		public new void ReadModuleInfo( BinaryReader BinaryStream, uint Count )
		{
		}

		public new bool LoadSymbols( string ExePath )
		{
			return Parser.LoadSymbols( ExePath, "", true, null );
		}

		public new void UnloadSymbols()
		{
			Parser.UnloadSymbols();
		}

		public new bool ResolveAddressToSymboInfo( ulong Address, ref string OutFileName, ref string OutFunction, ref int OutLineNumber )
		{
			return Parser.ResolveAddressToSymboInfo( Address, ref OutFileName, ref OutFunction, ref OutLineNumber );
		}
	}
}
