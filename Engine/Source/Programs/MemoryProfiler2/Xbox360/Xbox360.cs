/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;

using Xbox360Tools;

namespace MemoryProfiler2
{
	public partial class FXbox360SymbolParser : ISymbolParser
	{
		private FXbox360ManagedSymbolParser Parser = null;
		public FXbox360SymbolParser()
		{
			Parser = new FXbox360ManagedSymbolParser();
		}

		public ModuleInfo[] ModuleInfo = new ModuleInfo[0];

		public new void ReadModuleInfo( BinaryReader BinaryStream, uint Count )
		{
			ModuleInfo = new ModuleInfo[Count];

			for( uint ModuleIndex = 0; ModuleIndex < Count; ModuleIndex++ )
			{
				ModuleInfo[ModuleIndex] = new ModuleInfo( BinaryStream );
			}
		}

		public new bool LoadSymbols( string ExePath )
		{
			return Parser.LoadSymbols( ExePath, "", true, ModuleInfo );
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
