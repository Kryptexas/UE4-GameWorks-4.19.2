// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __WindowsPlatformStackWalkExt_h__
#define __WindowsPlatformStackWalkExt_h__
#pragma once

#undef PLATFORM_SUPPORTS_STACK_SYMBOLS
#define PLATFORM_SUPPORTS_STACK_SYMBOLS 1

/**
 * Windows implementation of stackwalking
 **/
struct CORE_API FWindowsPlatformStackWalkExt : public FGenericPlatformStackWalk
{
	static bool InitStackWalking();
	static void ShutdownStackWalking();

	static void InitSymbols( class FCrashInfo* CrashInfo );
	static bool OpenDumpFile( FCrashInfo* CrashInfo, const FString& DumpFileName );
	static void GetModuleList( FCrashInfo* CrashInfo );
	static void SetSymbolPathsFromModules( FCrashInfo* CrashInfo );
	static void GetModuleInfoDetailed( FCrashInfo* CrashInfo );
	static void GetSystemInfo( FCrashInfo* CrashInfo );
	static void GetThreadInfo( FCrashInfo* CrashInfo );
	static void GetExceptionInfo( FCrashInfo* CrashInfo );
	static void GetCallstacks( FCrashInfo* CrashInfo );

	static bool IsOffsetWithinModules( FCrashInfo* CrashInfo, uint64 Offset );
	static FString ExtractRelativePath( const TCHAR* BaseName, TCHAR* FullName );
};

#endif