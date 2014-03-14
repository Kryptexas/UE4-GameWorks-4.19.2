/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;

using PS3Tools;

namespace MemoryProfiler2
{
	public partial class FPS3SymbolParser : ISymbolParser
	{
		private FPS3ManagedSymbolParser Parser = null;
		public FPS3SymbolParser()
		{
			Parser = new FPS3ManagedSymbolParser();
		}

		public new void ReadModuleInfo( BinaryReader BinaryStream, uint Count )
		{
		}

		public new bool LoadSymbols( string ExePath )
		{
			return Parser.LoadSymbols( ExePath, "", false, null );
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
