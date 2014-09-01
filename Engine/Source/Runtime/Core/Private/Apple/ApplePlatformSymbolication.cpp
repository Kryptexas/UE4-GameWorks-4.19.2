// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ApplePlatformSymbolication.cpp: Apple platform implementation of symbolication
=============================================================================*/

#include "Core.h"

#include "ApplePlatformSymbolication.h"
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>

#if PLATFORM_MAC
extern "C"
{
	struct CSTypeRef
	{
		void* Buffer0;
		void* Buffer1;
	};
	
	typedef CSTypeRef CSSymbolicatorRef;
	typedef CSTypeRef CSSourceInfoRef;
	typedef CSTypeRef CSSymbolRef;
	typedef CSTypeRef CSSymbolOwnerRef;
	
	struct CSRange
	{
		uint64 Location;
		uint64 Length;
	};
	
	#define kCSNow								0x80000000u
	
	Boolean CSEqual(CSTypeRef CS1, CSTypeRef CS2);
	CFIndex CSGetRetainCount(CSTypeRef CS);
	Boolean CSIsNull(CSTypeRef CS);
	CSTypeRef CSRetain(CSTypeRef CS);
	void CSRelease(CSTypeRef CS);
	void CSShow(CSTypeRef CS);
	
	CSSymbolicatorRef CSSymbolicatorCreateWithTask(task_t Task);
	
	CSSourceInfoRef CSSymbolicatorGetSourceInfoWithAddressAtTime(CSSymbolicatorRef Symbolicator, vm_address_t Address, uint64_t Time);
	CSSymbolRef CSSymbolicatorGetSymbolWithMangledNameFromSymbolOwnerWithNameAtTime(CSSymbolicatorRef Symbolicator, const char* Symbol, const char* Name, uint64_t Time);
	
	const char* CSSymbolGetName(CSSymbolRef Symbol);
	CSRange CSSymbolGetRange(CSSymbolRef Symbol);
	CSSymbolOwnerRef CSSourceInfoGetSymbolOwner(CSSourceInfoRef Info);
	
	const char* CSSymbolOwnerGetName(CSSymbolOwnerRef symbol);
	
	int CSSourceInfoGetLineNumber(CSSourceInfoRef Info);
	const char* CSSourceInfoGetPath(CSSourceInfoRef Info);
	CSRange CSSourceInfoGetRange(CSSourceInfoRef Info);
	CSSymbolRef CSSourceInfoGetSymbol(CSSourceInfoRef Info);
}
#endif

bool FApplePlatformSymbolication::SymbolInfoForAddress(uint64 ProgramCounter, FProgramCounterSymbolInfo& Info)
{
#if PLATFORM_MAC
	bool bOK = false;
	CSSymbolicatorRef Symbolicator = CSSymbolicatorCreateWithTask(mach_task_self());
	if(!CSIsNull(Symbolicator))
	{
		CSSourceInfoRef Symbol = CSSymbolicatorGetSourceInfoWithAddressAtTime(Symbolicator, (vm_address_t)ProgramCounter, kCSNow);
		
		if(!CSIsNull(Symbol))
		{
			Info.LineNumber = CSSourceInfoGetLineNumber(Symbol);
			FCStringAnsi::Sprintf(Info.Filename, CSSourceInfoGetPath(Symbol));
			FCStringAnsi::Sprintf(Info.FunctionName, CSSymbolGetName(CSSourceInfoGetSymbol(Symbol)));
			CSRange CodeRange = CSSourceInfoGetRange(Symbol);
			Info.SymbolDisplacement = (ProgramCounter - CodeRange.Location);
			
			CSSymbolOwnerRef Owner = CSSourceInfoGetSymbolOwner(Symbol);
			if(!CSIsNull(Owner))
			{
				ANSICHAR const* DylibName = CSSymbolOwnerGetName(Owner);
				FCStringAnsi::Strcpy(Info.ModuleName, DylibName);
				
				bOK = Info.LineNumber != 0;
				
				CSRelease(Owner);
			}
			
			CSRelease(Symbol);
		}
		
		CSRelease(Symbolicator);
	}
	return bOK;
#else
	return false;
#endif
}

bool FApplePlatformSymbolication::SymbolInfoForFunctionFromModule(ANSICHAR const* MangledName, ANSICHAR const* ModuleName, FProgramCounterSymbolInfo& Info)
{
#if PLATFORM_MAC
	bool bOK = false;
	CSSymbolicatorRef Symbolicator = CSSymbolicatorCreateWithTask(mach_task_self());
	if(!CSIsNull(Symbolicator))
	{
		CSSymbolRef Symbol = CSSymbolicatorGetSymbolWithMangledNameFromSymbolOwnerWithNameAtTime(Symbolicator, MangledName, ModuleName, kCSNow);
		
		if(!CSIsNull(Symbol))
		{
			CSRange CodeRange = CSSymbolGetRange(Symbol);
		
			bOK = SymbolInfoForAddress(CodeRange.Location, Info);
	
			CSRelease(Symbol);
		}
		
		CSRelease(Symbolicator);
	}
	return bOK;
#else
	return false;
#endif
}
