/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;

using IPhoneTools;

namespace MemoryProfiler2
{
	public partial class FIPhoneSymbolParser : ISymbolParser
	{
		private FIPhoneManagedSymbolParser Parser = null;
		public FIPhoneSymbolParser()
		{
			Parser = new IPhoneTools.FIPhoneManagedSymbolParser();
		}

		public override void ReadModuleInfo( BinaryReader BinaryStream, uint Count )
		{
		}

		public override bool LoadSymbols(string ExePath)
		{
			return Parser.LoadSymbols( ExePath, "", true, null );
		}

		public override void UnloadSymbols()
		{
			Parser.UnloadSymbols();
		}

		public override bool ResolveAddressToSymboInfo(ulong Address, ref string OutFileName, ref string OutFunction, ref int OutLineNumber)
		{
			return Parser.ResolveAddressToSymboInfo( Address, ref OutFileName, ref OutFunction, ref OutLineNumber );
		}

		public override bool ResolveAddressBatchesToSymbolInfo(ulong[] AddressList, ref string[] OutFileNames, ref string[] OutFunctions, ref int[] OutLineNumbers)
		{
			return Parser.ResolveAddressBatchesToSymbolInfo(AddressList, ref OutFileNames, ref OutFunctions, ref OutLineNumbers);
		}
	}
}
